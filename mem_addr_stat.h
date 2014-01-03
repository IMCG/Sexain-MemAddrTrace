// mem_addr_stat.h
// Copyright (c) Jinglei Ren <jinglei.ren@stanzax.org>

#ifndef SEXAIN_MEM_ADDR_STAT_H_
#define SEXAIN_MEM_ADDR_STAT_H_

#include <cstdint>
#include <cerrno>
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
  int Fillout(double percents[], const int num); 
  void Clear();

 private:
  BlockSet blocks_;
  PageMap pages_;
  const unsigned int page_bits_;
  const unsigned int num_blocks_; // in a page
};

MemAddrStat::MemAddrStat(unsigned int page_bits) :
    page_bits_(page_bits), num_blocks_(1 << (page_bits - CACHE_BLOCK_BITS)) {
  if (num_blocks_ == 0) {
    std::cerr << "[Error] MemAddrStat init failed: page_bits="
        << page_bits_ << std::endl;
  }
}

inline void MemAddrStat::Input(uint64_t addr) {
  blocks_.insert(addr >> CACHE_BLOCK_BITS);
}

inline void MemAddrStat::Clear() {
  blocks_.clear();
  pages_.clear();
}

int MemAddrStat::Fillout(double percents[], const int num) {
  if (!pages_.empty()) {
    std::cerr << "[Warn] Fillout: group set not empty." << std::endl;
    return -EIO;
  }
  // Count how many blocks are dirty in a page
  for (BlockSet::iterator it = blocks_.begin();
      it != blocks_.end(); ++it) {
    pages_[(*it) >> (page_bits_ - CACHE_BLOCK_BITS)] += 1;
  }

  // Clear percents[] for page count
  for (int i = 0; i < num; ++i) {
    percents[i] = 0;
  }

  double percent;
  for (PageMap::iterator it = pages_.begin();
      it != pages_.end(); ++it) {
    percent = it->second / (double)num_blocks_;
    percents[int(percent * num)] += 1;
  }

  double num_pages = 0; // for integrity check
  for (int i = 0; i < num; ++i) {
    num_pages += percents[i];
    percents[i] /= pages_.size();
  }
  if (num_pages != pages_.size()) {
    std::cerr << "[Warn] Fillout() integrity check failed: "
        << num_pages << "!=" << pages_.size() << std::endl;
    return -EFAULT;
  }
  return 0;
}

#endif // SEXAIN_MEM_ADDR_STAT_H_

