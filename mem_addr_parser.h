// mem_addr_parser.h
// Copyright (c) 2014 Jinglei Ren <jinglei.ren@stanzax.org>

#ifndef SEXAIN_MEM_ADDR_PARSER_H_
#define SEXAIN_MEM_ADDR_PARSER_H_

#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cassert>
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
  uint32_t buf_len_;
  uint32_t ptr_bytes_;

  uint32_t next_;
  uint32_t* ins_array_;
  char* addr_array_;
  char* op_array_;
  Bytef* ins_comp_;
  Bytef* addr_comp_;
  Bytef* op_comp_;
};

MemAddrParser::MemAddrParser(const char* file) {
  file_ = fopen(file, "rb");
  if (!fread(&buf_len_, sizeof(buf_len_), 1, file_) ||
      !fread(&ptr_bytes_, sizeof(ptr_bytes_), 1, file_) ||
      buf_len_ > 0x10000000 || (ptr_bytes_ & 0x60) != 0) {
    fclose(file_);
    file_ = NULL;
    fprintf(stderr, "[Error] MemAddrParser init failed.\n");
    return;
  }
  ins_array_ = new uint32_t[buf_len_];
  addr_array_ = new char[buf_len_ * ptr_bytes_ / sizeof(char)];
  op_array_ = new char[buf_len_];
  ins_comp_ = (Bytef*)malloc(compressBound(sizeof(uint32_t) * buf_len_));
  addr_comp_ = (Bytef*)malloc(compressBound(ptr_bytes_ * buf_len_));
  op_comp_ = (Bytef*)malloc(compressBound(sizeof(char) * buf_len_));

  Replenish();
}

MemAddrParser::~MemAddrParser() {
  Close();
}

bool MemAddrParser::Replenish() {
  next_ = 0;
  if (!file_) return false;
  uLong ins_len = 0, addr_len = 0, op_len = 0;
  if (!fread(&ins_len, sizeof(ins_len), 1, file_)) { 
    fprintf(stderr, "MemAddrParser::Replenish() stops at %lu\n", ftell(file_));
    Close();
    return false;
  }
  if (!fread(ins_comp_, ins_len, 1, file_) ||
      !fread(&addr_len, sizeof(addr_len), 1, file_) ||
      !fread(addr_comp_, addr_len, 1, file_) ||
      !fread(&op_len, sizeof(op_len), 1, file_) ||
      !fread(op_comp_, op_len, 1, file_)) {
    fprintf(stderr, "[Error] MemAddrParser::Replenish() unexpected end: "
        "ins_len=%lu, addr_len=%lu, op_len=%lu\n", ins_len, addr_len, op_len);
    fprintf(stderr, "MemAddrParser::Replenish() stops at %lu\n", ftell(file_));
    Close();
    return false;
  }
  
  uLong len;
  if ((len = buf_len_ * sizeof(uint32_t),
      uncompress((Bytef*)ins_array_, &len, ins_comp_, ins_len) != Z_OK) ||
      (len = buf_len_ * ptr_bytes_,
      uncompress((Bytef*)addr_array_, &len, addr_comp_, addr_len) != Z_OK) ||
      (len = buf_len_ * sizeof(char),
      uncompress((Bytef*)op_array_, &len, op_comp_, op_len) != Z_OK)) {
    fprintf(stderr, "MemAddrParser::Replenish() failed on uncompression.\n");
    Close();
    return false;
  }

  if (len / sizeof(char) != buf_len_) {
    buf_len_ = len / sizeof(char);
    fprintf(stderr, "MemAddrParser::Replenish() partial buffer: %u\n",
        buf_len_);
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
  free(ins_comp_);
  free(addr_comp_);
  free(op_comp_);
}

bool MemAddrParser::Next(MemRecord* rec) {
  if (next_ == buf_len_ && !Replenish()) {
    return false;
  }

  rec->ins_seq = ins_array_[next_];
  rec->mem_addr = *((uint64_t*)
      (addr_array_ + ptr_bytes_ * next_ / sizeof(char)));
  rec->op = op_array_[next_];
  assert(rec->op == 'R' || rec->op == 'W');
  return ++next_;
}

#endif // SEXAIN_MEM_ADDR_PARSER_H_
