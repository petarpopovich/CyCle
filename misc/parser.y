%{
  #include "../inc/assembler.hpp"
  #include <cstdio>
  #include <iostream>

  using namespace std;

  int yylex();
  void yyerror(const char *s);
  extern int yylineno;

  extern Assembler* assembler;
%}

%defines "parser.hpp"
%output  "parser.cpp"

%union {
  int ival;
  char* sval;
}

%token <ival> LITERAL
%token <sval> SYMBOL
%token <ival> GPR
%token <ival> CSR

%token GLOBAL EXTERN SECTION WORD SKIP END
%token HALT INT IRET CALL RET JMP BEQ BNE BGT PUSH POP XCHG
%token ADD SUB MUL DIV NOT AND OR XOR SHL SHR LD ST CSRRD CSRWR

%token COMMA COLON DOLLAR O_BRACKET C_BRACKET PLUS
%token ENDL

%%

program:
  | program line
  ;

line:
    label directive ENDL
  | label instruction ENDL
  | label ENDL
  | directive ENDL
  | instruction ENDL
  | ENDL
  ;

label:
    SYMBOL COLON                                { assembler->label($1); }
  ;

directive:
    GLOBAL global_symbol_list
  | EXTERN extern_symbol_list
  | SECTION SYMBOL                              { assembler->sectionDirective($2); }
  | WORD word_initializer_list
  | SKIP LITERAL                                { assembler->skipDirective($2); }
  | END                                         { assembler->endDirective(); }
  ;

instruction:
    HALT                                        { assembler->haltInstruction(); }
  | INT                                         { assembler->intInstruction(); }
  | IRET                                        { assembler->iretInstruction(); }
  | CALL LITERAL                                { assembler->callInstruction($2); }
  | CALL SYMBOL                                 { assembler->callInstruction($2); }
  | RET                                         { assembler->retInstruction(); }
  | JMP LITERAL                                 { assembler->jmpInstruction($2); }
  | JMP SYMBOL                                  { assembler->jmpInstruction($2); }
  | BEQ GPR COMMA GPR COMMA LITERAL             { assembler->beqInstruction($2, $4, $6); }
  | BEQ GPR COMMA GPR COMMA SYMBOL              { assembler->beqInstruction($2, $4, $6); }
  | BNE GPR COMMA GPR COMMA LITERAL             { assembler->bneInstruction($2, $4, $6); }
  | BNE GPR COMMA GPR COMMA SYMBOL              { assembler->bneInstruction($2, $4, $6); }
  | BGT GPR COMMA GPR COMMA LITERAL             { assembler->bgtInstruction($2, $4, $6); }
  | BGT GPR COMMA GPR COMMA SYMBOL              { assembler->bgtInstruction($2, $4, $6); }
  | PUSH GPR                                    { assembler->pushInstruction($2); }
  | POP GPR                                     { assembler->popInstruction($2); }
  | XCHG GPR COMMA GPR                          { assembler->xchgInstruction($2, $4); }
  | ADD GPR COMMA GPR                           { assembler->addInstruction($2, $4); }
  | SUB GPR COMMA GPR                           { assembler->subInstruction($2, $4); }
  | MUL GPR COMMA GPR                           { assembler->mulInstruction($2, $4); }
  | DIV GPR COMMA GPR                           { assembler->divInstruction($2, $4); }
  | NOT GPR                                     { assembler->notInstruction($2); }
  | AND GPR COMMA GPR                           { assembler->andInstruction($2, $4); }
  | OR GPR COMMA GPR                            { assembler->orInstruction($2, $4); }
  | XOR GPR COMMA GPR                           { assembler->xorInstruction($2, $4); }
  | SHL GPR COMMA GPR                           { assembler->shlInstruction($2, $4); }
  | SHR GPR COMMA GPR                           { assembler->shrInstruction($2, $4); }
  | load
  | store
  | CSRRD CSR COMMA GPR                         { assembler->csrrdInstruction($2, $4); }
  | CSRWR GPR COMMA CSR                         { assembler->csrwrInstruction($2, $4); }
  ;

global_symbol_list:                                                           
    global_symbol_list COMMA SYMBOL             { assembler->globalDirective($3); }
  | SYMBOL                                      { assembler->globalDirective($1); }
  ;

extern_symbol_list:
    extern_symbol_list COMMA SYMBOL             { assembler->externDirective($3); }
  | SYMBOL                                      { assembler->externDirective($1); }
  ;

word_initializer_list:
    word_initializer_list COMMA SYMBOL          { assembler->wordDirective($3); }
  | word_initializer_list COMMA LITERAL         { assembler->wordDirective($3); }
  | SYMBOL                                      { assembler->wordDirective($1); }
  | LITERAL                                     { assembler->wordDirective($1); }
  ;

load:
    LD DOLLAR LITERAL COMMA GPR                              { assembler->ldImmInstruction($3, $5); }
  | LD DOLLAR SYMBOL COMMA GPR                               { assembler->ldImmInstruction($3, $5); }
  | LD LITERAL COMMA GPR                                     { assembler->ldMemDirInstruction($2, $4); }
  | LD SYMBOL COMMA GPR                                      { assembler->ldMemDirInstruction($2, $4); }
  | LD GPR COMMA GPR                                         { assembler->ldRegDirInstruction($2, $4); }
  | LD O_BRACKET GPR C_BRACKET COMMA GPR                     { assembler->ldRegIndInstruction($3, $6); }
  | LD O_BRACKET GPR PLUS LITERAL C_BRACKET COMMA GPR        { assembler->ldRegIndDispInstruction($3, $5, $8); }
  | LD O_BRACKET GPR PLUS SYMBOL C_BRACKET COMMA GPR         { assembler->ldRegIndDispInstruction($3, $5, $8); }
  ;

store:                                                       //not all options are possible
//    ST GPR COMMA DOLLAR LITERAL                              { assembler->stImmInstruction($2, $5); }
//  | ST GPR COMMA DOLLAR SYMBOL                               { assembler->stImmInstruction($2, $5); }
    ST GPR COMMA LITERAL                                     { assembler->stMemDirInstruction($2, $4); }
  | ST GPR COMMA SYMBOL                                      { assembler->stMemDirInstruction($2, $4); }
//  | ST GPR COMMA GPR                                         { assembler->stRegDirInstruction($2, $4); }
  | ST GPR COMMA O_BRACKET GPR C_BRACKET                     { assembler->stRegIndInstruction($2, $5); }
  | ST GPR COMMA O_BRACKET GPR PLUS LITERAL C_BRACKET        { assembler->stRegIndDispInstruction($2, $5, $7); }
  | ST GPR COMMA O_BRACKET GPR PLUS SYMBOL C_BRACKET         { assembler->stRegIndDispInstruction($2, $5, $7); }
  ;

%%

void yyerror(const char *s) {
    fprintf(stderr, "Error: %s in line %d\n", s, yylineno);
}