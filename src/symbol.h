#ifndef SYMBOL_H
#define SYMBOL_H

#include "ast.h"
#include <stdbool.h>
#include <stddef.h>

typedef struct SymbolEntry
{
    char *ident;
    TypeSpecifier type;
    size_t stackOffset;
    size_t size;
    bool isFunc;
} SymbolEntry;

typedef struct SymbolTable SymbolTable;

typedef struct SymbolTable
{
    SymbolTable *parentTable;
    SymbolTable *childTable;
    size_t currFrameOffset;

    SymbolEntry **entries;
    size_t size;
    size_t capacity;
} SymbolTable;

SymbolEntry *symbolEntryCreate(char *ident, TypeSpecifier type, size_t size, bool isFunc);
void symbolEntryDestroy(SymbolEntry *symbolEntry);

SymbolTable *symbolTableCreate(size_t symbolTableSize, SymbolTable *parentTable);
void symbolTableResize(SymbolTable *symbolTable, size_t symbolTableSize);
void symbolTablePush(SymbolTable *symbolTable, SymbolEntry *symbolEntry);
void symbolTableDestroy(SymbolTable *symbolTable);

SymbolEntry *getSymbolEntry(SymbolTable *symbolTable, char *ident);

SymbolTable *populateSymbolTable(FuncDef *rootExpr);

#endif
