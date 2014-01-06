// mem_addr_parser.h
// Copyright (c) 2014 Jinglei Ren <jinglei.ren@stanzax.org>

#ifndef SEXAIN_MEM_ADDR_PARSER_H_
#define SEXAIN_MEM_ADDR_PARSER_H_

#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cassert>
#include "zlib.h"

#ifdef NDEBUG
#define BUG_ON(v) (if (v) fprintf(stderr, "[Warn] %s, %d", __FILE__, __LINE__))
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
};

MemAddrParser::MemAddrParser(const char* file) {
  file_ = fopen(file, "rb");
  if (fread(&buffer_count_, sizeof(buffer_count_), 1, file_) != 1 ||
      fread(&ptr_bytes_, sizeof(ptr_bytes_), 1, file_) != 1 ||
      buffer_count() > 0x10000000 || (ptr_bytes_ & 0x60) != 0) {
    fclose(file_);
    file_ = NULL;
    fprintf(stderr, "[Error] MemAddrParser init failed.\n");
    return;
  }
  ins_array_ = new uint32_t[buffer_count()];
  addr_array_ = new char[buffer_count() * ptr_bytes_ / sizeof(char)];
  op_array_ = new char[buffer_count()];
  ins_comp_ = (Bytef*)malloc(compressBound(sizeof(uint32_t) * buffer_count()));
  addr_comp_ = (Bytef*)malloc(compressBound(ptr_bytes_ * buffer_count()));
  op_comp_ = (Bytef*)malloc(compressBound(sizeof(char) * buffer_count()));
  i_next_ = 0;
  i_limit_ = buffer_count();

  Replenish();
}

MemAddrParser::~MemAddrParser() {
  Close();
}

bool MemAddrParser::Replenish() {
  if (!file_) return false;
  uLong ins_len = 0, addr_len = 0, op_len = 0;
  if (fread(&ins_len, sizeof(ins_len), 1, file_) != 1) {
    assert(ftell(file_) == (fseek(file_, 0, SEEK_END), ftell(file_)));
    Close();
    return false;
  }
  BUG_ON(fread(ins_comp_, 1, ins_len, file_) != ins_len);

  BUG_ON(fread(&addr_len, sizeof(addr_len), 1, file_) != 1);
  BUG_ON(fread(addr_comp_, 1, addr_len, file_) != addr_len);

  BUG_ON(fread(&op_len, sizeof(op_len), 1, file_) != 1);
  BUG_ON(fread(op_comp_, 1, op_len, file_) != op_len);

  uLong len;
  int ret;
  len = buffer_count() * sizeof(uint32_t);
  ret = uncompress((Bytef*)ins_array_, &len, ins_comp_, ins_len);
  if (ret != Z_OK) {
    fprintf(stderr, "[Warn] MemAddrParser::Replenish(): for ins_array_ "
        "uncompress() failed: %d, ftell()=%lu\n", ret, ftell(file_));  
  }
  len = buffer_count() * ptr_bytes_;
  ret = uncompress((Bytef*)addr_array_, &len, addr_comp_, addr_len) != Z_OK;
  if (ret != Z_OK) {
    fprintf(stderr, "[Warn] MemAddrParser::Replenish(): for addr_array_ "
        "uncompress() failed: %d, ftell()=%lu\n", ret, ftell(file_));  
  }
  len = buffer_count() * sizeof(char);
  ret = uncompress((Bytef*)op_array_, &len, op_comp_, op_len) != Z_OK;
  if (ret != Z_OK) {
    fprintf(stderr, "[Warn] MemAddrParser::Replenish(): for op_array_ "
        "uncompress() failed: %d, ftell()=%lu\n", ret, ftell(file_));  
  }

  i_next_ = 0;
  if (len / sizeof(char) != i_limit_) {
    i_limit_ = len / sizeof(char);
    fprintf(stderr, "MemAddrParser::Replenish() partial buffer: %u\n",
        i_limit_);
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
  if (!file_ || (i_next_ == i_limit_ && !Replenish())) {
    return false;
  }

  rec->ins_seq = ins_array_[i_next_];
  rec->mem_addr = *((uint64_t*)
      (addr_array_ + ptr_bytes_ * i_next_ / sizeof(char)));
  rec->op = op_array_[i_next_];
  BUG_ON(rec->op != 'R' && rec->op != 'W');
  return ++i_next_;
}

#endif // SEXAIN_MEM_ADDR_PARSER_H_
