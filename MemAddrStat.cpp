// MemAddrStat.cpp
// Copyright (c) Jinglei Ren <jinglei.ren@stanzax.org>

#include <cerrno>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <string>
#include "mem_addr_parser.h"
#include "mem_addr_stat.h"

using namespace std;

int main(int argc, const char* argv[]) {
  if (argc != 5) {
    cerr << "Usage: " << argv[0]
        << " FILE NUM_EPOCH_INS PAGE_BITS NUM_BUCKETS" << endl;
    return EINVAL;
  }

  MemAddrParser parser(argv[1]);
  //ofstream output(string(argv[1]).append(".stat").c_str());
  const uint64_t epoch_ins = atoll(argv[2]);
  const int page_bits = atoi(argv[3]);
  const int num_buckets = atoi(argv[4]);

  double* row = new double[num_buckets];
  double* sum = new double[num_buckets];
  for (int i = 0; i < num_buckets; ++i) {
    sum[i] = 0;
  }

  MemAddrStat stat(page_bits);
  MemRecord rec;
  uint64_t num_ins = epoch_ins;
  while (parser.Next(&rec)) {
    if (rec.ins_seq >= num_ins) {
      stat.Fillout(row, num_buckets);
      for (int i = 0; i < num_buckets; ++i) {
        sum[i] += row[i];
      }
      num_ins += epoch_ins;  
      stat.Clear();
    }
    if (rec.op == 'W') stat.Input(rec.mem_addr);
  }

  uint64_t num_epochs = num_ins / epoch_ins - 1;
  cout << "Num. Epochs=" << num_epochs << endl;
  cout << 0;
  for (int i = 0; i < num_buckets; ++i) {
    cout << '\t' << sum[i] / num_epochs;
  }
  cout << endl;

  delete[] row;
  delete[] sum;
  //output.close();
  return 0;
}
