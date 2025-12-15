#include "../inc/linker.hpp"
#include "../common/exceptions.hpp"
#include "../common/objfile.hpp"
#include "../common/exefile.hpp"
#include "../common/exetxt.hpp"


int main(int argc, char* argv[]) {
  string exeFile = "";
  vector<string> objFiles;
  unordered_map<string, int> fixedAddressSections;
  int lastFixedAddress = 0;
  string lastFixedSection = "";
  bool hex = false, obj = false;
  try {
    for (int i = 1; i < argc; i++) {
      if(string(argv[i]) == "-hex") hex = true;
      else if (string(argv[i]) == "-o") {
        if (i != argc - 1 && !obj) {
          exeFile = "tests/" + string(argv[++i]);
          obj = true;
        }
        else throw InvalidCommand();
      }
      else if (string(argv[i]).compare(0, string("-place=").size(), "-place=") == 0) {
        string place = string(argv[i]).substr(string("-place=").size());
        int pos = place.find('@');
        if (pos == string::npos || pos == 0 || pos == place.size() - 1) throw InvalidCommand();
        string section  = place.substr(0, pos);
        unsigned int address = stoul(place.substr(pos + 1), nullptr, 0);
        if (fixedAddressSections.find(section) == fixedAddressSections.end()) {
          fixedAddressSections[section] = address;
          if (address > lastFixedAddress) {
            lastFixedAddress = address;
            lastFixedSection = section;
          }
        }
        else throw MultiFixSecDef(section);
      }
      else objFiles.push_back("tests/" + string(argv[i]));
    }
    if (!hex) throw InvalidCommand();
    if (objFiles.empty()) throw InvalidInputFile("");
    if (exeFile == "") exeFile = "tests/exeFile.hex";
    
    Linker* linker = new Linker(objFiles, exeFile, fixedAddressSections, lastFixedAddress, lastFixedSection);
    linker->mergeTables();
    linker->resolveSymbols();
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

void Linker::mergeTables() {

  unordered_map<string, SectionEntry> asmSectionTable;
  unordered_map<string, SymbolEntry> asmSymbolTable;

  for (string objFile : objFiles) {
    readObjectFile(objFile, asmSectionTable, asmSymbolTable);
    unordered_map<string, int> baseOffsets = {};

    for (auto& secEntry : asmSectionTable) {
      const string& secName = secEntry.first;
      SectionEntry& section = secEntry.second;
      if (sectionTable.find(secName) == sectionTable.end()) {
        sectionTable[secName] = section;
      }
      else {
        baseOffsets[secName] = sectionTable[secName].data.size();
        for (auto& relEntry : section.rela) {
          sectionTable[secName].rela.push_back(RelocationEntry{relEntry.offset+static_cast<int>(sectionTable[secName].data.size()), relEntry.symbol, relEntry.addend});
        }
        sectionTable[secName].data.insert(sectionTable[secName].data.end(), section.data.begin(), section.data.end());
      }
    }

    for (auto& secEntry : sectionTable) {
      const string& secName = secEntry.first;
      SectionEntry& section = secEntry.second;
      for (auto& relEntry : section.rela) {
        if (relEntry.symbol[0] == '.' && baseOffsets.find(relEntry.symbol.substr(1)) != baseOffsets.end()) {
          relEntry.addend += baseOffsets[relEntry.symbol.substr(1)];
        }
      }
    }

    for (auto& symEntry : asmSymbolTable) {
      const string& symName = symEntry.first;
      SymbolEntry& symbol = symEntry.second;
      if (symbol.global) {  //section is not global
        if (symbolTable.find(symName) == symbolTable.end()) {
          symbolTable[symName] = symbol;
          if (symbol.defined) symbolTable[symName].value += baseOffsets[symbol.section];
        }
        else {
          if (symbolTable[symName].defined && symbol.defined) throw MultiGlobSymDef(symName);
          if (symbol.defined) {
            symbolTable[symName] = symbol;
            symbolTable[symName].value += baseOffsets[symbol.section];
          }
        }
      }
    }
  }

}

void Linker::resolveSymbols() {

  for (const auto& symEntry : symbolTable) {
    if (!symEntry.second.defined) throw GlobalSymobolUndefined(symEntry.first);
  }

  for (const auto& fixedSecEntry : fixedAddressSections) {
    const string& fixedSecName = fixedSecEntry.first;
    const int& FixedSecAddr = fixedSecEntry.second;
    sectionTable[fixedSecName].base = FixedSecAddr; //we can check if theres no secName sec in sectab and return err
    for (const auto& secEntry : sectionTable) {
      const string& secName = secEntry.first;
      const SectionEntry& section = secEntry.second;
      if (fixedSecName != secName && 
        ((sectionTable[fixedSecName].base >= section.base && sectionTable[fixedSecName].base < section.base + section.data.size()) ||
        (section.base >= sectionTable[fixedSecName].base && section.base < sectionTable[fixedSecName].base + sectionTable[fixedSecName].data.size()))) 
        throw FixedSectionOverlap(fixedSecName, secName);
    }
  }

  int newSectionBase = lastFixedAddress + sectionTable[lastFixedSection].data.size(); 
  for (auto& secEntry : sectionTable) {
    const string& secName = secEntry.first;
    SectionEntry& section = secEntry.second;
    if (fixedAddressSections.find(secName) == fixedAddressSections.end()) {
      section.base = newSectionBase;
      newSectionBase += section.data.size();
    }
  }

  for (auto& secEntry : sectionTable) {
    const string& secName = secEntry.first;
    SectionEntry& section = secEntry.second;
    for (auto& relEntry : section.rela) {
      int symbolValue;
      if (relEntry.symbol[0] == '.') {
        symbolValue = sectionTable[relEntry.symbol.substr(1)].base + relEntry.addend;
      }
      else {  //global syms
        symbolValue = sectionTable[symbolTable[relEntry.symbol].section].base + symbolTable[relEntry.symbol].value;
      }
      for (int i = 0; i < 4; i++) {
        section.data[relEntry.offset + i] = symbolValue >> i * 8 & 0xFF;
      }
    }
  }

  writeExecutableFile(exeFile, sectionTable);
  //writeExecutableTextFile(exeFile + ".txt", sectionTable);
  // writeExecutableText(cout, sectionTable);

}