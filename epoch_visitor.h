// epoch_visitor.h
// Copyright (c) 2014 Jinglei Ren <jinglei.ren@stanzax.org>

#ifndef SEXAIN_EPOCH_VISITOR_H_
#define SEXAIN_EPOCH_VISITOR_H_

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <cassert>

#define CACHE_BLOCK_BITS 6

typedef std::unordered_set<uint64_t> BlockSet;

class EpochVisitor {
 public:
  virtual void Visit(const BlockSet& dirty_blocks) = 0;
};

class PageVisitor : public EpochVisitor {
 public:
  PageVisitor(int page_bits);
  virtual void Visit(const BlockSet& blocks) { ++num_visits_; }
  int page_bits() const { return page_bits_; }
  int page_blocks() const { return page_blocks_; }
  int num_visits() const { assert(num_visits_ >= 0); return num_visits_; }
 private:
  const int page_bits_;
  const int page_blocks_;
  int num_visits_;
};

class EpochDirtVisitor : public PageVisitor {
 public:
  EpochDirtVisitor(int page_bits);
  void Visit(const BlockSet& blocks);
  int FillEpochDirts(double dirts[], const int num_buckets) const;
 protected:
  typedef std::unordered_map<uint64_t, int> PageDirts;
  const PageDirts& page_dirts() const { return page_dirts_; }
 private:
  PageDirts page_dirts_;
  std::vector<unsigned int> dirt_pages_;
  unsigned int page_accum_;
};

class PageStatsVisitor : public EpochDirtVisitor {
 public:
  PageStatsVisitor(int page_bits) : EpochDirtVisitor(page_bits) { }
  void Visit(const BlockSet& blocks);
  int FillEpochSpans(double avg_epochs[], const int num_buckets) const;
 protected:
  struct DirtyStats {
    int blocks;
    int epochs;
    DirtyStats() : blocks(0), epochs(0) { }
  };
  typedef std::unordered_map<uint64_t, DirtyStats> PageStats;
  DirtyStats StatsOf(uint64_t page_i) const;
 private:
  PageStats page_stats_;
};

class PageDirtVisitor : public PageStatsVisitor {
 public:
  PageDirtVisitor(int page_bits) : PageStatsVisitor(page_bits) { }
  void Visit(const BlockSet& blocks);
  int FillOverallDirts(double dirts[], const int num_buckets) const;
 private:
  BlockSet overall_blocks_;
};

// Implementations

// PageVisitor

inline PageVisitor::PageVisitor(int page_bits) :
    page_bits_(page_bits), page_blocks_(1 << (page_bits - CACHE_BLOCK_BITS)) {
  assert(CACHE_BLOCK_BITS <= page_bits_ && page_bits_ <= 64);
  assert(page_blocks_ > 0);
  num_visits_ = 0;
}

// EpochDirtVisitor

inline EpochDirtVisitor::EpochDirtVisitor(int page_bits) :
    PageVisitor(page_bits), dirt_pages_(page_blocks(), 0) {
  page_accum_ = 0;
}

// PageStatsVisitor

inline PageStatsVisitor::DirtyStats PageStatsVisitor::StatsOf(
    uint64_t page_i) const {
  PageStats::const_iterator it = page_stats_.find(page_i);
  if (it != page_stats_.end()) return it->second;
  else return DirtyStats();
}

// PageDirtVisitor

inline void PageDirtVisitor::Visit(const BlockSet& blocks) {
  PageStatsVisitor::Visit(blocks);
  for (BlockSet::const_iterator it = blocks.begin(); it != blocks.end(); ++it) {
    overall_blocks_.insert(*it);
  }
}

#endif // SEXAIN_EPOCH_VISITOR_H_

