// MemAddrStat.cpp
// Copyright (c) Jinglei Ren <jinglei.ren@stanzax.org>

#include <cerrno>
#include <iostream>
#include "mem_addr_parser.h"

int main(int argc, const char* argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " FILE" << std::endl;
    return EINVAL;
  }

  MemAddrParser parser(argv[1]);
  MemRecord rec;
  while (parser.Next(&rec));
  return 0;
}
