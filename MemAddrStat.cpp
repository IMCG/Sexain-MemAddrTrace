// MemAddrStat.cpp
// Copyright (c) 2013 Jinglei Ren <jinglei.ren@stanzax.org>

#include <cerrno>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <string>
#include "mem_addr_parser.h"
#include "epoch_engine.h"

#define MEGA 1000000

using namespace std;

int main(int argc, const char* argv[]) {
  if (argc != 5) {
    cerr << "Usage: " << argv[0]
        << " FILE EPOCH_MEGA_INS PAGE_BITS NUM_BUCKETS" << endl;
    return EINVAL;
  }

  MemAddrParser parser(argv[1]);
  const uint64_t epoch_ins = atoll(argv[2]) * MEGA;
  const int page_bits = atoi(argv[3]);
  const int num_buckets = atoi(argv[4]);

  double* results = new double[num_buckets];
  for (int i = 0; i < num_buckets; ++i) {
    results[i] = 0;
  }

  PageDirtyRatioVisitor pdr_visitor(page_bits);
  EpochEngine engine(epoch_ins);
  engine.AddVisitor(&pdr_visitor);

  MemRecord rec;
  while (parser.Next(&rec)) {
    engine.Input(rec);
  }

  cout << "# num_epochs=" << engine.num_epochs() << endl;
  BUG_ON(pdr_visitor.Fillout(results, num_buckets) != engine.num_epochs());
  double left_sum = 0;
  cout << 0 << '\t' << left_sum << endl;
  for (int i = 0; i < num_buckets; ++i) {
    left_sum += results[i];
    cout << (double)(i + 1) / num_buckets << '\t' << left_sum << endl;
  }

  delete[] results;
  return 0;
}
