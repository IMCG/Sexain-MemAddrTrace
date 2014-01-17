// mem_addr_stat.h
// Copyright (c) 2013 Jinglei Ren <jinglei.ren@stanzax.org>

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
  void Fillout(double percents[], const int num_buckets);
  unsigned int GetBlockNum() const { return blocks_.size(); } 
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

void MemAddrStat::Fillout(double percents[], const int num_buckets) {
  assert(pages_.empty() && num_blocks_ % num_buckets == 0);
  // Count how many blocks are dirty in a page
  for (BlockSet::iterator it = blocks_.begin();
      it != blocks_.end(); ++it) {
    pages_[(*it) >> (page_bits_ - CACHE_BLOCK_BITS)] += 1;
  }

  // Clear percents[] for page count
  for (int i = 0; i < num_buckets; ++i) {
    percents[i] = 0;
  }

  int bi;
  for (PageMap::iterator it = pages_.begin();
      it != pages_.end(); ++it) {
    // num_blocks is exactly divisible by num
    bi = (int)((it->second - 1) / (double)num_blocks_ * num_buckets);
    assert(bi >= 0 && bi < num_buckets);
    percents[bi] += 1;
  }

  double num_pages = 0; // for integrity check
  for (int i = 0; i < num_buckets; ++i) {
    num_pages += percents[i];
    percents[i] /= (pages_.size() ? pages_.size() : 1);
  }
  assert(num_pages == pages_.size());
}

#endif // SEXAIN_MEM_ADDR_STAT_H_

