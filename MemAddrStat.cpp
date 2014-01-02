// MemAddrStat.cpp
// Copyright (c) Jinglei Ren <jinglei.ren@stanzax.org>

#include <cerrno>
#include <iostream>
#include <fstream>
#include "mem_addr_parser.h"

using std::endl;

int main(int argc, const char* argv[]) {
  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " FILE OUTPUT" << endl;
    return EINVAL;
  }

  MemAddrParser parser(argv[1]);
  std::ofstream output(argv[2]);

  MemRecord rec;
  while (parser.Next(&rec)) {
    output << rec.ins_seq << '\t' << rec.mem_addr << '\t' << rec.op << endl;
  }

  output.close();
  return 0;
}
