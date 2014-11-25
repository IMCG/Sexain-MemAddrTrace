//
//  stats.h
//
//  Created by Ren Jinglei on 11/24/14.
//  Copyright (c) 2014 Ren Jinglei <jinglei@ren.systems>.
//

#ifndef TraceSimulator_Stats_h
#define TraceSimulator_Stats_h

static const uint64_t kCacheLineSize = 64; // bytes

class Stats {
 public:
  Stats(int buffer_len, uint64_t block_size, bool has_dram) :
      buffer_len_(buffer_len), block_size_(block_size), has_dram_(has_dram),
      nvm_through_(0), dram_through_(0), epoch_num_(0) { }
  
  uint64_t nvm_through() const { return nvm_through_; }
  uint64_t dram_through() const { return dram_through_; }
  size_t epoch_num() const { return epoch_num_; }
  
  void OnCopy(size_t num = 1) {
    if (has_dram_) {
      dram_through_ += block_size_ * num;
    } else {
      nvm_through_ += block_size_ * num;
    }
  }
  
  void OnHit() {
    if (has_dram_) {
      dram_through_ += kCacheLineSize;
    } else {
      nvm_through_ += kCacheLineSize;
    }
  }
  
  void OnEpoch(size_t num) {
    ++epoch_num_;
    if (has_dram_) {
      nvm_through_ += buffer_len_ * kCacheLineSize + num * block_size_;
    } else {
      nvm_through_ += buffer_len_ * kCacheLineSize;
    }
  }
  
 private:
  const int buffer_len_;
  const uint64_t block_size_;
  const bool has_dram_;
  
  uint64_t nvm_through_;
  uint64_t dram_through_;
  size_t epoch_num_;
};

#endif
