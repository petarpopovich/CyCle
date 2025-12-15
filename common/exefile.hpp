#include "structures.hpp"
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <cctype>

using namespace std;


static inline char _H(unsigned v){ return "0123456789ABCDEF"[v & 0xF]; }
static inline void _put8(std::ofstream& f, uint32_t v){ for(int i=7;i>=0;--i) f.put(_H((v>>(i*4))&0xF)); }
static inline void _put2(std::ofstream& f, uint8_t b){ f.put(_H((b>>4)&0xF)); f.put(_H(b&0xF)); }
static inline uint8_t _hex(char c){
  if(c>='0'&&c<='9') return uint8_t(c-'0');
  c = (char)std::toupper((unsigned char)c);
  if(c>='A'&&c<='F') return uint8_t(c-'A'+10);
  throw std::runtime_error("invalid hex digit");
}

// zapis: globalno poravnato na 8 bajtova, sekcije spojene
inline void writeExecutableFile(const string& path,
                                  const unordered_map<string, SectionEntry>& sectionTable)
{
  struct N{ uint32_t base; const vector<unsigned char>* data; };
  std::vector<N> secs; secs.reserve(sectionTable.size());
  for (const auto& kv : sectionTable) if (!kv.second.data.empty())
    secs.push_back({ (uint32_t)kv.second.base, &kv.second.data });
  std::sort(secs.begin(), secs.end(), [](const N& a, const N& b){ return a.base < b.base; });

  std::ofstream f(path, std::ios::binary | std::ios::trunc);
  if (!f) throw std::runtime_error("cannot open for write: " + path);
  if (secs.empty()) return;

  uint8_t blk[8] = {}; bool has=false;
  uint32_t blkStart = (secs.front().base & ~uint32_t(7));

  auto flush = [&](){
    if(!has) return;
    _put8(f, blkStart);
    for(int i=0;i<8;++i) _put2(f, blk[i]);
    f.put('\n');
    for(int i=0;i<8;++i) blk[i]=0;
    has=false;
  };
  auto advance_to = [&](uint32_t addr){
    while(addr >= blkStart+8){ flush(); blkStart += 8; }
  };

  for (const auto& n : secs) {
    uint32_t a = n.base;
    const auto& d = *n.data;
    advance_to(a);
    for(size_t i=0;i<d.size();++i,++a){
      if(a == blkStart+8){ flush(); blkStart += 8; }
      blk[a-blkStart] = d[i]; has=true;
    }
  }
  flush();
}

// čitanje u mapu memorije
inline void readExecutableFile(const string& path,
                           unordered_map<unsigned int, unsigned char>& memory)
{
  std::ifstream f(path, std::ios::binary);
  if(!f) throw std::runtime_error("cannot open for read: " + path);
  string line;
  while(std::getline(f, line)){
    while(!line.empty() && (line.back()=='\r'||line.back()==' '||line.back()=='\t')) line.pop_back();
    if(line.empty()) continue;
    if(line.size()!=24) throw std::runtime_error("bad line length");
    uint32_t addr=0; for(int i=0;i<8;++i) addr=(addr<<4)|_hex(line[i]);
    for(int i=0;i<8;++i){
      uint8_t hi=_hex(line[8+2*i]), lo=_hex(line[8+2*i+1]);
      memory[addr+(uint32_t)i] = uint8_t((hi<<4)|lo);
    }
  }
  if(!f.eof()) throw std::runtime_error("read failed");
}
