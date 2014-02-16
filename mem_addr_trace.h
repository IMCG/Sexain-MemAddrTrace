// mem_addr_trace.h
// Copyright (c) 2013 Jinglei Ren <jinglei.ren@stanzax.org>

#ifndef SEXAIN_MEM_ADDR_TRACE_H_
#define SEXAIN_MEM_ADDR_TRACE_H_

#include <cassert>
#include <cstdint>
#include <iostream>
#include "zlib.h"

#ifdef NDEBUG
#define BUG_ON(v) do { \
  if (v) std::cerr << "[Warn]" << __FILE__ << ", " << __LINE__ << std::endl; \
while(0)
#else
#define BUG_ON(v) (assert(!(v)))
#endif

class MemAddrTrace {
 public:
  MemAddrTrace(uint32_t buf_len, const char* file, uint32_t max_size_mb); 
  ~MemAddrTrace();
 
  bool Input(uint32_t ins_seq, void* addr, char op);
  bool Flush();

  uint32_t buffer_size() const { return buf_len_; }
  FILE* file() const { return file_; }

  uint64_t file_size() const { return file_size_; }
  void set_file_size(uint32_t mb) { file_size_ = (uint64_t)mb << 20; }

 private:
  const uint32_t buf_len_;
  FILE* file_;
  uint64_t file_size_; // max file size
  uint32_t end_;
  uint32_t* ins_array_;
  void** addr_array_;
  char* op_array_;
  void* ins_compressed_;
  void* addr_compressed_;
  void* op_compressed_;
};

inline MemAddrTrace::~MemAddrTrace() {
  if (file_) fclose(file_);
  delete[] ins_array_;
  delete[] addr_array_;
  delete[] op_array_;
  free(ins_compressed_);
  free(addr_compressed_);
  free(op_compressed_);
}

inline bool MemAddrTrace::Input(uint32_t ins_seq, void* addr, char op) {
  if (end_ == buf_len_ && !Flush()) {
    return false;
  }

  ins_array_[end_] = ins_seq;
  addr_array_[end_] = addr;
  op_array_[end_] = op;
  ++end_;
  return true;
}

#endif // SEXAIN_MEM_ADDR_TRACE_H_

