%{
#include <string.h> 
#include "polly/OptParser.h"

/* prototypes */
varNodeType *var(char *);
nodeType *realign(varNodeType *, varNodeType *, unsigned);
nodeType *isplit(varNodeType *, varNodeType *, varNodeType *, char *, unsigned);
nodeType *affine(varNodeType *, char *);
nodeType *lift(varNodeType *, varNodeType *, unsigned);

nodeType *stmtPtr;

int yylex(void);

void yyerror(char *s);
%}

%union {
    char* sValue;                /* variable name */
    nodeType *nPtr;
    unsigned intValue;
};

%token <intValue> INT
%token <sValue> VAR STRING
%token REALIGN ISPLIT AFFINE LIFT

%type <nPtr> stmt

%%


stmt:
        REALIGN '(' VAR ',' VAR ',' INT ')'                              { stmtPtr = realign(var($3), var($5), $7); }
	| '(' VAR ',' VAR ')' '=' ISPLIT '(' VAR ',' STRING ',' INT ')'  { stmtPtr = isplit(var($2), var($4), var($9), $11, $13);}
	| AFFINE '(' VAR ',' STRING ')'                                  { stmtPtr  = affine(var($3), $5); }
	| VAR '=' LIFT '(' VAR ',' INT ')'                               { stmtPtr = lift(var($1), var($5), $7);}
        ;

%%

void yyerror(char *s) {
    //fprintf(stdout, "%s\n", s);
}

varNodeType *var(char *s) {
    varNodeType *p;

    /* allocate node */
    if ((p = malloc(sizeof(varNodeType))) == NULL)
        yyerror("out of memory");

    p->name = strdup(s);
    return p;
}

nodeType *realign(varNodeType *l1, varNodeType *l2, unsigned n){
    nodeType *p;

    /* allocate node */
    if ((p = malloc(sizeof(nodeType))) == NULL)
        yyerror("out of memory");

    /* copy information */
    p->type = typeRealign;
    p->m.l1 = l1;
    p->m.l2 = l2;
    p->m.n = n;

    return p;
}

nodeType *isplit(varNodeType *r1, varNodeType *r2, varNodeType *l, char *pred, unsigned n){
    nodeType *p;

    /* allocate node */
    if ((p = malloc(sizeof(nodeType))) == NULL)
        yyerror("out of memory");

    /* copy information */
    p->type = typeISplit;
    p->is.r1 = r1;
    p->is.r2 = r2;
    p->is.l = l;
    p->is.pred = strdup(pred);
    p->is.n = n;

    return p;
}

nodeType *affine(varNodeType *l, char *trans){
    nodeType *p;

    /* allocate node */
    if ((p = malloc(sizeof(nodeType))) == NULL)
        yyerror("out of memory");

    /* copy information */
    p->type = typeAffine;
    p->a.l = l;
    p->a.trans = strdup(trans);

    return p;
}

nodeType *lift(varNodeType *r, varNodeType *l, unsigned n){
    nodeType *p;

    /* allocate node */
    if ((p = malloc(sizeof(nodeType))) == NULL)
        yyerror("out of memory");

    /* copy information */
    p->type = typeLift;
    p->l.l = l;
    p->l.r = r;
    p->l.n = n;

    return p;
}
