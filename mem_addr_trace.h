// mem_addr_trace.h
// Jinglei Ren <jinglei.ren@gmail.com>
// Dec. 20, 2013

#ifndef SEXAIN_MEM_ADDR_TRACE_H_
#define SEXAIN_MEM_ADDR_TRACE_H_

#include <stdio.h>
#include "pin.H"

class MemAddrTrace {
 public:
  MemAddrTrace(int buf_size, FILE* file); 
  ~MemAddrTrace();
 
  void Input(UINT32 ins_seq, VOID* addr, char op);
  void Flush();

  int buffer_size() const { return buffer_size_; }
  FILE* file() const { return file_; }

 private:
  void Output(unsigned int end);

  const unsigned int buffer_size_;
  FILE* file_;
  PIN_LOCK lock_;
  unsigned int end_;
  UINT32* ins_array_;
  VOID** addr_array_;
  char* op_array_;
  UINT32* ins_compressed_;
  VOID** addr_compressed_;
  char* op_compressed_;
};

MemAddrTrace::MemAddrTrace(int buf_size, FILE* file):
    buffer_size_(buf_size), file_(file), end_(0) {
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
void MemAddrTrace::Output(unsigned int end) {
  if (end > buffer_size_) {
    fprintf(stderr, "[Warn] MemAddrTrace::Output receives overflowed end: %d\n",
      end);
    end = end_;
  }
  fwrite(ins_array_, sizeof(UINT32), end, file_);
  fwrite(addr_array_, sizeof(VOID*), end, file_);
  fwrite(op_array_, sizeof(char), end, file_);
  end_ = 0;
}

void MemAddrTrace::Input(UINT32 ins_seq, VOID* addr, char op) {
  PIN_GetLock(&lock_, 0);
  if (end_ == buffer_size_) {
    Output(buffer_size_);
  }

  ins_array_[end_] = ins_seq;
  addr_array_[end_] = addr;
  op_array_[end_] = op;
  ++end_;
  PIN_ReleaseLock(&lock_);
}

void MemAddrTrace::Flush() {
  PIN_GetLock(&lock_, 0);
  Output(end_);
  PIN_ReleaseLock(&lock_);
}

#endif // SEXAIN_MEM_ADDR_TRACE_H_
