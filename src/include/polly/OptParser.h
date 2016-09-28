#ifndef POLLY_OPT_PARSER
#define POLLY_OPT_PARSER

typedef enum { typeRealign, typeISplit, typeAffine, typeLift } nodeEnum;

// Variable
typedef struct {
    char * name;
} varNodeType;

// Realign Node Type
typedef struct {
    varNodeType * l1;
    varNodeType * l2;
    unsigned n;
} realignNodeType;

// Index Split Node Type
typedef struct {
	varNodeType * l;
	varNodeType * r1;
	varNodeType * r2;
	char * pred;
    unsigned n;
} isplitNodeType;

// Affine Transform Node Type
typedef struct {
	varNodeType * l;
	char * trans;
} affineNodeType;

// Lift Node Type
typedef struct {
	varNodeType * l;
	varNodeType * r;
	unsigned n;	
} liftNodeType;


typedef struct {
    nodeEnum type;              /* type of node */

    union {
        realignNodeType m;       
        isplitNodeType is;       
        affineNodeType a;       
        liftNodeType l;       
    };
} nodeType;

extern nodeType *stmtPtr;

#endif
