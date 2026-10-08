%{
#include "ast.h"
#include "parser.tab.h"
#include "string_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int show_tokens = 0;
int lexical_errors = 0;
int previous_token_line = 0;
static int last_token_line = 0;

static void print_token(const char *name) {
    printf("  linea %-3d %-22s %s\n", yylineno, name, yytext);
}

#define TOKEN(t) do {                         \
        previous_token_line = last_token_line; \
        last_token_line = yylineno;            \
        if (show_tokens) print_token(#t);      \
        return t;                              \
    } while (0)

static void lexical_error(const char *message) {
    lexical_errors++;
    fprintf(stderr, "DERROTA en la linea %d: %s '%s'\n", yylineno, message, yytext);
}
%}

%option noyywrap
%option yylineno
%option 8bit

%%

[ \t\r\n]+             ; /* Skip whitespace */
"#".*                  ;

"fundar_ciudad"        { yylval.line = yylineno; TOKEN(TOKEN_PROGRAM_START); }
"victoria"             { yylval.line = yylineno; TOKEN(TOKEN_PROGRAM_END); }
"reclutar"             { yylval.line = yylineno; TOKEN(TOKEN_RECRUIT); }

"recurso"              { yylval.type.value = TYPE_RECURSO; yylval.type.line = yylineno; TOKEN(TOKEN_TYPE); }
"epoca"                { yylval.type.value = TYPE_EPOCA;   yylval.type.line = yylineno; TOKEN(TOKEN_TYPE); }
"moral"                { yylval.type.value = TYPE_MORAL;   yylval.type.line = yylineno; TOKEN(TOKEN_TYPE); }
"heroe"                { yylval.type.value = TYPE_HEROE;   yylval.type.line = yylineno; TOKEN(TOKEN_TYPE); }
"aliado"               { yylval.type.value = TYPE_ALIADO;  yylval.type.line = yylineno; TOKEN(TOKEN_TYPE); }

"paz"                  { yylval.boolean.value = 1; yylval.boolean.line = yylineno; TOKEN(TOKEN_BOOL_LITERAL); }
"guerra"               { yylval.boolean.value = 0; yylval.boolean.line = yylineno; TOKEN(TOKEN_BOOL_LITERAL); }

"<-"                   { yylval.line = yylineno; TOKEN(TOKEN_ARROW); }
";"                    { TOKEN(';'); }
"="                    { TOKEN('='); }
","                    { TOKEN(','); }

\"[^"\n]*\"            {
                         /* Strip quotes */
                         yytext[yyleng - 1] = '\0';
                         yylval.str.text = string_copy(yytext + 1);
                         yylval.str.line = yylineno;
                         yytext[yyleng - 1] = '"';
                         TOKEN(TOKEN_STRING_LITERAL);
                       }
\"[^"\n]*              {
                         lexical_error("el texto no tiene comillas de cierre:");
                         yylval.str.text = string_copy(yytext + 1);
                         yylval.str.line = yylineno;
                         TOKEN(TOKEN_STRING_LITERAL);
                       }

[0-9]+\.[0-9]+         { yylval.dec.value = atof(yytext); yylval.dec.line = yylineno; TOKEN(TOKEN_FLOAT_LITERAL); }
[0-9]+                 { yylval.num.value = atol(yytext); yylval.num.line = yylineno; TOKEN(TOKEN_INT_LITERAL); }

[0-9]+[a-zA-Z_][a-zA-Z0-9_]*  {
                         lexical_error("un nombre no puede empezar con numero:");
                         yylval.str.text = string_copy(yytext);
                         yylval.str.line = yylineno;
                         TOKEN(TOKEN_IDENTIFIER);
                       }
[a-zA-Z_][a-zA-Z0-9_]* { yylval.str.text = string_copy(yytext); yylval.str.line = yylineno; TOKEN(TOKEN_IDENTIFIER); }

[a-zA-Z0-9_\x80-\xff]*[\x80-\xff][a-zA-Z0-9_\x80-\xff]*  {
                         lexical_error("los nombres no llevan tildes ni ene:");
                         yylval.str.text = string_copy(yytext);
                         yylval.str.line = yylineno;
                         TOKEN(TOKEN_IDENTIFIER);
                       }

.                      { lexical_error("simbolo no reconocido:"); }

%%
