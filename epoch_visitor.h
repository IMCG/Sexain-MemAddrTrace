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

PageVisitor::PageVisitor(int page_bits) :
    page_bits_(page_bits), page_blocks_(1 << (page_bits - CACHE_BLOCK_BITS)) {
  assert(CACHE_BLOCK_BITS <= page_bits_ && page_bits_ <= 64);
  assert(page_blocks_ > 0);
  num_visits_ = 0;
}

// EpochDirtVisitor

EpochDirtVisitor::EpochDirtVisitor(int page_bits) :
    PageVisitor(page_bits), dirt_pages_(page_blocks(), 0) {
  page_accum_ = 0;
}

void EpochDirtVisitor::Visit(const BlockSet& blocks) {
  PageVisitor::Visit(blocks);
  page_dirts_.clear();
  // Count how many blocks of a page are dirty within an epoch
  for (BlockSet::const_iterator it = blocks.begin();
      it != blocks.end(); ++it) {
    assert((page_dirts_[(*it) >> (page_bits() - CACHE_BLOCK_BITS)] += 1)
        <= page_blocks());
  }

  for (PageDirts::iterator it = page_dirts_.begin();
      it != page_dirts_.end(); ++it) {
    dirt_pages_[it->second - 1] += 1;
    ++page_accum_;
  }
}

int EpochDirtVisitor::FillEpochDirts(double ratios[], const int n) const {
  assert(page_blocks() % n == 0);
  for (int i = 0; i < n; ++i) ratios[i] = 0.0;
  if (page_accum_) {
    int unit = page_blocks() / n;
    for (int i = 0; i < page_blocks(); ++i) {
      ratios[i / unit] += (double)dirt_pages_[i] / page_accum_;
    }
  }
  return num_visits();
}

// PageStatsVisitor

void PageStatsVisitor::Visit(const BlockSet& blocks) {
  EpochDirtVisitor::Visit(blocks);
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

int PageStatsVisitor::FillEpochSpans(double epochs[], const int n) const {
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
      assert(epochs[i] <= num_visits());
    } else assert(epochs[i] == 0.0);
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

  PageDirts overall_page_dirts;
  for (BlockSet::const_iterator it = overall_blocks_.begin();
      it != overall_blocks_.end(); ++it) {
    overall_page_dirts[(*it) >> (page_bits() - CACHE_BLOCK_BITS)] += 1;
  }

  std::vector<int> num_pages(n, 0);
  int unit = page_blocks() / n;
  for (PageDirts::const_iterator it = overall_page_dirts.begin();
      it != overall_page_dirts.end(); ++it) {
    DirtyStats stats = StatsOf(it->first);
    int bi = (stats.blocks / stats.epochs - 1) / unit;
    dirts[bi] += it->second;
    num_pages[bi] += 1;
  }

  for (int i = 0; i < n; ++i) {
    if (num_pages[i]) {
      dirts[i] /= num_pages[i];
      assert(dirts[i] <= page_blocks());
    } else assert(dirts[i] == 0.0);
  }
  return num_visits();
}

#endif // SEXAIN_EPOCH_VISITOR_H_
