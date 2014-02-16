// mem_addr_parser.h
// Copyright (c) 2014 Jinglei Ren <jinglei.ren@stanzax.org>

#ifndef SEXAIN_MEM_ADDR_PARSER_H_
#define SEXAIN_MEM_ADDR_PARSER_H_

#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <iostream>
#include "zlib.h"

#ifdef NDEBUG
#define BUG_ON(v) do { \
  if (v) std::cerr << "[Warn]" << __FILE__ << ", " << __LINE__ << std::endl; \
while(0)
#else
#define BUG_ON(v) (assert(!(v)))
#endif

struct MemRecord {
  uint64_t ins_seq;
  uint64_t mem_addr;
  char op;
};

class MemAddrParser {
 public:
  MemAddrParser(const char* file);
  ~MemAddrParser();

  bool Next(MemRecord* rec);
  uint32_t buffer_count() const { return buffer_count_; }

 private:
  bool Replenish();
  void Close();

  FILE* file_;
  uint32_t buffer_count_;
  uint32_t ptr_bytes_;

  uint32_t i_next_;
  uint32_t i_limit_;
  uint32_t* ins_array_;
  char* addr_array_;
  char* op_array_;
  Bytef* ins_comp_;
  Bytef* addr_comp_;
  Bytef* op_comp_;

  uint64_t base_ins_;
  uint64_t base_step_;
  uint64_t last_ins_;
};

inline MemAddrParser::~MemAddrParser() {
  Close();
}

inline void MemAddrParser::Close() {
  if (!file_) return;
  fclose(file_);
  file_ = NULL;
  delete[] ins_array_;
  delete[] addr_array_;
  delete[] op_array_;
  free(ins_comp_);
  free(addr_comp_);
  free(op_comp_);
}

#endif // SEXAIN_MEM_ADDR_PARSER_H_

