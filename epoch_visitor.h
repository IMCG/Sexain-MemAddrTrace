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
  virtual void Visit(const BlockSet& blocks) = 0;
};

class BlockCountVisitor : public EpochVisitor {
 public:
  BlockCountVisitor() : count_(0) { }
  void Visit(const BlockSet& blocks) { count_ += blocks.size(); }
  uint64_t count() const { return count_; }
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

class EpochDirtVsisitor : public PageVisitor {
 public:
  EpochDirtVsisitor(int page_bits);
  void Visit(const BlockSet& blocks);
  int FillEpochDirts(double dirts[], const int num_buckets) const;
 protected:
  typedef std::unordered_map<uint64_t, int> PageDirts;
  const PageDirts& page_dirts() const { return page_dirts_; }
 private:
  PageDirts page_dirts_;
  std::vector<double> sum_ratios_;
};

class PageStatsVisitor : public EpochDirtVsisitor {
 public:
  PageStatsVisitor(int page_bits) : EpochDirtVsisitor(page_bits) { }
  void Visit(const BlockSet& blocks);
  int FillPageStats(double avg_epochs[], const int num_buckets) const;
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

PageVisitor::PageVisitor(int page_bits) :
    page_bits_(page_bits), page_blocks_(1 << (page_bits - CACHE_BLOCK_BITS)) {
  assert(page_blocks_);
  num_visits_ = 0;
}

void PageVisitor::Visit(const BlockSet& blocks) {
  ++num_visits_;
}

// EpochDirtVsisitor

EpochDirtVsisitor::EpochDirtVsisitor(int page_bits) :
    PageVisitor(page_bits), sum_ratios_(page_blocks(), 0.0) {
}

void EpochDirtVsisitor::Visit(const BlockSet& blocks) {
  PageVisitor::Visit(blocks);
  // Count how many blocks are dirty in a page
  page_dirts_.clear();
  for (BlockSet::const_iterator it = blocks.begin();
      it != blocks.end(); ++it) {
    page_dirts_[(*it) >> (page_bits() - CACHE_BLOCK_BITS)] += 1;
  }
  if (page_dirts_.empty()) return;

  for (PageDirts::iterator it = page_dirts_.begin();
      it != page_dirts_.end(); ++it) {
    sum_ratios_[it->second - 1] += (double)1 / page_dirts_.size();
  }
}

int EpochDirtVsisitor::FillEpochDirts(double dirts[], const int n) const {
  assert(page_blocks() % n == 0);
  for (int i = 0; i < n; ++i) dirts[i] = 0.0;
  if (num_visits()) {
    int unit = page_blocks() / n;
    for (int i = 0; i < page_blocks(); ++i) {
      dirts[i / unit] += sum_ratios_[i] / num_visits();
    }
  }
  return num_visits();
}

// PageStatsVisitor

void PageStatsVisitor::Visit(const BlockSet& blocks) {
  EpochDirtVsisitor::Visit(blocks);
  for (PageDirts::const_iterator it = page_dirts().begin();
      it != page_dirts().end(); ++it) {
    DirtyStats& stats = page_stats_[it->first];
    stats.blocks += it->second;
    stats.epochs += 1;
  }
}

PageStatsVisitor::DirtyStats PageStatsVisitor::StatsOf(uint64_t page_i) const {
  PageStats::const_iterator it = page_stats_.find(page_i);
  if (it != page_stats_.end()) return it->second;
  else return DirtyStats();
}

int PageStatsVisitor::FillPageStats(double epochs[], const int n) const {
  assert(page_blocks() % n == 0);
  for (int i = 0; i < n; ++i) epochs[i] = 0.0;

  std::vector<int> num_pages(n, 0);
  int unit = page_blocks() / n;

  for (PageStats::const_iterator it = page_stats_.begin();
      it != page_stats_.end(); ++it) {
    const DirtyStats& stats = it->second;
    int bi = (stats.blocks / stats.epochs - 1) / unit;
    epochs[bi] += stats.epochs;
    num_pages[bi] += 1;
  }

  for (int i = 0; i < n; ++i) {
    if (num_pages[i]) {
      epochs[i] /= num_pages[i];
    } else epochs[i] = 0.0;
  }

  for (int i = 0; i < n; ++i) {
    assert(epochs[i] == 0.0 || (1 <= epochs[i] && epochs[i] <= num_visits()));
  }
  return num_visits();
}

// PageDirtVisitor

void PageDirtVisitor::Visit(const BlockSet& blocks) {
  PageStatsVisitor::Visit(blocks);
  for (BlockSet::const_iterator it = blocks.begin(); it != blocks.end(); ++it) {
    overall_blocks_.insert(*it);
  }
}

int PageDirtVisitor::FillOverallDirts(double dirts[], const int n) const {
  assert(page_blocks() % n == 0);
  for (int i = 0; i < n; ++i) dirts[i] = 0.0;

  PageDirts overall_dirts;
  for (BlockSet::const_iterator it = overall_blocks_.begin();
      it != overall_blocks_.end(); ++it) {
    overall_dirts[(*it) >> (page_bits() - CACHE_BLOCK_BITS)] += 1;
  }

  std::vector<int> num_pages(n, 0);
  int unit = page_blocks() / n;
  for (PageDirts::const_iterator it = overall_dirts.begin();
      it != overall_dirts.end(); ++it) {
    DirtyStats stats = StatsOf(it->first);
    int bi = (stats.blocks / stats.epochs - 1) / unit;
    assert(it->second <= page_blocks());
    dirts[bi] += it->second;
    num_pages[bi] += 1;
  }

  for (int i = 0; i < n; ++i) {
    if (num_pages[i]) {
      dirts[i] /= num_pages[i];
    } else dirts[i] = 0.0;
  }
  return num_visits();
}

#endif // SEXAIN_EPOCH_VISITOR_H_
