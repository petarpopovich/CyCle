#include "structures.hpp"
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <unordered_map>

using namespace std;


// ----------------- mali binarni helperi (LE) -----------------
static inline void w8 (ofstream& f, uint8_t v){ f.put(char(v)); }
static inline void w32(ofstream& f, uint32_t v){ for (int i=0;i<4;++i) w8(f, uint8_t((v >> (8*i)) & 0xFF)); }
static inline void wI32(ofstream& f, int32_t v){ w32(f, static_cast<uint32_t>(v)); }

static inline uint8_t  r8 (ifstream& f){ return uint8_t(f.get()); }
static inline uint32_t r32(ifstream& f){ uint32_t v=0; for (int i=0;i<4;++i) v |= (uint32_t)r8(f) << (8*i); return v; }
static inline int32_t  rI32(ifstream& f){ return static_cast<int32_t>(r32(f)); }

static inline void wStr(ofstream& f, const string& s){
  w32(f, (uint32_t)s.size());
  if (!s.empty()) f.write(s.data(), (streamsize)s.size());
}
static inline string rStr(ifstream& f){
  uint32_t n = r32(f);
  string s(n, '\0');
  if (n) f.read(&s[0], (streamsize)n);
  return s;
}

// ----------------- upis .o fajla -----------------
void writeObjectFile(
  const string& path,
  const unordered_map<string, SectionEntry>& sectionTable,
  const unordered_map<string, SymbolEntry>&  symbolTable)
{
  ofstream f(path, ios::binary);
  if (!f) throw runtime_error("cannot open for write: " + path);

  // 1) Sekcije
  w32(f, (uint32_t)sectionTable.size());
  for (const auto& kv : sectionTable) {
    const string& secName = kv.first;
    const SectionEntry& sec = kv.second;

    wStr(f, secName);
    wI32(f, sec.base);

    // data
    w32(f, (uint32_t)sec.data.size());
    if (!sec.data.empty()) {
      f.write(reinterpret_cast<const char*>(sec.data.data()),
              (streamsize)sec.data.size());
    }

    // relokacije
    w32(f, (uint32_t)sec.rela.size());
    for (const auto& r : sec.rela) {
      wI32(f, r.offset);
      wStr(f, r.symbol);
      wI32(f, r.addend);
    }
  }

  // 2) Simboli
  w32(f, (uint32_t)symbolTable.size());
  for (const auto& kv : symbolTable) {
    const string& symName = kv.first;
    const SymbolEntry& s = kv.second;

    wStr(f, symName);
    wI32(f, s.value);
    w8 (f, s.global  ? 1u : 0u);
    w8 (f, s.defined ? 1u : 0u);
    wStr(f, s.section);
  }

  if (!f) throw runtime_error("write failed: " + path);
}

// ----------------- čitanje .o fajla -----------------
void readObjectFile(
  const string& path,
  unordered_map<string, SectionEntry>& sectionTable,
  unordered_map<string, SymbolEntry>&  symbolTable)
{
  ifstream f(path, ios::binary);
  if (!f) throw runtime_error("cannot open for read: " + path);

  sectionTable.clear();
  symbolTable.clear();

  // 1) Sekcije
  uint32_t nsec = r32(f);
  for (uint32_t i = 0; i < nsec; ++i) {
    string secName = rStr(f);
    SectionEntry sec;
    sec.name = secName;
    sec.base = rI32(f);

    // data
    uint32_t dataSz = r32(f);
    sec.data.resize(dataSz);
    if (dataSz) {
      f.read(reinterpret_cast<char*>(sec.data.data()), (streamsize)dataSz);
    }

    // relokacije
    uint32_t rcnt = r32(f);
    sec.rela.resize(rcnt);
    for (auto& r : sec.rela) {
      r.offset = rI32(f);
      r.symbol = rStr(f);
      r.addend = rI32(f);
    }

    sectionTable.emplace(move(secName), move(sec));
  }

  // 2) Simboli
  uint32_t nsym = r32(f);
  for (uint32_t i = 0; i < nsym; ++i) {
    SymbolEntry s;
    s.name    = rStr(f);
    s.value   = rI32(f);
    s.global  = (r8(f) != 0);
    s.defined = (r8(f) != 0);
    s.section = rStr(f);

    symbolTable.emplace(s.name, move(s));
  }

  if (!f) throw runtime_error("read failed: " + path);
}
