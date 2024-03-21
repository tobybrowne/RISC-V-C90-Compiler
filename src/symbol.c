#include <stddef.h>
#include <stdlib.h>

#include "ast.h"
#include "symbol.h"
#include <stdio.h>
#include <string.h>

size_t whileCount = 0;
size_t switchCount = 0;
size_t forCount = 0;

// constructor for symbol entry
SymbolEntry *symbolEntryCreate(char *ident, size_t storageSize, size_t typeSize, EntryType entryType)
{
    SymbolEntry *symbolEntry = malloc(sizeof(SymbolEntry));
    if (symbolEntry == NULL)
    {
        abort();
    }
    symbolEntry->ident = ident;
    switch (entryType)
    {
    case FUNCTION_ENTRY:
        symbolEntry->storageSize = storageSize + (4 * (2 + 11 + 7)) + (8 * (12)); // space allocated for ra and fp and s1-s11 and t0-t6 and ft0-ft11
        symbolEntry->typeSize = typeSize;
        break;

    case VARIABLE_ENTRY:
        symbolEntry->storageSize = storageSize;
        symbolEntry->typeSize = typeSize;
        break;

    case WHILE_ENTRY:
        symbolEntry->storageSize = 0;
        symbolEntry->typeSize = 0;
        break;

    case ARRAY_ENTRY:
        symbolEntry->storageSize = storageSize;
        symbolEntry->typeSize = typeSize;
        break;
    }
    symbolEntry->isGlobal = false;
    symbolEntry->entryType = entryType;
    return symbolEntry;
}

// destructor for symbol entry
void symbolEntryDestroy(SymbolEntry *symbolEntry)
{
    if (symbolEntry->entryType == WHILE_ENTRY || symbolEntry->entryType == SWITCH_ENTRY)
    {
        free(symbolEntry->ident);
    }
    free(symbolEntry);
}

// create symbol table
SymbolTable *symbolTableCreate(size_t entryLength, size_t childrenLength, SymbolTable *parentTable, SymbolEntry *masterFunc)
{
    SymbolTable *symbolTable = malloc(sizeof(SymbolTable));
    if (symbolTable == NULL)
    {
        abort();
    }

    // entry list constructor
    if (entryLength != 0)
    {
        symbolTable->entries = malloc(sizeof(SymbolEntry) * entryLength);
        if (symbolTable->entries == NULL)
        {
            abort();
        }
    }
    else
    {
        symbolTable->entries = NULL;
    }

    // child table list constructor
    if (childrenLength != 0)
    {
        symbolTable->childrenTables = malloc(sizeof(SymbolTable) * childrenLength);
        if (symbolTable->childrenTables == NULL)
        {
            abort();
        }
    }
    else
    {
        symbolTable->childrenTables = NULL;
    }

    symbolTable->parentTable = parentTable;
    symbolTable->masterFunc = masterFunc; // NULL for global scope

    symbolTable->entrySize = entryLength;
    symbolTable->entryCapacity = entryLength;

    symbolTable->childrenSize = childrenLength;
    symbolTable->chldrenCapacity = childrenLength;
    return symbolTable;
}

// resize the list of entries in a table
void entryListResize(SymbolTable *symbolTable, const size_t entryLength)
{
    if (symbolTable->entrySize != 0)
    {
        symbolTable->entrySize = entryLength;
        if (symbolTable->entrySize > symbolTable->entryCapacity)
        {
            while (symbolTable->entrySize > symbolTable->entryCapacity)
            {
                symbolTable->entryCapacity *= 2;
            }
            symbolTable->entries = realloc(symbolTable->entries, sizeof(SymbolEntry *) * symbolTable->entryCapacity);
            if (symbolTable->entries == NULL)
            {
                abort();
            }
        }
    }
    else
    {
        symbolTable->entrySize = entryLength;
        symbolTable->entries = malloc(sizeof(SymbolEntry *) * entryLength);
        if (symbolTable->entries == NULL)
        {
            abort();
        }
        symbolTable->entryCapacity = entryLength;
    }
}

// resize the list of children in a symbol table
void childrenListResize(SymbolTable *symbolTable, const size_t childrenLength)
{
    if (symbolTable->childrenSize != 0)
    {
        symbolTable->childrenSize = childrenLength;
        if (symbolTable->childrenSize > symbolTable->chldrenCapacity)
        {
            while (symbolTable->childrenSize > symbolTable->chldrenCapacity)
            {
                symbolTable->chldrenCapacity *= 2;
            }
            symbolTable->childrenTables = realloc(symbolTable->childrenTables, sizeof(SymbolTable *) * symbolTable->chldrenCapacity);
            if (symbolTable->childrenTables == NULL)
            {
                abort();
            }
        }
    }
    else
    {
        symbolTable->childrenSize = childrenLength;
        symbolTable->childrenTables = malloc(sizeof(SymbolTable *) * childrenLength);
        if (symbolTable->childrenTables == NULL)
        {
            abort();
        }
        symbolTable->chldrenCapacity = childrenLength;
    }
}

// Adds a symbol table entry to a symbol table
void entryPush(SymbolTable *symbolTable, SymbolEntry *symbolEntry)
{
    entryListResize(symbolTable, symbolTable->entrySize + 1);
    symbolTable->entries[symbolTable->entrySize - 1] = symbolEntry;

    // only update stack values of local decls, global is not on the stack
    if (symbolTable->masterFunc != NULL)
    {
        // first element stored at sp + 0
        symbolTable->masterFunc->storageSize += symbolEntry->storageSize;
        symbolEntry->stackOffset = symbolTable->masterFunc->storageSize;
    }
}

// Adds a child symbol table to a symbol table
void childTablePush(SymbolTable *symbolTable, SymbolTable *childTable)
{
    childrenListResize(symbolTable, symbolTable->childrenSize + 1);
    symbolTable->childrenTables[symbolTable->childrenSize - 1] = childTable;
}

// destructor for symbol table
void symbolTableDestroy(SymbolTable *symbolTable)
{
    for (size_t i = 0; i < symbolTable->entrySize; i++)
    {
        symbolEntryDestroy(symbolTable->entries[i]);
    }
    free(symbolTable->entries);

    if (symbolTable->childrenTables != NULL)
    {
        for (size_t i = 0; i < symbolTable->childrenSize; i++)
        {
            symbolTableDestroy(symbolTable->childrenTables[i]);
        }
    }
    free(symbolTable->childrenTables);
    free(symbolTable);
}

// overlapping names of variables and functions !?!?
SymbolEntry *getSymbolEntry(SymbolTable *symbolTable, char *ident, EntryType entryType)
{
    // base case
    if (symbolTable == NULL)
    {
        return NULL;
    }
    // search current table
    for (size_t i = 0; i < symbolTable->entrySize; i++)
    {
        if (strcmp(symbolTable->entries[i]->ident, ident) == 0 && entryType == symbolTable->entries[i]->entryType)
        {
            return symbolTable->entries[i];
        }
    }
    // search further tables
    return getSymbolEntry(symbolTable->parentTable, ident, entryType);
}

// prints a symbol entry to the terminal
void displaySymbolEntry(SymbolEntry *symbolEntry)
{
    char *type;
    switch (symbolEntry->type.dataType)
    {
    case INT_TYPE:
    {
        type = "INT";
        break;
    }
    case FLOAT_TYPE:
    {
        type = "FLOAT";
        break;
    }
    case CHAR_TYPE:
    {
        type = "CHAR";
        break;
    }
    case LONG_TYPE:
    {
        type = "LONG";
        break;
    }
    case INT_POINTER_TYPE:
    {
        type = "INT*";
        break; 
    }
    case CHAR_POINTER_TYPE:
    {
        type = "CHAR*";
        break; 
    }
    case VOID_POINTER_TYPE:
    {
        type = "VOID*";
        break; 
    }
    default:
    {
        type = "NULL";
    }
    }

    char *localGlobal;
    if (symbolEntry->isGlobal)
    {
        localGlobal = "GLOBAL";
    }
    else
    {
        localGlobal = "LOCAL";
    }
    printf("0x%x | %s | %zu | %zu | %zu | %s | %s\n", symbolEntry, symbolEntry->ident, symbolEntry->stackOffset, symbolEntry->typeSize, symbolEntry->storageSize, type, localGlobal);
}

// prints a symbol table and it's children to the terminal
void displaySymbolTable(SymbolTable *symbolTable)
{
    if (symbolTable->masterFunc == NULL)
    {
        printf("GLOBAL SCOPE\n");
    }
    else
    {
        printf("%s SCOPE\n", symbolTable->masterFunc->ident);
    }
    printf("===============================\n");

    for (size_t i = 0; i < symbolTable->entrySize; i++)
    {
        displaySymbolEntry(symbolTable->entries[i]);
    }
    printf("\n");

    if (symbolTable->childrenTables != NULL)
    {
        for (size_t i = 0; i < symbolTable->childrenSize; i++)
        {
            displaySymbolTable(symbolTable->childrenTables[i]);
        }
    }
}

void scanStmt(Stmt *stmt, SymbolTable *parentTable);
void scanExpr(Expr *expr, SymbolTable *parentTable);

void scanInitList(InitList *initList, SymbolTable *parentTable)
{
    for (size_t i = 0; i < initList->size; i++)
    {
        // expression initialiser
        if (initList->inits[i]->expr != NULL)
        {
            scanExpr(initList->inits[i]->expr, parentTable);
        }
        // list initializer
        else
        {
            scanInitList(initList->inits[i]->initList, parentTable);
        }
    }
}

// declaration second pass
void scanDecl(Decl *decl, SymbolTable *symbolTable)
{
    char *ident = decl->declInit->declarator->ident;
    TypeSpecifier type = *(decl->typeSpecList->typeSpecs[0]); // assumes a list of length 1 after type resolution stuff

    if (decl->declInit->declarator->isArray)
    {
        // int arraySize = evaluateConstantExpr(decl->declInit->declarator->arraySize);

        int arraySize = decl->declInit->declarator->arraySize->constant->int_const;
        SymbolEntry *symbolEntry = symbolEntryCreate(ident, storageSize(type.dataType) * arraySize, typeSize(type.dataType), ARRAY_ENTRY);
        symbolEntry->type = type;
        entryPush(symbolTable, symbolEntry);
        decl->symbolEntry = symbolEntry;

        if (decl->declInit->initList != NULL)
        {
            scanInitList(decl->declInit->initList, symbolTable);
        }
    }
    else
    {
        SymbolEntry *symbolEntry = symbolEntryCreate(ident, storageSize(type.dataType), typeSize(type.dataType), VARIABLE_ENTRY);
        symbolEntry->type = type;
        entryPush(symbolTable, symbolEntry);
        decl->symbolEntry = symbolEntry;

        if (decl->declInit->initExpr != NULL)
        {
            scanExpr(decl->declInit->initExpr, symbolTable);
        }
    }
}

// function expression second pass
void scanFuncExpr(FuncExpr *funcExpr, SymbolTable *parentTable)
{
    funcExpr->symbolEntry = getSymbolEntry(parentTable, funcExpr->ident, FUNCTION_ENTRY);
    for (size_t i = 0; i < funcExpr->argsSize; i++)
    {
        scanExpr(funcExpr->args[i], parentTable);
    }
}

// assignment second pass
void scanAssignment(AssignExpr *assignExpr, SymbolTable *parentTable)
{
    assignExpr->symbolEntry = getSymbolEntry(parentTable, assignExpr->ident, VARIABLE_ENTRY);

    if (assignExpr->lvalue != NULL)
    {
        scanExpr(assignExpr->lvalue, parentTable);
    }
    scanExpr(assignExpr->op, parentTable);
}

// variable second pass
void scanVariable(VariableExpr *variable, SymbolTable *parentTable)
{
    variable->symbolEntry = getSymbolEntry(parentTable, variable->ident, VARIABLE_ENTRY);
}

// operation expression second pass
void scanOperationExpr(OperationExpr *opExpr, SymbolTable *parentTable)
{
    scanExpr(opExpr->op1, parentTable);
    if (opExpr->op2 != NULL)
    {
        scanExpr(opExpr->op2, parentTable);
    }
    if (opExpr->op3 != NULL)
    {
        scanExpr(opExpr->op3, parentTable);
    }
}

// expression second pass
void scanExpr(Expr *expr, SymbolTable *parentTable)
{
    switch (expr->type)
    {
    case VARIABLE_EXPR:
    {
        scanVariable(expr->variable, parentTable);
        expr->variable->type = expr->variable->symbolEntry->type.dataType;
        break;
    }
    case CONSTANT_EXPR:
    {
        // doesnt require a second pass
        break;
    }
    case OPERATION_EXPR:
    {
        scanOperationExpr(expr->operation, parentTable);

        // this is botttom-up because of the recursive call previously
        if (expr->operation->operator== SIZEOF_OP)
        {
            expr->operation->type = UNSIGNED_INT_TYPE;
        }
        else
        {
            DataType op1Type = returnType(expr->operation->op1);
            DataType op2Type = returnType(expr->operation->op2);
            if(op1Type == INT_POINTER_TYPE || op1Type == CHAR_POINTER_TYPE || op1Type == VOID_POINTER_TYPE)
            {
                expr->operation->type = op1Type;
            }
            else if(op2Type == INT_POINTER_TYPE || op2Type == CHAR_POINTER_TYPE || op2Type == VOID_POINTER_TYPE)
            {
                expr->operation->type = op1Type;
            }
            else
            {
                expr->operation->type = op1Type;
            }
        }
        break;
    }
    case ASSIGN_EXPR:
    {
        scanAssignment(expr->assignment, parentTable);
        expr->assignment->type = returnType(expr->assignment->op);
        break;
    }
    case FUNC_EXPR:
    {
        scanFuncExpr(expr->function, parentTable);
        if (expr->function->symbolEntry != NULL)
        {
            expr->function->type = expr->function->symbolEntry->type.dataType;
        }
        else
        {
            printf("NULL");
        }
        break;
    }
    }
}

// switch statement second pass
void scanSwitchStmt(SwitchStmt *switchStmt, SymbolTable *parentTable)
{
    int strSize = snprintf(NULL, 0, "%lu", switchCount);
    char *switchID = malloc((strSize + 1) * sizeof(char));
    if (switchID == NULL)
    {
        abort();
    }
    sprintf(switchID, "%lu", whileCount);

    SymbolEntry *switchEntry = symbolEntryCreate(switchID, 0, 0, SWITCH_ENTRY);
    entryPush(parentTable, switchEntry);
    switchStmt->symbolEntry = switchEntry;
    switchCount += 1;
    scanExpr(switchStmt->selector, parentTable);
    scanStmt(switchStmt->body, parentTable);
}

// if statement second pass
void scanIfStmt(IfStmt *ifStmt, SymbolTable *parentTable)
{
    scanExpr(ifStmt->condition, parentTable);
    scanStmt(ifStmt->trueBody, parentTable);
    if (ifStmt->falseBody != NULL)
    {
        scanStmt(ifStmt->falseBody, parentTable);
    }
}

// for statement second pass
void scanForStmt(ForStmt *forStmt, SymbolTable *parentTable)
{
    int strSize = snprintf(NULL, 0, "%lu", forCount);
    char *forID = malloc((strSize + 1) * sizeof(char));
    if (forID == NULL)
    {
        abort();
    }
    sprintf(forID, "%lu", forCount);

    SymbolEntry *forEntry = symbolEntryCreate(forID, 0, 0, FOR_ENTRY); // make identifier work
    entryPush(parentTable, forEntry);
    forStmt->symbolEntry = forEntry;
    forCount += 1;

    scanStmt(forStmt->init, parentTable);
    scanStmt(forStmt->condition, parentTable);
    scanStmt(forStmt->body, parentTable);
    scanExpr(forStmt->modifier, parentTable);
}

// while statement second pass
void scanWhileStmt(WhileStmt *whileStmt, SymbolTable *parentTable)
{
    int strSize = snprintf(NULL, 0, "%lu", whileCount);
    char *whileID = malloc((strSize + 1) * sizeof(char));
    if (whileID == NULL)
    {
        abort();
    }
    sprintf(whileID, "%lu", whileCount);

    SymbolEntry *whileEntry = symbolEntryCreate(whileID, 0, 0, WHILE_ENTRY); // make identifier work
    entryPush(parentTable, whileEntry);
    whileStmt->symbolEntry = whileEntry;
    whileCount += 1;
    scanExpr(whileStmt->condition, parentTable);
    scanStmt(whileStmt->body, parentTable);
}

// compound statements (not in function defs) create a new scope table but not a new stack frame
void scanCompoundStmt(CompoundStmt *compoundStmt, SymbolTable *parentTable)
{
    // enter a new scope but not a new stack frame
    SymbolTable *childTable = symbolTableCreate(0, 0, parentTable, parentTable->masterFunc);
    childTablePush(parentTable, childTable);

    for (size_t i = 0; i < compoundStmt->declList.size; i++)
    {
        scanDecl(compoundStmt->declList.decls[i], childTable);
    }

    for (size_t i = 0; i < compoundStmt->stmtList.size; i++)
    {
        scanStmt(compoundStmt->stmtList.stmts[i], childTable);
    }
}

// finds closest switch
SymbolEntry *getClosestSwitch(SymbolTable *symbolTable)
{
    // base case
    if (symbolTable == NULL)
    {
        return NULL;
    }
    // search in reverse order (most recent switch)
    for (size_t i = 0; i < symbolTable->entrySize; i++)
    {
        SymbolEntry *currEntry = symbolTable->entries[symbolTable->entrySize - i - 1];
        if (currEntry->entryType == SWITCH_ENTRY)
        {
            return symbolTable->entries[symbolTable->entrySize - i - 1];
        }
    }

    // search further tables
    return getClosestSwitch(symbolTable->parentTable);
}

// label second pass
void scanLabelStmt(LabelStmt *labelStmt, SymbolTable *parentTable)
{
    labelStmt->symbolEntry = getClosestSwitch(parentTable);
    scanExpr(labelStmt->caseLabel, parentTable);
    scanStmt(labelStmt->body, parentTable);
}

// finds closest while/for/switch (closest scope)
SymbolEntry *getClosestBreakable(SymbolTable *symbolTable)
{
    // base case
    if (symbolTable == NULL)
    {
        return NULL;
    }
    // search in reverse order (most recent while)
    for (size_t i = 0; i < symbolTable->entrySize; i++)
    {
        SymbolEntry *currEntry = symbolTable->entries[symbolTable->entrySize - i - 1];
        if (currEntry->entryType == WHILE_ENTRY || currEntry->entryType == SWITCH_ENTRY || currEntry->entryType == FOR_ENTRY)
        {
            return symbolTable->entries[symbolTable->entrySize - i - 1];
        }
    }

    // search further tables
    return getClosestBreakable(symbolTable->parentTable);
}

// jump statement second pass
void scanJumpStmt(JumpStmt *jumpStmt, SymbolTable *parentTable)
{
    jumpStmt->symbolEntry = getClosestBreakable(parentTable->parentTable);
    if (jumpStmt->expr != NULL)
    {
        scanExpr(jumpStmt->expr, parentTable);
    }
}

// statement second pass
void scanStmt(Stmt *stmt, SymbolTable *parentTable)
{
    switch (stmt->type)
    {
    case WHILE_STMT:
        scanWhileStmt(stmt->whileStmt, parentTable);
        break;
    case FOR_STMT:
        scanForStmt(stmt->forStmt, parentTable);
        break;
    case IF_STMT:
        scanIfStmt(stmt->ifStmt, parentTable);
        break;
    case SWITCH_STMT:
        scanSwitchStmt(stmt->switchStmt, parentTable);
        break;
    case EXPR_STMT:
        scanExpr(stmt->exprStmt->expr, parentTable);
        break;
    case COMPOUND_STMT:
        scanCompoundStmt(stmt->compoundStmt, parentTable);
        break;
    case LABEL_STMT:
        scanLabelStmt(stmt->labelStmt, parentTable);
        break;
    case JUMP_STMT:
        scanJumpStmt(stmt->jumpStmt, parentTable);
        break;
    }
}

// function definition second pass
void scanFuncDef(FuncDef *funcDef, SymbolTable *parentTable)
{
    // new function def symbol entry
    SymbolEntry *funcDefEntry = symbolEntryCreate(funcDef->ident, 0, 0, FUNCTION_ENTRY);
    funcDefEntry->type = *(funcDef->retType->typeSpecs[0]);
    funcDefEntry->isGlobal = true;
    entryPush(parentTable, funcDefEntry);
    funcDef->symbolEntry = funcDefEntry;

    // new scope and new stack frame
    SymbolTable *childTable = symbolTableCreate(0, 0, parentTable, funcDefEntry);
    childTablePush(parentTable, childTable);

    if (funcDef->isParam)
    {
        // arguments added to child scope
        for (size_t i = 0; i < funcDef->args.size; i++)
        {
            if (funcDef->args.decls[i]->declInit != NULL)
            {
                scanDecl(funcDef->args.decls[i], childTable);
            }
        }
    }

    if (funcDef->body != NULL)
    {
        // add body to child table
        for (size_t i = 0; i < funcDef->body->compoundStmt->declList.size; i++)
        {
            scanDecl(funcDef->body->compoundStmt->declList.decls[i], childTable);
        }

        for (size_t i = 0; i < funcDef->body->compoundStmt->stmtList.size; i++)
        {
            scanStmt(funcDef->body->compoundStmt->stmtList.stmts[i], childTable);
        }
    }
}

// translation unit second pass
void scanTransUnit(TranslationUnit *transUnit, SymbolTable *parentTable)
{
    for (size_t i = 0; i < transUnit->size; i++)
    {
        if (transUnit->externDecls[i]->isFunc)
        {
            scanFuncDef(transUnit->externDecls[i]->funcDef, parentTable);
        }
        else
        {
            scanDecl(transUnit->externDecls[i]->decl, parentTable);
        }
    }
}

// starts the second pass
SymbolTable *populateSymbolTable(TranslationUnit *rootExpr)
{
    SymbolTable *globalTable = symbolTableCreate(0, 0, NULL, NULL); // global scope
    scanTransUnit(rootExpr, globalTable);
    return globalTable;
}

// returns the size of a given datatype
size_t typeSize(DataType type)
{
    switch (type)
    {
    case VOID_TYPE:
        return 4;
    case CHAR_TYPE:
        return 1;
    case SIGNED_CHAR_TYPE:
        return 1;
    case SHORT_TYPE:
        return 2;
    case UNSIGNED_SHORT_TYPE:
        return 2;
    case INT_TYPE:
        return 4;
    case UNSIGNED_INT_TYPE:
        return 4;
    case LONG_TYPE:
        return 8;
    case UNSIGNED_LONG_TYPE:
        return 8;
    case FLOAT_TYPE:
        return 4;
    case DOUBLE_TYPE:
        return 8;
    case INT_POINTER_TYPE:
        return 4;
    case CHAR_POINTER_TYPE:
        return 4;
    case VOID_POINTER_TYPE:
        return 4;

    default:
    {
        fprintf(stderr, "Type does not have a size, exiting...\n");
        exit(EXIT_FAILURE);
    }
    }
}

// returns the memory required dor a given datatype (different to type size)
size_t storageSize(DataType type)
{
    switch (type)
    {
    case VOID_TYPE:
        return 4;
    case CHAR_TYPE:
        return 4;
    case SIGNED_CHAR_TYPE:
        return 4;
    case SHORT_TYPE:
        return 4;
    case UNSIGNED_SHORT_TYPE:
        return 4;
    case INT_TYPE:
        return 4;
    case UNSIGNED_INT_TYPE:
        return 4;
    case LONG_TYPE:
        return 8;
    case UNSIGNED_LONG_TYPE:
        return 8;
    case FLOAT_TYPE:
        return 8;
    case DOUBLE_TYPE:
        return 8;
    case INT_POINTER_TYPE:
        return 4;
    case CHAR_POINTER_TYPE:
        return 4;
    case VOID_POINTER_TYPE:
        return 4;

    default:
    {
        fprintf(stderr, "Type does not have a storage size, exiting...\n");
        exit(EXIT_FAILURE);
    }
    }
}
