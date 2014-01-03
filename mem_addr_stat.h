// mem_addr_stat.h
// Copyright (c) Jinglei Ren <jinglei.ren@stanzax.org>

#ifndef SEXAIN_MEM_ADDR_STAT_H_
#define SEXAIN_MEM_ADDR_STAT_H_

#include <cstdint>
#include <cerrno>
#include <cassert>
#include <iostream>
#include <unordered_set>
#include <unordered_map>

typedef std::unordered_set<uint64_t> BlockSet;
typedef std::unordered_map<uint64_t, unsigned int> PageMap;

#define CACHE_BLOCK_BITS 6

class MemAddrStat {
 public:
  MemAddrStat(unsigned int page_bits);
  void Input(uint64_t addr);
  void Fillout(double percents[], const unsigned int num); 
  void Clear();

 private:
  BlockSet blocks_;
  PageMap pages_;
  const unsigned int page_bits_;
  const unsigned int num_blocks_; // in a page
};

MemAddrStat::MemAddrStat(unsigned int page_bits) :
    page_bits_(page_bits), num_blocks_(1 << (page_bits - CACHE_BLOCK_BITS)) {
  assert(num_blocks_);
}

inline void MemAddrStat::Input(uint64_t addr) {
  blocks_.insert(addr >> CACHE_BLOCK_BITS);
}

inline void MemAddrStat::Clear() {
  blocks_.clear();
  pages_.clear();
}

void MemAddrStat::Fillout(double percents[], const unsigned int num) {
  assert(pages_.empty() && num_blocks_ % num == 0);
  // Count how many blocks are dirty in a page
  for (BlockSet::iterator it = blocks_.begin();
      it != blocks_.end(); ++it) {
    pages_[(*it) >> (page_bits_ - CACHE_BLOCK_BITS)] += 1;
  }

  // Clear percents[] for page count
  for (unsigned int i = 0; i < num; ++i) {
    percents[i] = 0;
  }

  unsigned int pi;
  for (PageMap::iterator it = pages_.begin();
      it != pages_.end(); ++it) {
    // num_blocks is exactly divisible by num
    pi = (unsigned int)((it->second - 1) / (double)num_blocks_ * num);
    assert(pi < num);
    percents[pi] += 1;
  }

  double num_pages = 0; // for integrity check
  for (unsigned int i = 0; i < num; ++i) {
    num_pages += percents[i];
    percents[i] /= (pages_.size() ? pages_.size() : 1);
  }
  assert(num_pages == pages_.size());
}

#endif // SEXAIN_MEM_ADDR_STAT_H_

