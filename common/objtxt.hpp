#include "structures.hpp"
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <unordered_map>

using namespace std;


// -------- helpers (formatiranje) --------
static string escape_str(const string& s) {
  ostringstream os;
  for (unsigned char c : s) {
    switch (c) {
      case '\\': os << "\\\\"; break;
      case '\"': os << "\\\""; break;
      case '\n': os << "\\n";  break;
      case '\r': os << "\\r";  break;
      case '\t': os << "\\t";  break;
      default:
        if (c < 0x20 || c >= 0x7F) {
          os << "\\x" << hex << setw(2) << setfill('0') << int(c)
             << dec << setw(0) << setfill(' ');
        } else {
          os << char(c);
        }
    }
  }
  return os.str();
}

static void hex_dump(ostream& os,
                     const vector<unsigned char>& data,
                     size_t bytesPerLine = 16,
                     size_t group = 4) {
  auto oldflags = os.flags();
  auto oldfill  = os.fill();

  for (size_t i = 0; i < data.size(); i += bytesPerLine) {
    os << "      " << setw(6) << setfill('0') << hex << i << " : ";
    for (size_t j = 0; j < bytesPerLine; ++j) {
      if (j && group && (j % group == 0)) os << ' ';
      if (i + j < data.size()) {
        os << setw(2) << int(data[i + j]);
      } else {
        os << "  ";
      }
      if (j + 1 != bytesPerLine) os << ' ';
    }
    os << "  |";
    for (size_t j = 0; j < bytesPerLine && i + j < data.size(); ++j) {
      unsigned char c = data[i + j];
      os << (c >= 32 && c < 127 ? char(c) : '.');
    }
    os << "|\n";
  }

  os.flags(oldflags);
  os.fill(oldfill);
}

static void print_section(ostream& os, const string& secName, const SectionEntry& s) {
  os << "SECTION \"" << escape_str(secName) << "\" {\n";
  os << "  base   : " << s.base << "\n";
  os << "  size   : " << s.data.size() << " bytes\n";
  os << "  relocs : " << s.rela.size() << "\n";
  if (!s.rela.empty()) {
    os << "  relocation_table:\n";
    os << "    offset   symbol                     addend\n";
    os << "    ------   -------------------------  -------\n";
    for (const auto& r : s.rela) {
      os << "    " << setw(6) << r.offset
         << "   " << left << setw(25) << escape_str(r.symbol) << right
         << "  " << r.addend << "\n";
    }
  }
  if (!s.data.empty()) {
    os << "  data:\n";
    hex_dump(os, s.data, 16, 4);
  }
  os << "}\n\n";
}

static void print_symbol_header(ostream& os) {
  os << "SYMBOL TABLE\n";
  os << "  name                       section       value   scope    def\n";
  os << "  -------------------------  ------------  ------  -------  ----\n";
}
static const char* scope_name(bool global) { return global ? "GLOBAL" : "LOCAL"; }

static void print_symbol(ostream& os, const SymbolEntry& s) {
  auto oldflags = os.flags();
  os << "  " << left << setw(25) << escape_str(s.name) << "  "
     << left << setw(12) << escape_str(s.section) << "  "
     << right << setw(6) << s.value << "  "
     << left << setw(7) << scope_name(s.global) << "  "
     << (s.defined ? "yes" : "no") << "\n";
  os.flags(oldflags);
}

// -------- javne funkcije za ispis --------
void writeObjectText(ostream& os,
                     const unordered_map<string, SectionEntry>& sectionTable,
                     const unordered_map<string, SymbolEntry>&  symbolTable) {
  os << "=== SECTIONS (" << sectionTable.size() << ") ===\n\n";
  for (const auto& kv : sectionTable) {
    const string& name = kv.first;
    const SectionEntry& s = kv.second;
    print_section(os, name, s);
  }

  os << "=== SYMBOLS (" << symbolTable.size() << ") ===\n";
  print_symbol_header(os);
  for (const auto& kv : symbolTable) {
    print_symbol(os, kv.second);
  }
  os << "\n";
}

void writeObjectTextFile(const string& path,
                         const unordered_map<string, SectionEntry>& sectionTable,
                         const unordered_map<string, SymbolEntry>&  symbolTable) {
  ofstream f(path);
  if (!f) throw runtime_error("cannot open for write: " + path);
  writeObjectText(f, sectionTable, symbolTable);
  if (!f) throw runtime_error("write failed: " + path);
}
