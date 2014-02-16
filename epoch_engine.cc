// epoch_engine.cc
// Copyright (c) 2014 Jinglei Ren <jinglei.ren@stanzax.org>

#include "epoch_engine.h"

// EpochEngine

void EpochEngine::NewEpoch() {
  for (std::vector<EpochVisitor*>::iterator it = visitors_.begin();
      it != visitors_.end(); ++it) {
    (*it)->Visit(blocks_);
  }
  ++num_epochs_;
  overall_dirts_ += blocks_.size();
  blocks_.clear();
}

// InsEpochEngine

bool InsEpochEngine::Input(const MemRecord& rec) {
  if (!EpochEngine::Input(rec)) return false;
  if (overall_ins() > epoch_max_) {
    if (epoch_max_) {
      NewEpoch();
    }
    epoch_max_ = (rec.ins_seq / interval() + 1) * interval();
  }
  DirtyBlock(rec.mem_addr);
  return true;
}

