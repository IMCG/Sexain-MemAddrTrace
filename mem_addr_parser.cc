// mem_addr_parser.cc
// Copyright (c) 2014 Jinglei Ren <jinglei.ren@stanzax.org>

#include "mem_addr_parser.h"

MemAddrParser::MemAddrParser(const char* file) {
  file_ = fopen(file, "rb");
  if (fread(&buffer_count_, sizeof(buffer_count_), 1, file_) != 1 ||
      fread(&ptr_bytes_, sizeof(ptr_bytes_), 1, file_) != 1 ||
      buffer_count() > 0x10000000 || (ptr_bytes_ & 0x60) != 0) {
    fclose(file_);
    file_ = NULL;
    std::cerr << "[Error] MemAddrParser init failed." << std::endl;
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

  base_ins_ = 0;
  base_step_ = (uint64_t)1 << (sizeof(ins_array_[0]) * 8);
  last_ins_ = 0;

  Replenish();
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
  len = buffer_count() * sizeof(uint32_t);
  BUG_ON(uncompress((Bytef*)ins_array_, &len, ins_comp_, ins_len) != Z_OK);

  len = buffer_count() * ptr_bytes_;
  BUG_ON(uncompress((Bytef*)addr_array_, &len, addr_comp_, addr_len) != Z_OK);

  len = buffer_count() * sizeof(char);
  BUG_ON(uncompress((Bytef*)op_array_, &len, op_comp_, op_len) != Z_OK);

  i_next_ = 0;
  if (len / sizeof(char) != i_limit_) {
    i_limit_ = len / sizeof(char);
  }
  return true;
}

bool MemAddrParser::Next(MemRecord* rec) {
  if (!file_ || (i_next_ == i_limit_ && !Replenish())) {
    return false;
  }

  rec->ins_seq = ins_array_[i_next_] + base_ins_;
  if (rec->ins_seq < last_ins_) {
    assert(last_ins_ - rec->ins_seq > (base_step_ >> 3));
    rec->ins_seq += base_step_;
    base_ins_ += base_step_;
  }
  last_ins_ = rec->ins_seq;
 
  rec->mem_addr = *((uint64_t*)
      (addr_array_ + ptr_bytes_ * i_next_ / sizeof(char)));
  rec->op = op_array_[i_next_];
  BUG_ON(rec->op != 'R' && rec->op != 'W');

#ifdef STDOUT
  std::cout << rec->ins_seq << '\t' << rec->mem_addr << '\t'
      << rec->op << std::endl;
#endif
  return ++i_next_;
}

