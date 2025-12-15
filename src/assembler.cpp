#include "../inc/assembler.hpp"
#include "../common/exceptions.hpp"
#include "../common/objfile.hpp"
#include "../common/objtxt.hpp"

extern FILE *yyin;
extern int yyparse();

Assembler* assembler;


int main(int argc, char* argv[]) {
  string asmFile, objFile;
  try {
    if (argc != 2 && argc != 4 || argc == 4 && string(argv[1]) != "-o") {
      throw InvalidCommand();
    }
    asmFile = "tests/" + string(argv[argc-1]);
    if (argc == 4) objFile = "tests/" + string(argv[2]);
    else objFile = "tests/objFile.o";
    
    assembler = new Assembler(asmFile, objFile);
    yyin = fopen(asmFile.c_str(), "r");
    if (!yyin) throw InvalidInputFile(asmFile);
    yyparse();
    assembler->makeObjFile();
  }
  catch (const runtime_error& e) {
    cerr << "Error: " << e.what() << endl;
    return -2;
  }
  catch (const exception& e) {
    return -1;
  }
  return 0;
}

void Assembler::makeObjFile() {
  writeObjectFile(objFile, sectionTable, symbolTable);
  //writeObjectTextFile(objFile + ".txt", sectionTable, symbolTable);
  // writeObjectText(cout, sectionTable, symbolTable);
}

void Assembler::arrangeRelocations() {
  for (auto& secEntry : sectionTable) {
    const string& secName = secEntry.first;
    SectionEntry& section = secEntry.second;
    for (auto& relEntry : section.rela) {
      const SymbolEntry& symbol = symbolTable[relEntry.symbol];
      if (!symbol.global) {
        if (!symbol.defined) throw LocalSymobolUndefined(symbol.name);
        relEntry.symbol = "." + symbol.section;
        relEntry.addend = symbol.value;
      }
    }
  }
}

void Assembler::label(string s) {
  if (symbolTable.find(s) == symbolTable.end()) {
    symbolTable[s] = SymbolEntry{s, static_cast<int>(sectionTable[currentSection].data.size()), false, true, currentSection};
    return;
  }
  if (symbolTable[s].defined) {
    throw MultiLocSymDef(s);
  }
  symbolTable[s].value = sectionTable[currentSection].data.size();
  symbolTable[s].defined = true;
  symbolTable[s].section = currentSection;
}

void Assembler::globalDirective(string s) {
  if (symbolTable.find(s) == symbolTable.end()) {
    symbolTable[s] = SymbolEntry{s, -1, true, false, "UND"};
  }
  else {
    symbolTable[s].global = true;
  }
}

void Assembler::externDirective(string s) {
  if (symbolTable.find(s) == symbolTable.end()) {
    symbolTable[s] = SymbolEntry{s, -1, true, false, "UND"};
  }
  else {
    symbolTable[s].global = true;
  }
}

void Assembler::sectionDirective(string s) {
  if (sectionTable.find(s) == sectionTable.end()) {
    sectionTable[s] = SectionEntry{s};
    symbolTable["." + s] = SymbolEntry{s, 0, false, true, s};
  }
  currentSection = s;
}

void Assembler::wordDirective(string s) {
  if (symbolTable.find(s) == symbolTable.end()) {
    symbolTable[s] = SymbolEntry{s};
  }
  sectionTable[currentSection].rela.push_back(RelocationEntry{static_cast<int>(sectionTable[currentSection].data.size()), s});
  for (int i = 0; i < 4; i++) {
    sectionTable[currentSection].data.push_back(0x00);
  }
}

void Assembler::wordDirective(int n) {
  for (int i = 0; i < 4; i++) {
    sectionTable[currentSection].data.push_back(n >> i * 8 & 0xFF);
  }
}

void Assembler::skipDirective(int n) {
  for (int i = 0; i < n; i++) {
    sectionTable[currentSection].data.push_back(0x00);
  }
}

void Assembler::endDirective() {    //remove helper func?
  //final actions
  //call arrangeRelocs() ?!
  arrangeRelocations();
}

void Assembler::haltInstruction() {
  sectionTable[currentSection].data.push_back(0x00);
  sectionTable[currentSection].data.push_back(0x00);
  sectionTable[currentSection].data.push_back(0x00);
  sectionTable[currentSection].data.push_back(0x00);
}

void Assembler::intInstruction() {
  sectionTable[currentSection].data.push_back(0x00);
  sectionTable[currentSection].data.push_back(0x00);
  sectionTable[currentSection].data.push_back(0x00);
  sectionTable[currentSection].data.push_back(0x10);
}

void Assembler::iretInstruction() {
  sectionTable[currentSection].data.push_back(0x04);
  sectionTable[currentSection].data.push_back(0x00);
  sectionTable[currentSection].data.push_back(0x0E);
  sectionTable[currentSection].data.push_back(0x96);

  sectionTable[currentSection].data.push_back(0x08);
  sectionTable[currentSection].data.push_back(0x00);
  sectionTable[currentSection].data.push_back(0xFE);
  sectionTable[currentSection].data.push_back(0x93);
}

void Assembler::callInstruction(unsigned int n) {
  if (n < 0x800) {
    sectionTable[currentSection].data.push_back(n & 0xFF);
    sectionTable[currentSection].data.push_back(n >> 8 & 0x0F);
    sectionTable[currentSection].data.push_back(0x00);
    sectionTable[currentSection].data.push_back(0x20);
  }
  else {
    sectionTable[currentSection].data.push_back(0x04);
    sectionTable[currentSection].data.push_back(0x00);
    sectionTable[currentSection].data.push_back(0xF0);
    sectionTable[currentSection].data.push_back(0x21);

    sectionTable[currentSection].data.push_back(0x04);
    sectionTable[currentSection].data.push_back(0x00);
    sectionTable[currentSection].data.push_back(0xF0);
    sectionTable[currentSection].data.push_back(0x30);

    for (int i = 0; i < 4; i++) {
      sectionTable[currentSection].data.push_back(n >> i * 8 & 0xFF);
    }
  }
}

void Assembler::callInstruction(string s) {
  if (symbolTable.find(s) == symbolTable.end()) {
    symbolTable[s] = SymbolEntry{s};
  }
  else if (symbolTable[s].section == currentSection && sectionTable[currentSection].data.size() - symbolTable[s].value + 4 <= 0x800) {
    int n = sectionTable[currentSection].data.size() - symbolTable[s].value + 4;
    sectionTable[currentSection].data.push_back(-n & 0xFF);
    sectionTable[currentSection].data.push_back(-n >> 8 & 0x0F);
    sectionTable[currentSection].data.push_back(0xF0);
    sectionTable[currentSection].data.push_back(0x20);
    return;
  }
  
  sectionTable[currentSection].data.push_back(0x04);
  sectionTable[currentSection].data.push_back(0x00);
  sectionTable[currentSection].data.push_back(0xF0);
  sectionTable[currentSection].data.push_back(0x21);

  sectionTable[currentSection].data.push_back(0x04);
  sectionTable[currentSection].data.push_back(0x00);
  sectionTable[currentSection].data.push_back(0xF0);
  sectionTable[currentSection].data.push_back(0x30);

  sectionTable[currentSection].rela.push_back(RelocationEntry{static_cast<int>(sectionTable[currentSection].data.size()), s});
  for (int i = 0; i < 4; i++) {
    sectionTable[currentSection].data.push_back(0x00);
  }
}

void Assembler::retInstruction() {
  sectionTable[currentSection].data.push_back(0x04);
  sectionTable[currentSection].data.push_back(0x00);
  sectionTable[currentSection].data.push_back(0xFE);
  sectionTable[currentSection].data.push_back(0x93);
}

void Assembler::jmpInstruction(unsigned int n) {
  if (n < 0x800) {
    sectionTable[currentSection].data.push_back(n & 0xFF);
    sectionTable[currentSection].data.push_back(n >> 8 & 0x0F); //probably can remove & 0x0F
    sectionTable[currentSection].data.push_back(0x00);
    sectionTable[currentSection].data.push_back(0x30);
  }
  else {
    sectionTable[currentSection].data.push_back(0x00);
    sectionTable[currentSection].data.push_back(0x00);
    sectionTable[currentSection].data.push_back(0xF0);
    sectionTable[currentSection].data.push_back(0x38);

    for (int i = 0; i < 4; i++) {
      sectionTable[currentSection].data.push_back(n >> i * 8 & 0xFF);
    }
  }
}

void Assembler::jmpInstruction(string s) {
  if (symbolTable.find(s) == symbolTable.end()) {
    symbolTable[s] = SymbolEntry{s};
  }
  else if (symbolTable[s].section == currentSection && sectionTable[currentSection].data.size() - symbolTable[s].value + 4 <= 0x800) {
    int n = sectionTable[currentSection].data.size() - symbolTable[s].value + 4;
    sectionTable[currentSection].data.push_back(-n & 0xFF);
    sectionTable[currentSection].data.push_back(-n >> 8 & 0x0F);
    sectionTable[currentSection].data.push_back(0xF0);
    sectionTable[currentSection].data.push_back(0x30);
    return;
  }
  sectionTable[currentSection].data.push_back(0x00);
  sectionTable[currentSection].data.push_back(0x00);
  sectionTable[currentSection].data.push_back(0xF0);
  sectionTable[currentSection].data.push_back(0x38);

  sectionTable[currentSection].rela.push_back(RelocationEntry{static_cast<int>(sectionTable[currentSection].data.size()), s});
  for (int i = 0; i < 4; i++) {
    sectionTable[currentSection].data.push_back(0x00);
  }
}

void Assembler::beqInstruction(int r1, int r2, unsigned int n) {
  if (n < 0x800) {
    sectionTable[currentSection].data.push_back(n & 0xFF);
    sectionTable[currentSection].data.push_back(n >> 8 & 0x0F | r2 << 4);
    sectionTable[currentSection].data.push_back(r1 & 0x0F);
    sectionTable[currentSection].data.push_back(0x31);
  }
  else {
    sectionTable[currentSection].data.push_back(0x04);
    sectionTable[currentSection].data.push_back(r2 << 4);
    sectionTable[currentSection].data.push_back(r1 & 0x0F | 0xF0);
    sectionTable[currentSection].data.push_back(0x39);

    sectionTable[currentSection].data.push_back(0x04);
    sectionTable[currentSection].data.push_back(0x00);
    sectionTable[currentSection].data.push_back(0xF0);
    sectionTable[currentSection].data.push_back(0x30);

    for (int i = 0; i < 4; i++) {
      sectionTable[currentSection].data.push_back(n >> i * 8 & 0xFF);
    }
  }
}

void Assembler::beqInstruction(int r1, int r2, string s) {
  if (symbolTable.find(s) == symbolTable.end()) {
    symbolTable[s] = SymbolEntry{s};
  }
  else if (symbolTable[s].section == currentSection && sectionTable[currentSection].data.size() - symbolTable[s].value + 4 <= 0x800) {
    int n = sectionTable[currentSection].data.size() - symbolTable[s].value + 4;
    sectionTable[currentSection].data.push_back(-n & 0xFF);
    sectionTable[currentSection].data.push_back(-n >> 8 & 0x0F | r2 << 4);
    sectionTable[currentSection].data.push_back(r1 & 0x0F | 0xF0);
    sectionTable[currentSection].data.push_back(0x31);
    return;
  }
  sectionTable[currentSection].data.push_back(0x04);
  sectionTable[currentSection].data.push_back(r2 << 4);
  sectionTable[currentSection].data.push_back(r1 & 0x0F | 0xF0);
  sectionTable[currentSection].data.push_back(0x39);

  sectionTable[currentSection].data.push_back(0x04);
  sectionTable[currentSection].data.push_back(0x00);
  sectionTable[currentSection].data.push_back(0xF0);
  sectionTable[currentSection].data.push_back(0x30);

  sectionTable[currentSection].rela.push_back(RelocationEntry{static_cast<int>(sectionTable[currentSection].data.size()), s});
  for (int i = 0; i < 4; i++) {
    sectionTable[currentSection].data.push_back(0x00);
  }
}

void Assembler::bneInstruction(int r1, int r2, unsigned int n) {
  if (n < 0x800) {
    sectionTable[currentSection].data.push_back(n & 0xFF);
    sectionTable[currentSection].data.push_back(n >> 8 & 0x0F | r2 << 4);
    sectionTable[currentSection].data.push_back(r1 & 0x0F);
    sectionTable[currentSection].data.push_back(0x32);
  }
  else {
    sectionTable[currentSection].data.push_back(0x04);
    sectionTable[currentSection].data.push_back(r2 << 4);
    sectionTable[currentSection].data.push_back(r1 & 0x0F | 0xF0);
    sectionTable[currentSection].data.push_back(0x3A);

    sectionTable[currentSection].data.push_back(0x04);
    sectionTable[currentSection].data.push_back(0x00);
    sectionTable[currentSection].data.push_back(0xF0);
    sectionTable[currentSection].data.push_back(0x30);

    for (int i = 0; i < 4; i++) {
      sectionTable[currentSection].data.push_back(n >> i * 8 & 0xFF);
    }
  }
}

void Assembler::bneInstruction(int r1, int r2, string s) {
  if (symbolTable.find(s) == symbolTable.end()) {
    symbolTable[s] = SymbolEntry{s};
  }
  else if (symbolTable[s].section == currentSection && sectionTable[currentSection].data.size() - symbolTable[s].value + 4 <= 0x800) {
    int n = sectionTable[currentSection].data.size() - symbolTable[s].value + 4;
    sectionTable[currentSection].data.push_back(-n & 0xFF);
    sectionTable[currentSection].data.push_back(-n >> 8 & 0x0F | r2 << 4);
    sectionTable[currentSection].data.push_back(r1 & 0x0F | 0xF0);
    sectionTable[currentSection].data.push_back(0x32);
    return;
  }
  sectionTable[currentSection].data.push_back(0x04);
  sectionTable[currentSection].data.push_back(r2 << 4);
  sectionTable[currentSection].data.push_back(r1 & 0x0F | 0xF0);
  sectionTable[currentSection].data.push_back(0x3A);

  sectionTable[currentSection].data.push_back(0x04);
  sectionTable[currentSection].data.push_back(0x00);
  sectionTable[currentSection].data.push_back(0xF0);
  sectionTable[currentSection].data.push_back(0x30);

  sectionTable[currentSection].rela.push_back(RelocationEntry{static_cast<int>(sectionTable[currentSection].data.size()), s});
  for (int i = 0; i < 4; i++) {
    sectionTable[currentSection].data.push_back(0x00);
  }
}

void Assembler::bgtInstruction(int r1, int r2, unsigned int n) {
  if (n < 0x800) {
    sectionTable[currentSection].data.push_back(n & 0xFF);
    sectionTable[currentSection].data.push_back(n >> 8 & 0x0F | r2 << 4);
    sectionTable[currentSection].data.push_back(r1 & 0x0F);
    sectionTable[currentSection].data.push_back(0x33);
  }
  else {
    sectionTable[currentSection].data.push_back(0x04);
    sectionTable[currentSection].data.push_back(r2 << 4);
    sectionTable[currentSection].data.push_back(r1 & 0x0F | 0xF0);
    sectionTable[currentSection].data.push_back(0x3B);

    sectionTable[currentSection].data.push_back(0x04);
    sectionTable[currentSection].data.push_back(0x00);
    sectionTable[currentSection].data.push_back(0xF0);
    sectionTable[currentSection].data.push_back(0x30);

    for (int i = 0; i < 4; i++) {
      sectionTable[currentSection].data.push_back(n >> i * 8 & 0xFF);
    }
  }
}

void Assembler::bgtInstruction(int r1, int r2, string s) {
  if (symbolTable.find(s) == symbolTable.end()) {
    symbolTable[s] = SymbolEntry{s};
  }
  else if (symbolTable[s].section == currentSection && sectionTable[currentSection].data.size() - symbolTable[s].value + 4 <= 0x800) {
    int n = sectionTable[currentSection].data.size() - symbolTable[s].value + 4;
    sectionTable[currentSection].data.push_back(-n & 0xFF);
    sectionTable[currentSection].data.push_back(-n >> 8 & 0x0F | r2 << 4);
    sectionTable[currentSection].data.push_back(r1 & 0x0F | 0xF0);
    sectionTable[currentSection].data.push_back(0x33);
    return;
  }
  sectionTable[currentSection].data.push_back(0x04);
  sectionTable[currentSection].data.push_back(r2 << 4);
  sectionTable[currentSection].data.push_back(r1 & 0x0F | 0xF0);
  sectionTable[currentSection].data.push_back(0x3B);

  sectionTable[currentSection].data.push_back(0x04);
  sectionTable[currentSection].data.push_back(0x00);
  sectionTable[currentSection].data.push_back(0xF0);
  sectionTable[currentSection].data.push_back(0x30);

  sectionTable[currentSection].rela.push_back(RelocationEntry{static_cast<int>(sectionTable[currentSection].data.size()), s});
  for (int i = 0; i < 4; i++) {
    sectionTable[currentSection].data.push_back(0x00);
  }
}

void Assembler::pushInstruction(int r1) {
  sectionTable[currentSection].data.push_back(0xFC);
  sectionTable[currentSection].data.push_back(0x0F | r1 << 4);
  sectionTable[currentSection].data.push_back(0xE0);
  sectionTable[currentSection].data.push_back(0x81);
}

void Assembler::popInstruction(int r1) {
  sectionTable[currentSection].data.push_back(0x04);
  sectionTable[currentSection].data.push_back(0x00);
  sectionTable[currentSection].data.push_back(0x0E | r1 << 4);
  sectionTable[currentSection].data.push_back(0x93);
}

void Assembler::xchgInstruction(int r1, int r2) {
  sectionTable[currentSection].data.push_back(0x00);
  sectionTable[currentSection].data.push_back(r1 << 4);
  sectionTable[currentSection].data.push_back(r2);
  sectionTable[currentSection].data.push_back(0x40);
}

void Assembler::addInstruction(int r1, int r2) {
  sectionTable[currentSection].data.push_back(0x00);
  sectionTable[currentSection].data.push_back(r1 << 4);
  sectionTable[currentSection].data.push_back(r2 | r2 << 4);
  sectionTable[currentSection].data.push_back(0x50);
}

void Assembler::subInstruction(int r1, int r2) {
  sectionTable[currentSection].data.push_back(0x00);
  sectionTable[currentSection].data.push_back(r1 << 4);
  sectionTable[currentSection].data.push_back(r2 | r2 << 4);
  sectionTable[currentSection].data.push_back(0x51);
}

void Assembler::mulInstruction(int r1, int r2) {
  sectionTable[currentSection].data.push_back(0x00);
  sectionTable[currentSection].data.push_back(r1 << 4);
  sectionTable[currentSection].data.push_back(r2 | r2 << 4);
  sectionTable[currentSection].data.push_back(0x52);
}

void Assembler::divInstruction(int r1, int r2) {
  sectionTable[currentSection].data.push_back(0x00);
  sectionTable[currentSection].data.push_back(r1 << 4);
  sectionTable[currentSection].data.push_back(r2 | r2 << 4);
  sectionTable[currentSection].data.push_back(0x53);
}

void Assembler::notInstruction(int r1) {
  sectionTable[currentSection].data.push_back(0x00);
  sectionTable[currentSection].data.push_back(0x00);
  sectionTable[currentSection].data.push_back(r1 | r1 << 4);
  sectionTable[currentSection].data.push_back(0x60);
}

void Assembler::andInstruction(int r1, int r2) {
  sectionTable[currentSection].data.push_back(0x00);
  sectionTable[currentSection].data.push_back(r1 << 4);
  sectionTable[currentSection].data.push_back(r2 | r2 << 4);
  sectionTable[currentSection].data.push_back(0x61);
}

void Assembler::orInstruction(int r1, int r2) {
  sectionTable[currentSection].data.push_back(0x00);
  sectionTable[currentSection].data.push_back(r1 << 4);
  sectionTable[currentSection].data.push_back(r2 | r2 << 4);
  sectionTable[currentSection].data.push_back(0x62);
}

void Assembler::xorInstruction(int r1, int r2) {
  sectionTable[currentSection].data.push_back(0x00);
  sectionTable[currentSection].data.push_back(r1 << 4);
  sectionTable[currentSection].data.push_back(r2 | r2 << 4);
  sectionTable[currentSection].data.push_back(0x63);
}

void Assembler::shlInstruction(int r1, int r2) {
  sectionTable[currentSection].data.push_back(0x00);
  sectionTable[currentSection].data.push_back(r1 << 4);
  sectionTable[currentSection].data.push_back(r2 | r2 << 4);
  sectionTable[currentSection].data.push_back(0x70);
}

void Assembler::shrInstruction(int r1, int r2) {
  sectionTable[currentSection].data.push_back(0x00);
  sectionTable[currentSection].data.push_back(r1 << 4);
  sectionTable[currentSection].data.push_back(r2 | r2 << 4);
  sectionTable[currentSection].data.push_back(0x71);
}

void Assembler::ldImmInstruction(int n, int r1) {
  if (n >= -0x800 && n < 0x800) {
    sectionTable[currentSection].data.push_back(n & 0xFF);
    sectionTable[currentSection].data.push_back(n >> 8 & 0x0F);
    sectionTable[currentSection].data.push_back(r1 << 4);
    sectionTable[currentSection].data.push_back(0x91);
  }
  else {
    // sectionTable[currentSection].data.push_back(0x04);
    // sectionTable[currentSection].data.push_back(0x00);
    // sectionTable[currentSection].data.push_back(0x0F | r1 << 4);
    // sectionTable[currentSection].data.push_back(0x92);

    // sectionTable[currentSection].data.push_back(0x04);
    // sectionTable[currentSection].data.push_back(0x00);
    // sectionTable[currentSection].data.push_back(0xF0);
    // sectionTable[currentSection].data.push_back(0x30);

    sectionTable[currentSection].data.push_back(0x04);
    sectionTable[currentSection].data.push_back(0x00);
    sectionTable[currentSection].data.push_back(0x0F | r1 << 4);
    sectionTable[currentSection].data.push_back(0x93);

    for (int i = 0; i < 4; i++) {
      sectionTable[currentSection].data.push_back(n >> i * 8 & 0xFF);
    }
  }
}

void Assembler::ldImmInstruction(string s, int r1) {
  if (symbolTable.find(s) == symbolTable.end()) {
    symbolTable[s] = SymbolEntry{s};
  }
  else if (symbolTable[s].section == currentSection && sectionTable[currentSection].data.size() - symbolTable[s].value + 4 <= 0x800) {
    int n = sectionTable[currentSection].data.size() - symbolTable[s].value + 4;
    sectionTable[currentSection].data.push_back(-n & 0xFF);
    sectionTable[currentSection].data.push_back(-n >> 8 & 0x0F);
    sectionTable[currentSection].data.push_back(0x0F | r1 << 4);
    sectionTable[currentSection].data.push_back(0x92);
    return;
  }
  // sectionTable[currentSection].data.push_back(0x04);
  // sectionTable[currentSection].data.push_back(0x00);
  // sectionTable[currentSection].data.push_back(0x0F | r1 << 4);
  // sectionTable[currentSection].data.push_back(0x92);

  // sectionTable[currentSection].data.push_back(0x04);
  // sectionTable[currentSection].data.push_back(0x00);
  // sectionTable[currentSection].data.push_back(0xF0);
  // sectionTable[currentSection].data.push_back(0x30);

  sectionTable[currentSection].data.push_back(0x04);
  sectionTable[currentSection].data.push_back(0x00);
  sectionTable[currentSection].data.push_back(0x0F | r1 << 4);
  sectionTable[currentSection].data.push_back(0x93);

  sectionTable[currentSection].rela.push_back(RelocationEntry{static_cast<int>(sectionTable[currentSection].data.size()), s});
  for (int i = 0; i < 4; i++) {
    sectionTable[currentSection].data.push_back(0x00);
  }
}

void Assembler::ldMemDirInstruction(unsigned int n, int r1) {
  if (n < 0x800) {
    sectionTable[currentSection].data.push_back(n & 0xFF);
    sectionTable[currentSection].data.push_back(n >> 8 & 0x0F);
    sectionTable[currentSection].data.push_back(r1 << 4);
    sectionTable[currentSection].data.push_back(0x92);
  }
  else {
    // sectionTable[currentSection].data.push_back(0x04);
    // sectionTable[currentSection].data.push_back(0x00);
    // sectionTable[currentSection].data.push_back(0x0F | r1 << 4);
    // sectionTable[currentSection].data.push_back(0x92);

    // sectionTable[currentSection].data.push_back(0x04);
    // sectionTable[currentSection].data.push_back(0x00);
    // sectionTable[currentSection].data.push_back(0xF0);
    // sectionTable[currentSection].data.push_back(0x30);

    sectionTable[currentSection].data.push_back(0x04);
    sectionTable[currentSection].data.push_back(0x00);
    sectionTable[currentSection].data.push_back(0x0F | r1 << 4);
    sectionTable[currentSection].data.push_back(0x93);

    for (int i = 0; i < 4; i++) {
      sectionTable[currentSection].data.push_back(n >> i * 8 & 0xFF);
    }

    sectionTable[currentSection].data.push_back(0x00);
    sectionTable[currentSection].data.push_back(0x00);
    sectionTable[currentSection].data.push_back(r1 | r1 << 4);
    sectionTable[currentSection].data.push_back(0x92);
  }
}

void Assembler::ldMemDirInstruction(string s, int r1) {
  if (symbolTable.find(s) == symbolTable.end()) {
    symbolTable[s] = SymbolEntry{s};
  }
  else if (symbolTable[s].section == currentSection && sectionTable[currentSection].data.size() - symbolTable[s].value + 4 <= 0x800) {
    int n = sectionTable[currentSection].data.size() - symbolTable[s].value + 4;
    sectionTable[currentSection].data.push_back(-n & 0xFF);
    sectionTable[currentSection].data.push_back(-n >> 8 & 0x0F);
    sectionTable[currentSection].data.push_back(0x0F | r1 << 4);
    sectionTable[currentSection].data.push_back(0x92);
    return;
  }
  // sectionTable[currentSection].data.push_back(0x04);
  // sectionTable[currentSection].data.push_back(0x00);
  // sectionTable[currentSection].data.push_back(0x0F | r1 << 4);
  // sectionTable[currentSection].data.push_back(0x92);

  // sectionTable[currentSection].data.push_back(0x04);
  // sectionTable[currentSection].data.push_back(0x00);
  // sectionTable[currentSection].data.push_back(0xF0);
  // sectionTable[currentSection].data.push_back(0x30);

  sectionTable[currentSection].data.push_back(0x04);
  sectionTable[currentSection].data.push_back(0x00);
  sectionTable[currentSection].data.push_back(0x0F | r1 << 4);
  sectionTable[currentSection].data.push_back(0x93);

  sectionTable[currentSection].rela.push_back(RelocationEntry{static_cast<int>(sectionTable[currentSection].data.size()), s});
  for (int i = 0; i < 4; i++) {
    sectionTable[currentSection].data.push_back(0x00);
  }

  sectionTable[currentSection].data.push_back(0x00);
  sectionTable[currentSection].data.push_back(0x00);
  sectionTable[currentSection].data.push_back(r1 | r1 << 4);
  sectionTable[currentSection].data.push_back(0x92);
}

void Assembler::ldRegDirInstruction(int r1, int r2) {
  sectionTable[currentSection].data.push_back(0x00);
  sectionTable[currentSection].data.push_back(0x00);
  sectionTable[currentSection].data.push_back(r1 | r2 << 4);
  sectionTable[currentSection].data.push_back(0x91);
}

void Assembler::ldRegIndInstruction(int r1, int r2) {
  sectionTable[currentSection].data.push_back(0x00);
  sectionTable[currentSection].data.push_back(0x00);
  sectionTable[currentSection].data.push_back(r1 | r2 << 4);
  sectionTable[currentSection].data.push_back(0x92);
}

void Assembler::ldRegIndDispInstruction(int r1, int n, int r2) {
  if (n >= -0x800 && n < 0x800) {
    sectionTable[currentSection].data.push_back(n & 0xFF);
    sectionTable[currentSection].data.push_back(n >> 8 & 0x0F);
    sectionTable[currentSection].data.push_back(r1 | r2 << 4);
    sectionTable[currentSection].data.push_back(0x92);
  }
  else throw TooLargeDispValue(n);
}

void Assembler::ldRegIndDispInstruction(int r1, string s, int r2) {
  throw CantResolveSymDisp(s);
}

void Assembler::stMemDirInstruction(int r1, unsigned int n) {  //store: memdir, regind, reginddisp
  if (n < 0x800) {
    sectionTable[currentSection].data.push_back(n & 0xFF);
    sectionTable[currentSection].data.push_back(n >> 8 & 0x0F | r1 << 4);
    sectionTable[currentSection].data.push_back(0x00);
    sectionTable[currentSection].data.push_back(0x80);
  }
  else {
    sectionTable[currentSection].data.push_back(0x04);
    sectionTable[currentSection].data.push_back(r1 << 4);
    sectionTable[currentSection].data.push_back(0xF0);
    sectionTable[currentSection].data.push_back(0x82);

    sectionTable[currentSection].data.push_back(0x04);
    sectionTable[currentSection].data.push_back(0x00);
    sectionTable[currentSection].data.push_back(0xF0);
    sectionTable[currentSection].data.push_back(0x30);

    for (int i = 0; i < 4; i++) {
      sectionTable[currentSection].data.push_back(n >> i * 8 & 0xFF);
    }
  }
}

void Assembler::stMemDirInstruction(int r1, string s) {
  if (symbolTable.find(s) == symbolTable.end()) {
    symbolTable[s] = SymbolEntry{s};
  }
  else if (symbolTable[s].section == currentSection && sectionTable[currentSection].data.size() - symbolTable[s].value + 4 <= 0x800) {
    int n = sectionTable[currentSection].data.size() - symbolTable[s].value + 4;
    sectionTable[currentSection].data.push_back(-n & 0xFF);
    sectionTable[currentSection].data.push_back(-n >> 8 & 0x0F | r1 << 4);
    sectionTable[currentSection].data.push_back(0xF0);
    sectionTable[currentSection].data.push_back(0x80);
    return;
  }
  sectionTable[currentSection].data.push_back(0x04);
  sectionTable[currentSection].data.push_back(r1 << 4);
  sectionTable[currentSection].data.push_back(0xF0);
  sectionTable[currentSection].data.push_back(0x82);

  sectionTable[currentSection].data.push_back(0x04);
  sectionTable[currentSection].data.push_back(0x00);
  sectionTable[currentSection].data.push_back(0xF0);
  sectionTable[currentSection].data.push_back(0x30);

  sectionTable[currentSection].rela.push_back(RelocationEntry{static_cast<int>(sectionTable[currentSection].data.size()), s});
  for (int i = 0; i < 4; i++) {
    sectionTable[currentSection].data.push_back(0x00);
  }
}

void Assembler::stRegIndInstruction(int r1, int r2) {
  sectionTable[currentSection].data.push_back(0x00);
  sectionTable[currentSection].data.push_back(r1 << 4);
  sectionTable[currentSection].data.push_back(r2 << 4);
  sectionTable[currentSection].data.push_back(0x80);
}

void Assembler::stRegIndDispInstruction(int r1, int r2, int n) {
  if (n >= -0x800 && n < 0x800) {
    sectionTable[currentSection].data.push_back(n & 0xFF);
    sectionTable[currentSection].data.push_back(n >> 8 & 0x0F | r1 << 4);
    sectionTable[currentSection].data.push_back(r2 << 4);
    sectionTable[currentSection].data.push_back(0x80);
  }
  else throw TooLargeDispValue(n);
}

void Assembler::stRegIndDispInstruction(int r1, int r2, string s) {
  throw CantResolveSymDisp(s);
}

void Assembler::csrrdInstruction(int r1, int r2) {
  sectionTable[currentSection].data.push_back(0x00);
  sectionTable[currentSection].data.push_back(0x00);
  sectionTable[currentSection].data.push_back(r1 | r2 << 4);
  sectionTable[currentSection].data.push_back(0x90);
}

void Assembler::csrwrInstruction(int r1, int r2) {
  sectionTable[currentSection].data.push_back(0x00);
  sectionTable[currentSection].data.push_back(0x00);
  sectionTable[currentSection].data.push_back(r1 | r2 << 4);
  sectionTable[currentSection].data.push_back(0x94);
}