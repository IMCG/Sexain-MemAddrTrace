// MemAddrStats.cpp
// Copyright (c) 2013 Jinglei Ren <jinglei.ren@stanzax.org>

#include <cerrno>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <fstream>
#include <string>
#include "mem_addr_parser.h"
#include "epoch_visitor.h"
#include "epoch_engine.h"

#define MEGA 1000000

using namespace std;

static inline int NumBuckets(int page_bits) {
  int buckets = 1 << (page_bits - CACHE_BLOCK_BITS);
  return buckets > 16 ? 16 : buckets;
}

int main(int argc, const char* argv[]) {
  if (argc < 6) {
    cerr << "Usage: " << argv[0]
        << " FILE [-e EPOCH_INTERVAL]... [-p PAGE_BITS]" << endl;
    return EINVAL;
  }

  const char* input = argv[1];
  vector<int> arg_epochs;
  vector<int> arg_pages;
  for (int i = 2; i < argc; ++i) {
    if (strcmp(argv[i], "-e") == 0) {
      if (++i < argc) arg_epochs.push_back(atoi(argv[i]));
      else cerr << "[Err] Wrong argument!" << endl;
    } else if (strcmp(argv[i], "-p") == 0) {
      if ( ++i < argc) arg_pages.push_back(atoi(argv[i]));
      else cerr << "[Err] Wrong argument!" << endl;
    } else {
      cerr << "[Err] Wrong argument: " << argv[i] << endl;
      return EINVAL;
    }
  }

  MemAddrParser parser(input);
  vector<DirtEpochEngine> engines;
  for (vector<int>::iterator it = arg_epochs.begin();
      it != arg_epochs.end(); ++it) {
    engines.push_back(*it);
  }

  vector< vector<PageDirtVisitor> > dirt_visitors;
  for (vector<int>::iterator it = arg_pages.begin();
      it != arg_pages.end(); ++it) {
    dirt_visitors.push_back(vector<PageDirtVisitor>(engines.size(), *it));
  }

  // Register visitors after they are stably allocated.
  for (vector< vector<PageDirtVisitor> >::iterator it = dirt_visitors.begin();
      it != dirt_visitors.end(); ++it) {
    for (unsigned int i = 0; i < engines.size(); ++i) {
      engines[i].AddVisitor(&(*it)[i]);
    }
  }
 
  MemRecord rec;
  while (parser.Next(&rec)) {
    for (vector<DirtEpochEngine>::iterator it = engines.begin();
        it != engines.end(); ++it) {
      it->Input(rec);
    }
  }

  for (unsigned int pi = 0; pi < arg_pages.size(); ++pi) {
    int buckets = NumBuckets(arg_pages[pi]);
    vector<double> epoch_ratios(buckets);
    vector<double> overall_dirts(buckets);
    vector<double> epochs(buckets);
    for (unsigned int ei = 0; ei < engines.size(); ++ei) {
      dirt_visitors[pi][ei].FillEpochDirts(epoch_ratios.data(), buckets);
      dirt_visitors[pi][ei].FillOverallDirts(overall_dirts.data(), buckets);
      dirt_visitors[pi][ei].FillEpochSpans(epochs.data(), buckets);

      string filename(input);
      filename.append("-").append(to_string(engines[ei].interval()));
      filename.append("-").append(to_string(arg_pages[pi])).append(".stats");
      ofstream fout(filename);
      fout << "# num_epochs=" << engines[ei].num_epochs() << endl;
      fout << "# epoch_interval=" << fixed
          << (double)engines[ei].overall_ins() / MEGA / engines[ei].num_epochs()
          << "M" << endl;
      fout << "# Epoch DR, CDF, Overall DR, Epoch Span" << endl;
      double left_sum = 0;
      for (int i = 0; i < buckets; ++i) {
        fout << (double)i / buckets << '\t'
            << left_sum << '\t'
            << overall_dirts[i] / dirt_visitors[pi][ei].page_blocks() << '\t'
            << epochs[i] / engines[ei].num_epochs() << endl;
        left_sum += epoch_ratios[i];
      }
      fout << 1 << '\t' << left_sum << endl;
    }
  }

  return 0;
}

