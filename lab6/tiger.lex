%{
/* Lab2 Attention: You are only allowed to add code in this file and start at Line 26.*/
#include <string.h>
#include "util.h"
#include "errormsg.h"
#include "symbol.h"
#include "absyn.h"
#include "y.tab.h"

int charPos=1;

int yywrap(void)
{
 charPos=1;
 return 1;
}

void adjust(void)
{
 EM_tokPos=charPos;
 charPos+=yyleng;
}

/*
* Please don't modify the lines above.
* You can add C declarations of your own below.
*/

#define ADJ adjust()

int len=0;
char* str=NULL;
int comm_cnt=0;

/* @function: generate_str
 * @input: NULL
 * @output: An empty string with a '\0'; 
 */
char *generate_str()
{
  len=0;
  str=checked_malloc(len+1);
  str[0]='\0';
  return str;
}

/* @function: add_char
 * @input: a single char
 * @output: The string value,whose tail was added the char
 */
char *add_char(char c)
{
  len++;
  str=realloc(str,len+1);
  str[len-1]=c;
  str[len]='\0';
  return str;
}

/* @function: free_char
 * @input: null
 * @output: Free the memory of str and reset 
 * the length of it.
 */
void free_str()
{
  free(str);
  str=NULL;
  len=0;
}

%}

%Start COMMENT STR

%%
  /* 
  * Below is an example, which you can wipe out
  * and write reguler expressions and actions of your own.
  */ 

<INITIAL>"\n"                        {ADJ; EM_newline();continue;}

<INITIAL>","                         {ADJ;return COMMA;}
<INITIAL>":"                         {ADJ;return COLON;}
<INITIAL>";"                         {ADJ;return SEMICOLON;}
<INITIAL>"{"                         {ADJ;return LBRACE;}
<INITIAL>"}"                         {ADJ;return RBRACE;}
<INITIAL>"("                         {ADJ;return LPAREN;}
<INITIAL>")"                         {ADJ;return RPAREN;}
<INITIAL>"["                         {ADJ;return LBRACK;}
<INITIAL>"]"                         {ADJ;return RBRACK;}
<INITIAL>"."                         {ADJ;return DOT;}
<INITIAL>"+"                         {ADJ;return PLUS;}
<INITIAL>"-"                         {ADJ;return MINUS;}
<INITIAL>"*"                         {ADJ;return TIMES;}
<INITIAL>"/"                         {ADJ;return DIVIDE;}
<INITIAL>"="                         {ADJ;return EQ;}
<INITIAL>"<>"                        {ADJ;return NEQ;}
<INITIAL>"<"                         {ADJ;return LT;}
<INITIAL>"<="                        {ADJ;return LE;}
<INITIAL>">"                         {ADJ;return GT;}
<INITIAL>">="                        {ADJ;return GE;}
<INITIAL>"&"                         {ADJ;return AND;}
<INITIAL>"|"                         {ADJ;return OR;}
<INITIAL>":="                        {ADJ;return ASSIGN;}

<INITIAL>array                       {ADJ;return ARRAY;}
<INITIAL>if                          {ADJ;return IF;}
<INITIAL>then                        {ADJ;return THEN;}
<INITIAL>else                        {ADJ;return ELSE;}
<INITIAL>while                       {ADJ;return WHILE;}
<INITIAL>for                         {ADJ;return FOR;}
<INITIAL>to                          {ADJ;return TO;}
<INITIAL>do                          {ADJ;return DO;}
<INITIAL>let                         {ADJ;return LET;}
<INITIAL>in                          {ADJ;return IN;}
<INITIAL>end                         {ADJ;return END;}
<INITIAL>of                          {ADJ;return OF;} 
<INITIAL>break                       {ADJ;return BREAK;}
<INITIAL>nil                         {ADJ;return NIL;}
<INITIAL>function                    {ADJ;return FUNCTION;}
<INITIAL>var                         {ADJ;return VAR;}
<INITIAL>type                        {ADJ;return TYPE;}

<INITIAL>[a-zA-Z][a-zA-Z0-9_]*       {ADJ;yylval.sval=String(yytext);return ID;}
<INITIAL>[0-9]+                      {ADJ;yylval.ival=atoi(yytext);return INT;}
<INITIAL>[ \t]*                      {ADJ;continue;}

<INITIAL>"/*"                        {ADJ;comm_cnt++;BEGIN COMMENT;}
<INITIAL>\"                          {ADJ;generate_str();BEGIN STR;}

<STR>\" {
  charPos+=yyleng;
  if(strlen(str) == 0)
  {
    yylval.sval="";
  }
  else
    yylval.sval=String(str);
  free_str();
  BEGIN INITIAL;
  return STRING;
  }
<STR>"\\n"                           {charPos+=yyleng;add_char('\n');continue;}
<STR>"\\t"                           {charPos+=yyleng;add_char('\t');continue;}
<STR>"\\\""                          {charPos+=yyleng;add_char('\"');continue;}
<STR>"\\\\"                          {charPos+=yyleng;add_char('\\');continue;}
<STR>\\[0-9]{3}                      {charPos+=yyleng;add_char((char)atoi(yytext+1));continue;}
<STR>\\[ \n\t\f]+\\                  {charPos+=yyleng;continue;}
<STR>"\\\^A"                         {charPos+=yyleng;add_char((char)1);continue;}
<STR>"\\\^B"                         {charPos+=yyleng;add_char((char)2);continue;}
<STR>"\\\^C"                         {charPos+=yyleng;add_char((char)3);continue;}
<STR>"\\\^D"                         {charPos+=yyleng;add_char((char)4);continue;}
<STR>"\\\^E"                         {charPos+=yyleng;add_char((char)5);continue;}
<STR>"\\\^F"                         {charPos+=yyleng;add_char((char)6);continue;}
<STR>"\\\^G"                         {charPos+=yyleng;add_char((char)7);continue;}
<STR>"\\\^H"                         {charPos+=yyleng;add_char((char)8);continue;}
<STR>"\\\^I"                         {charPos+=yyleng;add_char((char)9);continue;}
<STR>"\\\^J"                         {charPos+=yyleng;add_char((char)10);continue;}
<STR>"\\\^K"                         {charPos+=yyleng;add_char((char)11);continue;}
<STR>"\\\^L"                         {charPos+=yyleng;add_char((char)12);continue;}
<STR>"\\\^M"                         {charPos+=yyleng;add_char((char)13);continue;}
<STR>"\\\^N"                         {charPos+=yyleng;add_char((char)14);continue;}
<STR>"\\\^O"                         {charPos+=yyleng;add_char((char)15);continue;}
<STR>"\\\^P"                         {charPos+=yyleng;add_char((char)16);continue;}
<STR>"\\\^Q"                         {charPos+=yyleng;add_char((char)17);continue;}
<STR>"\\\^R"                         {charPos+=yyleng;add_char((char)18);continue;}
<STR>"\\\^S"                         {charPos+=yyleng;add_char((char)19);continue;}
<STR>"\\\^T"                         {charPos+=yyleng;add_char((char)20);continue;}
<STR>"\\\^U"                         {charPos+=yyleng;add_char((char)21);continue;}
<STR>"\\\^V"                         {charPos+=yyleng;add_char((char)22);continue;}
<STR>"\\\^W"                         {charPos+=yyleng;add_char((char)23);continue;}
<STR>"\\\^X"                         {charPos+=yyleng;add_char((char)24);continue;}
<STR>"\\\^Y"                         {charPos+=yyleng;add_char((char)25);continue;}
<STR>"\\\^Z"                         {charPos+=yyleng;add_char((char)26);continue;}


<STR>.                               {charPos+=yyleng;add_char(*yytext);continue;}

<COMMENT>"*/"  {
  ADJ;
  comm_cnt--;
  if(comm_cnt == 0)
    BEGIN INITIAL;
}
<COMMENT>"/*"                        {ADJ;comm_cnt++;continue;}
<COMMENT>"\n"                        {ADJ;EM_newline();continue;}
<COMMENT>.                           {ADJ;}
