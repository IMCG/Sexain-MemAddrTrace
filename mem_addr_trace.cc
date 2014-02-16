// mem_addr_trace.cc
// Copyright (c) 2013 Jinglei Ren <jinglei.ren@stanzax.org>

#include "mem_addr_trace.h"

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

