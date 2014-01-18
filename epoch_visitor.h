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

class DirtyRatioVisitor : public EpochVisitor {
 public:
  DirtyRatioVisitor(int page_bits);
  void Visit(const BlockSet& blocks);
  int Fillout(double percents[], const int num_buckets); // adding to percents
 private:
  const int page_bits_;
  const int num_blocks_;
  int num_visits_;
  std::vector<double> sum_ratios_;
};

// Implementations

// DirtyRatioVisitor

DirtyRatioVisitor::DirtyRatioVisitor(int page_bits) :
    page_bits_(page_bits), num_blocks_(1 << (page_bits - CACHE_BLOCK_BITS)),
    sum_ratios_(num_blocks_, 0.0) {
  assert(num_blocks_);
  num_visits_ = 0;
}

void DirtyRatioVisitor::Visit(const BlockSet& blocks) {
  std::unordered_map<uint64_t, int> pages;
  // Count how many blocks are dirty in a page
  for (BlockSet::const_iterator it = blocks.begin();
      it != blocks.end(); ++it) {
    pages[(it->first) >> (page_bits_ - CACHE_BLOCK_BITS)] += 1;
  }
  if (pages.empty()) return;

  for (std::unordered_map<uint64_t, int>::iterator it = pages.begin();
      it != pages.end(); ++it) {
    sum_ratios_[it->second - 1] += (double)1 / pages.size();
  }
  ++num_visits_;
}

int DirtyRatioVisitor::Fillout(double percents[], const int n) {
  assert(num_blocks_ % n == 0);
  if (num_visits_) {
    int unit = num_blocks_ / n;
    for (int i = 0; i < num_blocks_; ++i) {
      percents[i / unit] += sum_ratios_[i] / num_visits_;
    }
  }
  return num_visits_;
}

#endif // SEXAIN_EPOCH_VISITOR_H_
