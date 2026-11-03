#ifndef SITH_COG_YACC_H
#define SITH_COG_YACC_H

#include <rdroid/types.h>
#include <sith/types.h>
#include <std/types.h>

J3D_EXTERN_C_START

#define IDENTIFIER 257
#define CONSTANT_INT 258
#define CONSTANT_FLOAT 259
#define STRING_LITERAL 260
#define VECTOR_LITERAL 261
#define LE_OP 262
#define GE_OP 263
#define EQ_OP 264
#define NE_OP 265
#define AND_OP 266
#define OR_OP 267
#define TYPE_NAME 268
#define IFX 269
#define IF 270
#define ELSE 271
#define SWITCH 272
#define WHILE 273
#define DO 274
#define FOR 275
#define GOTO 276
#define CONTINUE 277
#define BREAK 278
#define RETURN 279
#define CALL 280
typedef union {
    void *pointerValue;
    float floatValue;
    int intValue;
    rdVector3 vecValue;
    SithCogSyntaxNode *pNode;
} YYSTYPE;
extern YYSTYPE yylval;

extern size_t yylinenum;
extern int yyleng;
extern tFileHandle yyin, yyout;

int yyparse(void);
int yylex(void);
void yyrestart(tFileHandle input_file);

static void sithCogYacc_ResetGlobals(void){}

J3D_EXTERN_C_END

#endif //SITH_COG_YACC_H