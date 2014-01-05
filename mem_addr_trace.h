// mem_addr_trace.h
// Copyright (c) 2013 Jinglei Ren <jinglei.ren@stanzax.org>

#ifndef SEXAIN_MEM_ADDR_TRACE_H_
#define SEXAIN_MEM_ADDR_TRACE_H_

#include <stdio.h>
#include <cassert>
#include "pin.H"
#include "zlib.h"

class MemAddrTrace {
 public:
  MemAddrTrace(UINT32 buf_size, const char* file, UINT32 max_size_mb); 
  ~MemAddrTrace();
 
  bool Input(UINT32 ins_seq, VOID* addr, char op);
  void Flush();

  UINT32 buffer_size() const { return buf_len_; }
  FILE* file() const { return file_; }

  UINT64 file_size() const { return file_size_; }
  void set_file_size(UINT32 mb) { file_size_ = (UINT64)mb << 20; }

 private:
  bool DoFlush();

  const UINT32 buf_len_;
  FILE* file_;
  UINT64 file_size_; // max file size
  PIN_LOCK lock_;
  UINT32 end_;
  UINT32* ins_array_;
  VOID** addr_array_;
  char* op_array_;
  void* ins_compressed_;
  void* addr_compressed_;
  void* op_compressed_;
};

MemAddrTrace::MemAddrTrace(UINT32 buf_size, const char* file, UINT32 max_mb):
    buf_len_(buf_size), end_(0) {
  file_ = fopen(file, "wb");
  fwrite(&buf_size, sizeof(buf_size), 1, file_);

  UINT32 ptr_bytes = sizeof(VOID*);
  fwrite(&ptr_bytes, sizeof(ptr_bytes), 1, file_);

  set_file_size(max_mb);
  PIN_InitLock(&lock_);
  ins_array_ = new UINT32[buf_len_];
  addr_array_ = new VOID*[buf_len_];
  op_array_ = new char[buf_len_];
  ins_compressed_ = malloc(compressBound(sizeof(UINT32) * buf_len_));
  addr_compressed_ = malloc(compressBound(sizeof(VOID*) * buf_len_));
  op_compressed_ = malloc(compressBound(sizeof(char) * buf_len_));
}

MemAddrTrace::~MemAddrTrace() {
  fclose(file_);
  delete[] ins_array_;
  delete[] addr_array_;
  delete[] op_array_;
  free(ins_compressed_);
  free(addr_compressed_);
  free(op_compressed_);
}

// Should be protected by lock_
bool MemAddrTrace::DoFlush() {
  assert(end_ <= buf_len_);
  if (end_ == 0) return true;
  if ((UINT64)ftell(file_) > file_size_) return false;

  uLong len;

  len = compressBound(sizeof(UINT32) * end_);
  assert(compress((Bytef*)ins_compressed_, &len,
      (Bytef*)ins_array_, sizeof(UINT32) * end_) == Z_OK);
  assert(fwrite(&len, sizeof(len), 1, file_) == 1);
  assert(fwrite(ins_compressed_, 1, len, file_) == len);

  len = compressBound(sizeof(VOID*) * end_);
  assert(compress((Bytef*)addr_compressed_, &len,
      (Bytef*)addr_array_, sizeof(VOID*) * end_) == Z_OK);
  assert(fwrite(&len, sizeof(len), 1, file_) == 1);
  assert(fwrite(addr_compressed_, 1, len, file_) == len);

  len = compressBound(sizeof(char) * end_);
  assert(compress((Bytef*)op_compressed_, &len,
      (Bytef*)op_array_, sizeof(char) * end_) == Z_OK);
  assert(fwrite(&len, sizeof(len), 1, file_) == 1);
  assert(fwrite(op_compressed_, 1, len, file_) == len);

  end_ = 0;
  return true;
}

bool MemAddrTrace::Input(UINT32 ins_seq, VOID* addr, char op) {
  PIN_GetLock(&lock_, 0);
  if (end_ == buf_len_ && !DoFlush()) {
    PIN_ReleaseLock(&lock_);
    return false;
  }

  ins_array_[end_] = ins_seq;
  addr_array_[end_] = addr;
  op_array_[end_] = op;
  ++end_;
  PIN_ReleaseLock(&lock_);
  return true;
}

void MemAddrTrace::Flush() {
  PIN_GetLock(&lock_, 0);
  DoFlush();
  PIN_ReleaseLock(&lock_);
}

#endif // SEXAIN_MEM_ADDR_TRACE_H_
