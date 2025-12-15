#include "../common/structures.hpp"
#include <string>
#include <vector>
#include <unordered_map>

using namespace std;


class Linker {

private:

  vector<string> objFiles;
  string exeFile;
  unordered_map<string, int> fixedAddressSections;
  int lastFixedAddress;
  string lastFixedSection;
  unordered_map<string, SectionEntry> sectionTable;
  unordered_map<string, SymbolEntry> symbolTable;

public:

  Linker(vector<string> objFiles, string exeFile, unordered_map<string, int> fixedAddressSections, int lastFixedAddress, string lastFixedSection) : 
    objFiles(objFiles), exeFile(exeFile), fixedAddressSections(fixedAddressSections), lastFixedAddress(lastFixedAddress), lastFixedSection(lastFixedSection) {};

  void mergeTables();
  void resolveSymbols();

};