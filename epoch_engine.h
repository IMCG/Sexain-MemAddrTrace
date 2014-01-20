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
  EpochEngine(uint64_t num_ins) : epoch_ins_(num_ins), epoch_max_(0),
      num_epochs_(0) { }
  void AddVisitor(EpochVisitor* v) { visitors_.push_back(v); }
  void Input(const MemRecord& rec); // assumes increasing ins_seq
  int num_epochs() const { return num_epochs_; }
  uint64_t epoch_ins() const { return epoch_ins_; }
 
 private:
  uint64_t epoch_ins_;
  uint64_t epoch_max_;
  int num_epochs_;
  std::vector<EpochVisitor*> visitors_;
  BlockSet blocks_;
  bool DoVisit();
  void Reset();
};

void EpochEngine::Input(const MemRecord& rec) {
  if (rec.op != 'W') return;
  if (rec.ins_seq > epoch_max_) {
    if (epoch_max_ && DoVisit()) {
      // entering next epoch
      Reset();
      ++num_epochs_;
    }
    epoch_max_ = (rec.ins_seq / epoch_ins() + 1) * epoch_ins();
  }
  blocks_.insert(rec.mem_addr >> CACHE_BLOCK_BITS);
}

bool EpochEngine::DoVisit() {
  if (blocks_.empty()) return false;
  for (std::vector<EpochVisitor*>::iterator it = visitors_.begin();
      it != visitors_.end(); ++it) {
    (*it)->Visit(blocks_);
  }
  return true;
}

void EpochEngine::Reset() {
  blocks_.clear();
}

#endif // SEXAIN_EPOCH_ENGINE_H_

