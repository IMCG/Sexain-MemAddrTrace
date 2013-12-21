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
 
  void Input(UINT64 time, VOID* addr, char op);

  int buffer_size() const { return buffer_size_; }
  FILE* file() const { return file_; }

 private:
  const int buffer_size_;
  FILE* file_;
  unsigned int index_;
  UINT64* time_array_;
  VOID** addr_array_;
  char* op_array_;
  UINT64* time_compressed_;
  VOID** addr_compressed_;
  char* op_compressed_;
};

MemAddrTrace::MemAddrTrace(int buf_size, FILE* file):
    buffer_size_(buf_size), file_(file), index_(0) {
  time_array_ = new UINT64[buffer_size_];
  addr_array_ = new VOID*[buffer_size_];
  op_array_ = new char[buffer_size_];
  time_compressed_ = new UINT64[buffer_size_];
  addr_compressed_ = new VOID*[buffer_size_];
  op_compressed_ = new char[buffer_size_];
}

MemAddrTrace::~MemAddrTrace() {
  fflush(file_);
  delete time_array_;
  delete addr_array_;
  delete op_array_;
  delete time_compressed_;
  delete addr_compressed_;
  delete op_compressed_;
}

void MemAddrTrace::Input(UINT64 time, VOID* addr, char op) {
  
}

#endif // SEXAIN_MEM_ADDR_TRACE_H_
