// mem_addr_parser.h
// Copyright (c) 2014 Jinglei Ren <jinglei.ren@stanzax.org>

#ifndef SEXAIN_MEM_ADDR_PARSER_H_
#define SEXAIN_MEM_ADDR_PARSER_H_

#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include "zlib.h"

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

 private:
  bool Replenish();
  void Close();

  FILE* file_;
  uint32_t buf_size_;
  uint32_t ptr_bytes_;

  uint32_t next_;
  uint32_t* ins_array_;
  char* addr_array_;
  char* op_array_;
  void* ins_compressed_;
  void* addr_compressed_;
  void* op_compressed_;
};

MemAddrParser::MemAddrParser(const char* file) {
  file_ = fopen(file, "rb");
  if (!fread(&buf_size_, sizeof(buf_size_), 1, file_) ||
      !fread(&ptr_bytes_, sizeof(ptr_bytes_), 1, file_) ||
      buf_size_ > 0x10000000 || (ptr_bytes_ & 0x60) != 0) {
    fclose(file_);
    file_ = NULL;
    fprintf(stderr, "[Error] MemAddrParser init failed.\n");
    return;
  }
  ins_array_ = new uint32_t[buf_size_];
  addr_array_ = new char[buf_size_ * ptr_bytes_ / sizeof(char)];
  op_array_ = new char[buf_size_];
  ins_compressed_ = malloc(compressBound(sizeof(uint32_t) * buf_size_));
  addr_compressed_ = malloc(compressBound(ptr_bytes_ * buf_size_));
  op_compressed_ = malloc(compressBound(sizeof(char) * buf_size_));

  Replenish();
}

MemAddrParser::~MemAddrParser() {
  Close();
}

bool MemAddrParser::Replenish() {
  if (!file_) return false;
  uLong len;
  if (!fread(&len, sizeof(len), 1, file_) ||
      !fread(ins_compressed_, len, 1, file_) ||
      !fread(&len, sizeof(len), 1, file_) ||
      !fread(addr_compressed_, len, 1, file_) ||
      !fread(&len, sizeof(len), 1, file_) ||
      !fread(op_compressed_, len, 1, file_)) {
    fprintf(stderr, "MemAddrParser::Replenish() stops at %lu\n", ftell(file_));
    Close();
    return false;
  }
  
  return true;
}

void MemAddrParser::Close() {
  if (!file_) return;
  fclose(file_);
  file_ = NULL;
  delete[] ins_array_;
  delete[] addr_array_;
  delete[] op_array_;
  free(ins_compressed_);
  free(addr_compressed_);
  free(op_compressed_);
}

bool MemAddrParser::Next(MemRecord* rec) {
  return false;
}

#endif // SEXAIN_MEM_ADDR_PARSER_H_
