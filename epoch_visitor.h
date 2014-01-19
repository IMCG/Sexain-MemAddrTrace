// epoch_visitor.h
// Copyright (c) 2014 Jinglei Ren <jinglei.ren@stanzax.org>

#ifndef SEXAIN_EPOCH_VISITOR_H_
#define SEXAIN_EPOCH_VISITOR_H_

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <cassert>

#define CACHE_BLOCK_BITS 6

typedef std::unordered_map<uint64_t, uint64_t> BlockSet;

class EpochVisitor {
 public:
  virtual void Visit(const BlockSet& blocks) = 0;
};

class BlockCountVisitor : public EpochVisitor {
 public:
  BlockCountVisitor() : count_(0) { }
  void Visit(const BlockSet& blocks) { count_ += blocks.size(); }
  uint64_t count() { return count_; }
 private:
  uint64_t count_;
};

class PageVisitor : public EpochVisitor {
 public:
  PageVisitor(int page_bits);
  virtual void Visit(const BlockSet& blocks);
  int page_bits() const { return page_bits_; }
  int page_blocks() const { return page_blocks_; }
  int num_visits() const { return num_visits_; }
 private:
  const int page_bits_;
  const int page_blocks_;
  int num_visits_;
};

class DirtyRatioVisitor : public PageVisitor {
 public:
  DirtyRatioVisitor(int page_bits);
  void Visit(const BlockSet& blocks);
  int FillDirtyRatios(double percents[], const int num_buckets);
 protected:
  typedef std::unordered_map<uint64_t, int> PageDirts;
  PageDirts page_dirts_;
 private:
  std::vector<double> sum_ratios_;
};

class PageStatsVisitor : public DirtyRatioVisitor {
 public:
  PageStatsVisitor(int page_bits);
  void Visit(const BlockSet& blocks);
  int FillPageStats(int counts[], const int num_buckets);
 private:
  struct PageStats { int blocks; int epochs; };
  std::unordered_map<uint64_t, PageStats> sum_stats_;
};

// Implementations

// PageVisitor

PageVisitor::PageVisitor(int page_bits) :
    page_bits_(page_bits), page_blocks_(1 << (page_bits - CACHE_BLOCK_BITS)) {
  assert(page_blocks_);
  num_visits_ = 0;
}

void PageVisitor::Visit(const BlockSet& blocks) {
  ++num_visits_;
}

// DirtyRatioVisitor

DirtyRatioVisitor::DirtyRatioVisitor(int page_bits) :
    PageVisitor(page_bits), sum_ratios_(page_blocks(), 0.0) {
}

void DirtyRatioVisitor::Visit(const BlockSet& blocks) {
  PageVisitor::Visit(blocks);
  // Count how many blocks are dirty in a page
  page_dirts_.clear();
  for (BlockSet::const_iterator it = blocks.begin();
      it != blocks.end(); ++it) {
    page_dirts_[(it->first) >> (page_bits() - CACHE_BLOCK_BITS)] += 1;
  }
  if (page_dirts_.empty()) return;

  for (PageDirts::iterator it = page_dirts_.begin();
      it != page_dirts_.end(); ++it) {
    sum_ratios_[it->second - 1] += (double)1 / page_dirts_.size();
  }
}

int DirtyRatioVisitor::FillDirtyRatios(double percents[], const int n) {
  assert(page_blocks() % n == 0);
  for (int i = 0; i < n; ++i) percents[i] = 0.0;
  if (num_visits()) {
    int unit = page_blocks() / n;
    for (int i = 0; i < page_blocks(); ++i) {
      percents[i / unit] += sum_ratios_[i] / num_visits();
    }
  }
  return num_visits();
}

#endif // SEXAIN_EPOCH_VISITOR_H_
