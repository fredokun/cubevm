
%{

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "cubealloc.h"
#include "cubestr.h"
#include "cubeparse.h"
#include "cubeparse.tab.h"

char *GLOBAL_reserved_ident[] = { "new", "or", "and", "not", "true", "false", "if", "then", "else", "def", "NONE", "let", "react", "pown", "own" };
int GLOBAL_keyword[] = { TOK_NEW, TOK_OR, TOK_AND, TOK_NOT, TOK_TRUE, TOK_FALSE, TOK_IF, TOK_THEN, TOK_ELSE, TOK_DEF, TOK_NONE, TOK_LET, TOK_REACT, TOK_POWN, TOK_OWN };
int GLOBAL_nb_reserved_ident = 15;
 
%}

%option noyywrap

DIGIT  [0-9]
IDSYM1  [_a-zA-Z]
IDSYM2  {DIGIT}|{IDSYM1}
INTEGER {DIGIT}+
REAL {INTEGER}"."{INTEGER}|{INTEGER}("."{INTEGER})?[eE][+-]{INTEGER}
IDENT {IDSYM1}{IDSYM2}*
PRIM "#"{IDENT}
STRING \"[^\"]*\"
COMMENT "//".*$
INPUT {IDENT}"?"
OUTPUT {IDENT}"!"

%%

{INTEGER}  yylval.integer = int_from_cstr(yytext); return TOK_INT;

{REAL}     yylval.real = real_from_cstr(yytext); return TOK_REAL;

{STRING}   { // strings are cached in a string pool
             int size = strlen(yytext)-2; // remove the guillemets
             char * str = (char *) MEM_ALLOC_ARRAY(sizeof(char),size+1,"char","cubeparse.l","{STRING}");
             if(str==NULL)
               lexer_error("Cannot allocate memory for string");
             strncpy(str,yytext+1,size);
             str[size]=(char)0;
             yylval.integer = str_make(&GLOBAL_string_pool,str,size,-1); // permanent string
             return TOK_STR;
           }

{IDENT}    {
             int idref = GLOBAL_get_keyword(yytext);
             if(idref>=0) { // reserved keyword
               return GLOBAL_keyword[idref];
             }
             // or it's a correct identifier
             char * str = (char *) MEM_ALLOC_ARRAY(sizeof(char),strlen(yytext)+1,"char","cubeparse.l","{IDENT}");
             if(str==NULL)
               lexer_error("Cannot allocate memory for identifier");
             strncpy(str,yytext,strlen(yytext)+1);
	     yylval.string = str;
	     return TOK_ID;
           }

{INPUT}    {
             int size = strlen(yytext)-1;
             char * str = (char *) MEM_ALLOC_ARRAY(sizeof(char),size+1,"char","cubeparse.l","{INPUT}");
             if(str==NULL)
               lexer_error("Cannot allocate memory for input channel string");
             strncpy(str,yytext,size);
             str[size]=(char)0;
	     yylval.string = str;
	     return TOK_IN;
           }

{OUTPUT}    {
             int size = strlen(yytext)-1;
             char * str = (char *) MEM_ALLOC_ARRAY(sizeof(char),size+1,"char","cubeparse.l","{OUTPUT}");
             if(str==NULL)
               lexer_error("Cannot allocate memory for output channel string");
             strncpy(str,yytext,size);
             str[size]=(char)0;
	     yylval.string = str;
	     return TOK_OUT;
           }

{PRIM}     {  
             int size = strlen(yytext);
             char * str = (char *) MEM_ALLOC_ARRAY(sizeof(char),size+1,"char","cubeparse.l","{PRIM}");
             if(str==NULL)
               lexer_error("Cannot allocate memory for primitive string");
             strncpy(str,yytext,size);
             str[size]=(char)0;
	     yylval.string = str;
             return TOK_PRIM;
           }
  
{COMMENT} // do nothing

"/*"        {
            register int c;

            for ( ; ; )
                {
                while ( (c = input()) != '*' &&
                        c != EOF )
                    ;    /* eat up text of comment */

                if ( c == '*' )
                    {
                    while ( (c = input()) == '*' )
                        ;
                    if ( c == '/' )
                        break;    /* found the end */
                    }

                if ( c == EOF )
                    {
                    lexer_error( "EOF in comment" );
                    break;
                    }
                }
            }

"||"   return TOK_PAR;

"++"   return TOK_SUM;

"<>" return TOK_DIFF;

">="   return TOK_GTE;

">" return TOK_GT;

"<="   return TOK_LWE;

"<"  return TOK_LW;


"+"|"-"|"*"|"-"|"("|")"|"["|"]"|","|"="|";"|"{"|"}"|"%"|"/"  return yytext[0]; 

[ \t]+  /* skip */

"\n"  GLOBAL_line_no++;

.    lexer_error("Unexpected token '%s'", yytext);

%%

int GLOBAL_get_keyword(char *ident) {
  int i;
  for(i=0;i<GLOBAL_nb_reserved_ident;i++) {
    if(strcmp(GLOBAL_reserved_ident[i],ident)==0)
      return i;
  }

  return -1;
}

void lexer_error(char * message, ...) {
  va_list args;

  fprintf(stderr,"Lexer error at line %d \n ==> ",
	  GLOBAL_line_no);
  va_start(args,message);
  vfprintf(stderr, message, args);
  va_end(args);
  fprintf(stderr, "\n");
  exit(EXIT_FAILURE);
}


