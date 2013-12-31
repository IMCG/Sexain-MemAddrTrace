// mem_addr_trace.h
// Jinglei Ren <jinglei.ren@gmail.com>
// Dec. 20, 2013

#ifndef SEXAIN_MEM_ADDR_TRACE_H_
#define SEXAIN_MEM_ADDR_TRACE_H_

#include <stdio.h>
#include "pin.H"
#include "zlib.h"

class MemAddrTrace {
 public:
  MemAddrTrace(UINT32 buf_size, FILE* file, UINT32 file_size_mb); 
  ~MemAddrTrace();
 
  void Input(UINT32 ins_seq, VOID* addr, char op);
  void Flush();

  UINT32 buffer_size() const { return buffer_size_; }
  FILE* file() const { return file_; }

  UINT64 file_size() const { return file_size_; }
  void set_file_size(UINT32 mb) { file_size_ = (UINT64)mb << 20; }

 private:
  void DoFlush();

  const UINT32 buffer_size_;
  FILE* file_;
  UINT64 file_size_; // max file size
  PIN_LOCK lock_;
  UINT32 end_;
  UINT32* ins_array_;
  VOID** addr_array_;
  char* op_array_;
  UINT32* ins_compressed_;
  VOID** addr_compressed_;
  char* op_compressed_;
};

MemAddrTrace::MemAddrTrace(UINT32 buf_size, FILE* file, UINT32 file_size_mb):
    buffer_size_(buf_size), file_(file), end_(0) {
  set_file_size(file_size_mb);
  PIN_InitLock(&lock_);
  ins_array_ = new UINT32[buffer_size_];
  addr_array_ = new VOID*[buffer_size_];
  op_array_ = new char[buffer_size_];
  ins_compressed_ = new UINT32[buffer_size_];
  addr_compressed_ = new VOID*[buffer_size_];
  op_compressed_ = new char[buffer_size_];
}

MemAddrTrace::~MemAddrTrace() {
  fflush(file_);
  delete ins_array_;
  delete addr_array_;
  delete op_array_;
  delete ins_compressed_;
  delete addr_compressed_;
  delete op_compressed_;
}

// Should be protected by lock_
void MemAddrTrace::DoFlush() {
  if (end_ > buffer_size_) {
    fprintf(stderr, "[Warn] MemAddrTrace::DoFlush overflows with end = %d\n",
      end_);
    end_ = buffer_size_;
  }
  if ((UINT64)ftell(file_) < file_size_) {
    uLong buf_len;

    buf_len = sizeof(UINT32) * end_;
    if (compress((Bytef*)ins_compressed_, &buf_len,
        (Bytef*)ins_array_, buf_len) == Z_OK) {
      fwrite(&buf_len, sizeof(buf_len), 1, file_);
      fwrite(ins_compressed_, buf_len, 1, file_);
    } else {
      fprintf(stderr, "[Warn] MemAddrTrace::DoFlush failed on compression.\n");
    }

    buf_len = sizeof(VOID*) * end_;
    if (compress((Bytef*)addr_compressed_, &buf_len,
        (Bytef*)addr_array_, buf_len) == Z_OK) {
      fwrite(&buf_len, sizeof(buf_len), 1, file_);
      fwrite(addr_compressed_, buf_len, 1, file_);
    } else {
      fprintf(stderr, "[Warn] MemAddrTrace::DoFlush failed on compression.\n");
    }

    buf_len = sizeof(char) * end_;
    if (compress((Bytef*)op_compressed_, &buf_len,
        (Bytef*)op_array_, buf_len) == Z_OK) {
      fwrite(&buf_len, sizeof(buf_len), 1, file_);
      fwrite(op_compressed_, buf_len, 1, file_);
    } else {
      fprintf(stderr, "[Warn] MemAddrTrace::DoFlush failed on compression.\n");
    }
  }
  end_ = 0;
}

void MemAddrTrace::Input(UINT32 ins_seq, VOID* addr, char op) {
  PIN_GetLock(&lock_, 0);
  if (end_ == buffer_size_) {
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
