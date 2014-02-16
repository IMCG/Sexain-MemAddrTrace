// epoch_visitor.cc
// Copyright (c) 2014 Jinglei Ren <jinglei.ren@stanzax.org>

#include "epoch_visitor.h"

// EpochDirtVisitor

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

