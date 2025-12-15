#include "../common/structures.hpp"
#include <string>
#include <vector>
#include <unordered_map>

using namespace std;


class Assembler {

private:
  
  string asmFile;
  string objFile;
  string currentSection;
  unordered_map<string, SectionEntry> sectionTable;
  unordered_map<string, SymbolEntry> symbolTable;

public:

  Assembler(string asmFile, string objFile) : asmFile(asmFile), objFile(objFile) {};

  void makeObjFile();
  void arrangeRelocations();

  void label(string);

  void globalDirective(string);
  void externDirective(string);
  void sectionDirective(string);
  void wordDirective(string);
  void wordDirective(int);
  void skipDirective(int);
  void endDirective();

  void haltInstruction();
  void intInstruction();
  void iretInstruction();
  void callInstruction(unsigned int);
  void callInstruction(string);
  void retInstruction();
  void jmpInstruction(unsigned int);
  void jmpInstruction(string);
  void beqInstruction(int, int, unsigned int);
  void beqInstruction(int, int, string);
  void bneInstruction(int, int, unsigned int);
  void bneInstruction(int, int, string);
  void bgtInstruction(int, int, unsigned int);
  void bgtInstruction(int, int, string);
  void pushInstruction(int);
  void popInstruction(int);
  void xchgInstruction(int, int);
  void addInstruction(int, int);
  void subInstruction(int, int);
  void mulInstruction(int, int);
  void divInstruction(int, int);
  void notInstruction(int);
  void andInstruction(int, int);
  void orInstruction(int, int);
  void xorInstruction(int, int);
  void shlInstruction(int, int);
  void shrInstruction(int, int);
  void ldImmInstruction(int, int);
  void ldImmInstruction(string, int);
  void ldMemDirInstruction(unsigned int, int);
  void ldMemDirInstruction(string, int);
  void ldRegDirInstruction(int, int);
  void ldRegIndInstruction(int, int);
  void ldRegIndDispInstruction(int, int, int);
  void ldRegIndDispInstruction(int, string, int);  //cant use symbol as displacement
  void stImmInstruction(int, int);      //doesnt exist
  void stImmInstruction(int, string);   //doesnt exist
  void stMemDirInstruction(int, unsigned int);
  void stMemDirInstruction(int, string);
  void stRegDirInstruction(int, int);   //doesnt exist
  void stRegIndInstruction(int, int);
  void stRegIndDispInstruction(int, int, int);
  void stRegIndDispInstruction(int, int, string); //cant use symbol as displacement
  void csrrdInstruction(int, int);
  void csrwrInstruction(int, int);
  
};