%{
#include <stdio.h>
#include <stdlib.h>
#include "cli.tab.h"
#include "commands/commands.h"

int yylex(void);
void yyerror(const char *s);
%}

%union {
    char *string;
}

%token CREATE PROJECT INSTALL TEMPLATE COPY DELETE CLEAR
%token <string> IDENTIFIER STRING
%token NEWLINE

%%

commands:
      /* empty */                /* allow empty input */
    | command commands           /* multiple commands */
    | NEWLINE commands           /* allow blank lines between commands */
;

command:
      create_project NEWLINE
    | install_template NEWLINE
    | copy_template NEWLINE
    | delete_template NEWLINE
    | clear_terminal NEWLINE
;

install_template: 
    INSTALL TEMPLATE STRING { install_template_cmd($3); }

create_project:
    CREATE PROJECT { create_project_cmd(); }
;

copy_template:
    COPY TEMPLATE { copy_template_cmd(); }
;

delete_template: 
    DELETE TEMPLATE { delete_template_cmd(); }
;

clear_terminal:
    CLEAR { clear_terminal_cmd(); }
;

%%

void yyerror(const char *s) {
    fprintf(stderr, "Parse error: %s\n", s);
    fflush(stderr);
    fflush(stdout);
}