%{
  #include "parser.hpp"
  #include <iostream>

  using namespace std;
%}

%option noyywrap
%option header-file="lexer.hpp"
%option outfile="lexer.cpp"

%%

\.global          return GLOBAL;
\.extern          return EXTERN;
\.section         return SECTION;
\.word            return WORD;
\.skip            return SKIP;
\.end             return END;

halt              return HALT;
int               return INT;
iret              return IRET;
call              return CALL;
ret               return RET;
jmp               return JMP;
beq               return BEQ;
bne               return BNE;
bgt               return BGT;
push              return PUSH;
pop               return POP;
xchg              return XCHG;
add               return ADD;
sub               return SUB;
mul               return MUL;
div               return DIV;
not               return NOT;
and               return AND;
or                return OR;
xor               return XOR;
shl               return SHL;
shr               return SHR;
ld                return LD;
st                return ST;
csrrd             return CSRRD;
csrwr             return CSRWR;

%r0               yylval.ival = 0; return GPR;
%r1               yylval.ival = 1; return GPR;
%r2               yylval.ival = 2; return GPR;
%r3               yylval.ival = 3; return GPR;
%r4               yylval.ival = 4; return GPR;
%r5               yylval.ival = 5; return GPR;
%r6               yylval.ival = 6; return GPR;
%r7               yylval.ival = 7; return GPR;
%r8               yylval.ival = 8; return GPR;
%r9               yylval.ival = 9; return GPR;
%r10              yylval.ival = 10; return GPR;
%r11              yylval.ival = 11; return GPR;
%r12              yylval.ival = 12; return GPR;
%r13              yylval.ival = 13; return GPR;
%r14              yylval.ival = 14; return GPR;
%r15              yylval.ival = 15; return GPR;
%sp               yylval.ival = 14; return GPR;
%pc               yylval.ival = 15; return GPR;

%status           yylval.ival = 0; return CSR;
%handler          yylval.ival = 1; return CSR;
%cause            yylval.ival = 2; return CSR;

,                 return COMMA;
:                 return COLON;
\$                return DOLLAR;
\[                return O_BRACKET;
\]                return C_BRACKET;
\+                return PLUS;

[ \t]+        
\n                yylineno++; return ENDL;
#.*   

[a-zA-Z_][a-zA-Z0-9_]*       yylval.sval = strdup(yytext); return SYMBOL;
(0[Xx])?[0-9A-Fa-f]+         yylval.ival = strtol(yytext, NULL, 0); return LITERAL;

.                 // greska?!

%%