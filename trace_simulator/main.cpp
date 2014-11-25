//
//  main.cpp
//  TraceParser
//
//  Created by Ren Jinglei on 11/23/14.
//  Copyright (c) 2014 Ren Jinglei <jinglei@ren.systems>.
//

#include <vector>
#include <iostream>
#include <fstream>
#include <cassert>

#include "trace_simulator.h"

#define M (1000000)

using namespace std;

void PrintStats(const TraceSimulator *ts) {
  cout << ts->block_bits() << '\t';
  cout << ts->BasicStats().epoch_num() << '\t';
  cout << ts->BasicStats().nvm_through() << '\t';
  cout << ts->BasicStats().dram_through() << endl;
}

int main(int argc, const char * argv[]) {
  if (argc != 5) {
    cerr << "Wrong # arguments: " << argc << endl;
    cerr << "USAGE: " << argv[0] << " FILE_NAME BUF_LEN INS_BEGIN INS_NUM" << endl;
    return EINTR;
  }
  
  const char *filename = argv[1];
  const int buf_len = atoi(argv[2]);
  const long long ins_begin = atoll(argv[3]) * M;
  const long long ins_num = atoll(argv[4]) * M;
  
  ifstream fin(filename);
  if (!fin.is_open()) {
    cerr << "Failed to open " << filename << endl;
    return ENFILE;
  }
  
  vector<TraceSimulator *> simulators;
  for (int i = 3; i < 5; ++i) {
    simulators.push_back(new TraceSimulator(buf_len, 2  * i, false));
  }
  for (int i = 5; i < 9; ++i) {
    simulators.push_back(new TraceSimulator(buf_len, 2  * i, true));
  }
  
  long long ins_total = 0;
  
  while (!fin.eof()) {
    int is_read;
    uint64_t addr;
    int ins_inc;
    
    fin >> is_read >> addr >> ins_inc;
    
    ins_total += ins_inc;
    if (is_read) continue;
    
    if (ins_total >= ins_begin && ins_total - ins_begin < ins_num) {
      for (TraceSimulator *ts : simulators) {
        ts->Put(addr, ins_total - ins_begin);
      }
    }
  }
  
  for (TraceSimulator *ts : simulators) {
    PrintStats(ts);
  }
  
  return 0;
}
