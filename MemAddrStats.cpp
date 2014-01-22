// MemAddrStats.cpp
// Copyright (c) 2013 Jinglei Ren <jinglei.ren@stanzax.org>

#include <cerrno>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <string>
#include "mem_addr_parser.h"
#include "epoch_visitor.h"
#include "epoch_engine.h"

#define INS_BASE 10
#define BIT_STEP 2
#define MEGA 1000000

using namespace std;

static inline int NumBuckets(int page_bits) {
  int buckets = 1 << (page_bits - CACHE_BLOCK_BITS);
  return buckets > 16 ? 16 : buckets;
}

int main(int argc, const char* argv[]) {
  if (argc != 6) {
    cerr << "Usage: " << argv[0]
        << " FILE MIN_MEGA_INS MAX_MEGA_INS"
        << " MIN_PAGE_BITS MAX_PAGE_BITS" << endl;
    return EINVAL;
  }

  const char* input = argv[1];
  MemAddrParser parser(input);
  const uint64_t min_ins = atoi(argv[2]) * MEGA;
  const uint64_t max_ins = atoi(argv[3]) * MEGA;
  const int min_bits = atoi(argv[4]);
  const int max_bits = atoi(argv[5]);

  vector<EpochEngine> engines;
  for (uint64_t ins = min_ins; ins <= max_ins; ins *= INS_BASE) {
    engines.push_back(ins);
  }

  vector< vector<PageDirtVisitor> > dirt_visitors;
  for (int bits = min_bits; bits <= max_bits; bits += BIT_STEP) {
    dirt_visitors.push_back(vector<PageDirtVisitor>(engines.size(), bits));
  }

  vector<BlockCountVisitor> bc_visitors(engines.size());

  // Register visitors after they are stably allocated.
  for (vector< vector<PageDirtVisitor> >::iterator it = dirt_visitors.begin();
      it != dirt_visitors.end(); ++it) {
    for (unsigned int i = 0; i < engines.size(); ++i) {
      engines[i].AddVisitor(&(*it)[i]);
    }
  }
  for (unsigned int i = 0; i < engines.size(); ++i) {
    engines[i].AddVisitor(&bc_visitors[i]);
  }
 
  MemRecord rec;
  while (parser.Next(&rec)) {
    for (vector<EpochEngine>::iterator it = engines.begin();
        it != engines.end(); ++it) {
      it->Input(rec);
    }
  }

  double* epoch_ratios = new double[NumBuckets(max_bits)];
  double* overall_dirts = new double[NumBuckets(max_bits)];
  double* epochs = new double[NumBuckets(max_bits)];
  for (int bits = min_bits; bits <= max_bits; bits += BIT_STEP) {
    int buckets = NumBuckets(bits);
    int bi = (bits - min_bits) / BIT_STEP; 
    for (unsigned int ei = 0; ei < engines.size(); ++ei) {
      dirt_visitors[bi][ei].FillEpochDirts(epoch_ratios, buckets);
      dirt_visitors[bi][ei].FillOverallDirts(overall_dirts, buckets);
      dirt_visitors[bi][ei].FillEpochSpans(epochs, buckets);

      string filename(input);
      filename.append("-").append(to_string(engines[ei].epoch_ins() / MEGA));
      filename.append("-").append(to_string(bits)).append(".stats");
      ofstream fout(filename);
      fout << "# num_epochs=" << engines[ei].num_epochs() << endl;
      fout << "# dirty_throughput="
          << (double)bc_visitors[ei].count() / engines[ei].num_epochs() << endl;
      fout << "# Epoch DR, CDF, Overall DR, Epoch Span" << endl;
      double left_sum = 0;
      for (int i = 0; i < buckets; ++i) {
        fout << (double)i / buckets << '\t'
            << left_sum << '\t'
            << overall_dirts[i] / dirt_visitors[bi][ei].page_blocks() << '\t'
            << epochs[i] / engines[ei].num_epochs() << endl;
        left_sum += epoch_ratios[i];
      }
      fout << 1 << '\t' << left_sum << endl;
    }
  }

  delete[] epoch_ratios;
  delete[] epochs;
  return 0;
}