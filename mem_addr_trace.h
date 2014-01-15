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

MemAddrTrace::MemAddrTrace(uint32_t buf_len, const char* file, uint32_t max_mb):
    buf_len_(buf_len), end_(0) {
  file_ = fopen(file, "wb");
  fwrite(&buf_len_, sizeof(buf_len_), 1, file_);

  uint32_t ptr_bytes = sizeof(void*);
  fwrite(&ptr_bytes, sizeof(ptr_bytes), 1, file_);

  set_file_size(max_mb);
  ins_array_ = new uint32_t[buf_len_];
  addr_array_ = new void*[buf_len_];
  op_array_ = new char[buf_len_];
  ins_compressed_ = malloc(compressBound(sizeof(uint32_t) * buf_len_));
  addr_compressed_ = malloc(compressBound(sizeof(void*) * buf_len_));
  op_compressed_ = malloc(compressBound(sizeof(char) * buf_len_));
}

MemAddrTrace::~MemAddrTrace() {
  if (file_) fclose(file_);
  delete[] ins_array_;
  delete[] addr_array_;
  delete[] op_array_;
  free(ins_compressed_);
  free(addr_compressed_);
  free(op_compressed_);
}

// Should be protected by lock for multi-threading
bool MemAddrTrace::Flush() {
  BUG_ON(end_ > buf_len_);
  if (end_ == 0) return true;
  if (!file_ || (uint64_t)ftell(file_) > file_size_) return false;

  uLong len;

  len = compressBound(sizeof(uint32_t) * end_);
  BUG_ON(compress((Bytef*)ins_compressed_, &len,
      (Bytef*)ins_array_, sizeof(uint32_t) * end_) != Z_OK);
  BUG_ON(fwrite(&len, sizeof(len), 1, file_) != 1);
  BUG_ON(fwrite(ins_compressed_, 1, len, file_) != len);

  len = compressBound(sizeof(void*) * end_);
  BUG_ON(compress((Bytef*)addr_compressed_, &len,
      (Bytef*)addr_array_, sizeof(void*) * end_) != Z_OK);
  BUG_ON(fwrite(&len, sizeof(len), 1, file_) != 1);
  BUG_ON(fwrite(addr_compressed_, 1, len, file_) != len);

  len = compressBound(sizeof(char) * end_);
  BUG_ON(compress((Bytef*)op_compressed_, &len,
      (Bytef*)op_array_, sizeof(char) * end_) != Z_OK);
  BUG_ON(fwrite(&len, sizeof(len), 1, file_) != 1);
  BUG_ON(fwrite(op_compressed_, 1, len, file_) != len);

  fflush(file_);
  end_ = 0;
  return true;
}

bool MemAddrTrace::Input(uint32_t ins_seq, void* addr, char op) {
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
