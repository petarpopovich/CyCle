#include "structures.hpp"
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <iostream>

using namespace std;


static inline char _HX(unsigned v){ return "0123456789ABCDEF"[v & 0xF]; }
static inline void _out8(std::ostream& os, uint32_t v){ for(int i=7;i>=0;--i) os.put(_HX((v>>(i*4))&0xF)); }
static inline void _out2(std::ostream& os, uint8_t b){ os.put(_HX((b>>4)&0xF)); os.put(_HX(b&0xF)); }

// tekstualni ispis: AAAAAAAA: BB BB ... (globalno 8B, sekcije spojene)
inline void writeExecutableText(std::ostream& os,
                                  const unordered_map<string, SectionEntry>& sectionTable)
{
  struct N{ uint32_t base; const vector<unsigned char>* data; };
  std::vector<N> secs; secs.reserve(sectionTable.size());
  for (const auto& kv : sectionTable) if (!kv.second.data.empty())
    secs.push_back({ (uint32_t)kv.second.base, &kv.second.data });
  std::sort(secs.begin(), secs.end(), [](const N& a, const N& b){ return a.base < b.base; });
  if (secs.empty()) return;

  uint8_t blk[8] = {}; bool has=false;
  uint32_t blkStart = (secs.front().base & ~uint32_t(7));

  auto flush = [&](){
    if(!has) return;
    _out8(os, blkStart); os << ": ";
    for(int i=0;i<8;++i){ _out2(os, blk[i]); os << ' '; }
    os << '\n';
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

inline void writeExecutableTextFile(const string& path,
                                      const unordered_map<string, SectionEntry>& sectionTable)
{
  std::ofstream f(path);
  if(!f) throw std::runtime_error("cannot open for write: " + path);
  writeExecutableText(f, sectionTable);
  if(!f) throw std::runtime_error("write failed");
}
