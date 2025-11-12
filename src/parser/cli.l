%option noyywrap

%{
#include "cli.tab.h"
#include <stdlib.h>
#include <string.h>

extern char *yytext;
int yylex(void);
void yyerror(const char *s);
%}

%%

"create"        { return CREATE; }
"project"       { return PROJECT; }
"install"       { return INSTALL; }
"template"      { return TEMPLATE; }
"copy"          { return COPY; }
"delete"        { return DELETE; }
"clear"         { return CLEAR; }

[a-zA-Z_][a-zA-Z0-9_.]*  { 
    yylval.string = strdup(yytext);
    return IDENTIFIER;
}

\"([^\"\\]|\\.|\\\n)*\"    {
    yylval.string = strdup(yytext + 1);
    yylval.string[strlen(yylval.string) - 1] = '\0';
    return STRING;
}

'([^'\\]|\\.)*'            {  
    yylval.string = strdup(yytext + 1);
    yylval.string[strlen(yylval.string) - 1] = '\0';
    return STRING;
}

[ \t\r]+      { /* skip whitespace */ }

\n            { return NEWLINE; }

.             { return yytext[0]; }

%%