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
  const PageDirts& page_dirts() const { return page_dirts_; }
 private:
  PageDirts page_dirts_;
  std::vector<double> sum_ratios_;
};

class PageStatsVisitor : public DirtyRatioVisitor {
 public:
  PageStatsVisitor(int page_bits) : DirtyRatioVisitor(page_bits) { }
  void Visit(const BlockSet& blocks);
  int FillPageStats(double avg_epochs[], const int num_buckets);
 private:
  struct DirtyStats {
    int blocks;
    int epochs;
    DirtyStats() : blocks(0), epochs(0) { }
  };
  typedef std::unordered_map<uint64_t, DirtyStats> PageStats;
  PageStats page_stats_;
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

// PageStatsVisitor

void PageStatsVisitor::Visit(const BlockSet& blocks) {
  DirtyRatioVisitor::Visit(blocks);
  for (PageDirts::const_iterator it = page_dirts().begin();
      it != page_dirts().end(); ++it) {
    DirtyStats& stats = page_stats_[it->first];
    stats.blocks += it->second;
    stats.epochs += 1;
  }
}

int PageStatsVisitor::FillPageStats(double avg_epochs[], const int n) {
  assert(page_blocks() % n == 0);
  for (int i = 0; i < n; ++i) avg_epochs[i] = 0;

  std::vector<double> sum_epochs(n, 0.0);
  std::vector<int> num_pages(n, 0);
  int unit = page_blocks() / n;

  for (PageStats::iterator it = page_stats_.begin();
      it != page_stats_.end(); ++it) {
    DirtyStats& stats = it->second;
    int bi = (stats.blocks / stats.epochs - 1) / unit;
    sum_epochs[bi] += stats.epochs;
    num_pages[bi] += 1;
  }

  for (int i = 0; i < page_blocks(); ++i) {
    avg_epochs[i / unit] += sum_epochs[i] / num_pages[i];
  }

  for (int i = 0; i < n; ++i) {
    assert(1 <= avg_epochs[i] && avg_epochs[i] <= num_visits());
  }
  return num_visits();
}

#endif // SEXAIN_EPOCH_VISITOR_H_
