%{
#include <string.h>
#include <stdlib.h>
#include "polly/OptParser.h"
#include "y.tab.h"

void yyerror(char *);
%}

%%

\{(\\.|[^"])*\} {
		yylval.sValue = strdup(yytext);
		return STRING;
	    }

"realign"     {
		return REALIGN;
	    }

"isplit"    { 
		return ISPLIT;
	    }

"affine"    { 
		return AFFINE;
	    }

"lift"    { 
		return LIFT;
	    }

[0-9]+ {
            yylval.intValue = atoi(yytext);
            return INT;
        }

[A-Za-z][A-Za-z0-9]* { 
                yylval.sValue = strdup(yytext);
                return VAR;
            }

[()=;,] {
                return *yytext;
             }

[ \t\n]+        ;       /* ignore whitespace */

.               yyerror("Unknown character");
%%
int yywrap(void) {
    return 1;
}


