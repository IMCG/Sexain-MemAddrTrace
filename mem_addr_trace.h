// mem_addr_trace.h
// Copyright (c) 2013 Jinglei Ren <jinglei.ren@stanzax.org>

#ifndef SEXAIN_MEM_ADDR_TRACE_H_
#define SEXAIN_MEM_ADDR_TRACE_H_

#include <stdio.h>
#include "pin.H"
#include "zlib.h"

class MemAddrTrace {
 public:
  MemAddrTrace(UINT32 buf_size, const char* file, UINT32 max_size_mb); 
  ~MemAddrTrace();
 
  void Input(UINT32 ins_seq, VOID* addr, char op);
  void Flush();

  UINT32 buffer_size() const { return buf_len_; }
  FILE* file() const { return file_; }

  UINT64 file_size() const { return file_size_; }
  void set_file_size(UINT32 mb) { file_size_ = (UINT64)mb << 20; }

 private:
  void DoFlush();

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
void MemAddrTrace::DoFlush() {
  if (end_ > buf_len_) {
    fprintf(stderr, "[Warn] MemAddrTrace::DoFlush overflows with end = %d\n",
      end_);
    end_ = buf_len_;
  }
  if ((UINT64)ftell(file_) < file_size_) {
    uLong len;

    len = compressBound(sizeof(UINT32) * end_);
    if (compress((Bytef*)ins_compressed_, &len,
        (Bytef*)ins_array_, sizeof(UINT32) * end_) == Z_OK) {
      fwrite(&len, sizeof(len), 1, file_);
      fwrite(ins_compressed_, len, 1, file_);
    } else {
      fprintf(stderr, "[Warn] MemAddrTrace::DoFlush failed on compression.\n");
    }

    len = compressBound(sizeof(VOID*) * end_);
    if (compress((Bytef*)addr_compressed_, &len,
        (Bytef*)addr_array_, sizeof(VOID*) * end_) == Z_OK) {
      fwrite(&len, sizeof(len), 1, file_);
      fwrite(addr_compressed_, len, 1, file_);
    } else {
      fprintf(stderr, "[Warn] MemAddrTrace::DoFlush failed on compression.\n");
    }

    len = compressBound(sizeof(char) * end_);
    if (compress((Bytef*)op_compressed_, &len,
        (Bytef*)op_array_, sizeof(char) * end_) == Z_OK) {
      fwrite(&len, sizeof(len), 1, file_);
      fwrite(op_compressed_, len, 1, file_);
    } else {
      fprintf(stderr, "[Warn] MemAddrTrace::DoFlush failed on compression.\n");
    }
  }
  end_ = 0;
}

void MemAddrTrace::Input(UINT32 ins_seq, VOID* addr, char op) {
  PIN_GetLock(&lock_, 0);
  if (end_ == buf_len_) {
    DoFlush();
  }

  ins_array_[end_] = ins_seq;
  addr_array_[end_] = addr;
  op_array_[end_] = op;
  ++end_;
  PIN_ReleaseLock(&lock_);
}

void MemAddrTrace::Flush() {
  PIN_GetLock(&lock_, 0);
  DoFlush();
  PIN_ReleaseLock(&lock_);
}

#endif // SEXAIN_MEM_ADDR_TRACE_H_
