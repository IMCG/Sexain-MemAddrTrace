// epoch_engine.h
// Copyright (c) 2014 Jinglei Ren <jinglei.ren@stanzax.org>

#ifndef SEXAIN_EPOCH_ENGINE_H_
#define SEXAIN_EPOCH_ENGINE_H_

#include <cstdint>
#include <vector>
#include "mem_addr_parser.h"
#include "epoch_visitor.h"

class EpochEngine {
 public:
  EpochEngine(int interval);
  void AddVisitor(EpochVisitor* v) { visitors_.push_back(v); }
  virtual bool Input(const MemRecord& rec); // assumes increasing ins_seq
  int num_epochs() const { return num_epochs_; }
  int interval() const { return interval_; }
  uint64_t overall_ins() const { return overall_ins_; }
  uint64_t overall_dirts() const { return overall_dirts_; }
 protected:
  void DirtyBlock(uint64_t mem_addr);
  int NumBlocks() { return blocks_.size(); }
  void NextEpoch();
 private:
  std::vector<EpochVisitor*> visitors_;
  BlockSet blocks_;
  int interval_;
  int num_epochs_;
  uint64_t overall_ins_;
  uint64_t overall_dirts_;
};

class DirtEpochEngine : public EpochEngine {
 public:
  DirtEpochEngine(int epoch_dirts) : EpochEngine(epoch_dirts) { }
  bool Input(const MemRecord& rec);
};

class InsEpochEngine : public EpochEngine {
 public:
  InsEpochEngine(int num_ins) : EpochEngine(num_ins), epoch_max_(0) { }
  bool Input(const MemRecord& rec);
 private:
  uint64_t epoch_max_;
};

// Implementations

// EpochEngine

EpochEngine::EpochEngine(int interval) : interval_(interval) {
  num_epochs_ = 0;
  overall_ins_ = 0;
  overall_dirts_ = 0;
}

bool EpochEngine::Input(const MemRecord& rec) {
  if (rec.op != 'W') return false;
  assert(rec.ins_seq >= overall_ins_);
  overall_ins_ = rec.ins_seq;
  return true;
}

void EpochEngine::DirtyBlock(uint64_t mem_addr) {
  blocks_.insert(mem_addr >> CACHE_BLOCK_BITS);
}

void EpochEngine::NextEpoch() {
  for (std::vector<EpochVisitor*>::iterator it = visitors_.begin();
      it != visitors_.end(); ++it) {
    (*it)->Visit(blocks_);
  }
  ++num_epochs_;
  overall_dirts_ += blocks_.size();
  blocks_.clear();
}

// DirtEpochEngine

bool DirtEpochEngine::Input(const MemRecord& rec) {
  if (!EpochEngine::Input(rec)) return false;
  if (NumBlocks() == interval()) NextEpoch();
  DirtyBlock(rec.mem_addr); 
  return true;
}

// InsEpochEngine

bool InsEpochEngine::Input(const MemRecord& rec) {
  if (!EpochEngine::Input(rec)) return false;
  if (overall_ins() > epoch_max_) {
    if (epoch_max_) {
      NextEpoch();
    }
    epoch_max_ = (rec.ins_seq / interval() + 1) * interval();
  }
  DirtyBlock(rec.mem_addr);
  return true;
}

#endif // SEXAIN_EPOCH_ENGINE_H_

