#pragma once

#include <string>
#include <vector>
#include <unordered_map>

using namespace std;


struct SymbolEntry {
  string  name;  //redundant
  int     value = -1;
  bool    global = false;  //always true in linker symtab
  bool    defined = false; //redundant? (section==0 <=> defined==false)
  string  section = "UND";  //string - section name ?! / alternative int - section id
};
struct RelocationEntry {  //mozemo da dodamo polje za ime simbola
  int     offset;
  string  symbol; //section
  int     addend = 0;
};
struct SectionEntry {
  string        name;
  unsigned int  base = 0;
  vector<RelocationEntry> rela;
  vector<unsigned char>   data;
};