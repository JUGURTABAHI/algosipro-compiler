%{
  #include <stdlib.h>
  #include <stdio.h>
  #include <limits.h>
  #include <string.h>

  #include "commun.h"
  #include "AST.h"
  #include "compil.tab.h"

  void string_to_int(int *r, const char *s);
  void yytext_copy(char **copy, const char *s);
%}

%option noyywrap
%option yylineno

%x COMMENT

%%

"\\begin{algo}"   { return BEGIN_ALGO; }
"\\end{algo}"     { return END_ALGO;   } 




"\\COM"             { BEGIN(COMMENT); }
<COMMENT>"\\ENDCOM" { BEGIN(INITIAL); }
<COMMENT>\n         { /* ignore */ }
<COMMENT>.          { /* ignore */ }

"#".*               { /* ignore jusqu'à la fin de la ligne */ }
"%".*               { /* ignore jusqu'à la fin de la ligne */ }
"//".*              { /* ignore jusqu'à la fin de la ligne */ }

"\\SET"           { return SET;        }
"\\RETURN"        { return RETURN;     }
"\\IF"            { return IF;         }
"\\ELSE"          { return ELSE;       }
"\\FI"            { return FI;         }   
"\\DOWHILE"       { return DOWHILE;    }
"\\DOFORI"        { return DOFORI;     }

"\\CALL"          { return CALL;       }
"\\OD"            { return OD;         }
"\\PRINTD"        { return PRINTD;     }
"\\PRINTS"        { return PRINTS;     }

"{"               { return '{';        }
"}"               { return '}';        }
"("               { return '(';        }
")"               { return ')';        }

"&&"              { return AND;        }
"||"              { return OR;         }
"=="|"="          { return EQ;         }
"!="              { return NE;         }
"<="              { return LEQ;        }
">="              { return GTH;        }
"<"               { return '<';        }
">"               { return '>';        }

"+"               { return '+';        }
"-"               { return '-';        }
"*"               { return '*';        }
"/"               { return '/';        }
","               { return ',';        }
";"               { return ';';        }

\"([^"]*)\"  {
    yytext_copy(&yylval.id, yytext);
    return STRING;
}

[a-zA-Z_][a-zA-Z0-9_]*      { yytext_copy(&yylval.id, yytext); return IDENTIFIER;   }
[0-9]+                      { string_to_int(&yylval.integer, yytext); return INT;   }
[[:space:]]+                { /* ignorer caractères blancs */                       }
<<EOF>>                     { return 0;                                             }
.                           { fprintf(stderr, "Caractère inconnu: '%s'\n", yytext); }

%%

void string_to_int(int *r, const char *s) {
  char *p;
  long v;
  errno = 0;
  v = strtol(s, &p, 10);
  if (*p != '\0' || errno == ERANGE || v < INT_MIN || v > INT_MAX) {
    fprintf(stderr, "Error converting string to int\n");
    exit(EXIT_FAILURE);
  }
  *r = v;
}

void yytext_copy(char **id, const char *s) {
  char *copy = malloc(strlen(s) + 1);
  if (!copy) {
    fprintf(stderr, ANSI_RED "Memory allocation problem.\n" ANSI_RESET);
    exit(EXIT_FAILURE);
  }
  strcpy(copy, s);
  *id = copy;
}
