//
//  trace_simulator.h
//
//  Created by Ren Jinglei on 11/24/14.
//  Copyright (c) 2014 Ren Jinglei <jinglei@ren.systems>.
//

#ifndef TraceSimulator_trace_simulator_h
#define TraceSimulator_trace_simulator_h

#include <unordered_map>
#include "stats.h"
#include "index_queue.h"

typedef enum {
  FREE,
  CLEAN,
  DIRTY,
  HIDDEN,
} State;

struct Tag {
  uint64_t value;
  Tag(uint64_t addr, int bits) : value(addr >> bits) { }
  bool operator==(const Tag &other) { return value == other.value; }
};

static const Tag kInvalidTag(uint64_t(-1), 0);

struct IndexEntry {
  Tag tag;
  State state;
  
  IndexEntry() : tag(kInvalidTag), state(FREE) { }
};

struct TagHash {
  inline std::size_t operator()(const Tag &tag) const {
    std::hash<uint64_t> hasher;
    return hasher(tag.value);
  }
};

struct TagEqual {
  inline bool operator()(const Tag &x, const Tag &y) const {
    return x.value == y.value;
  }
};

class TraceSimulator {
public:
  TraceSimulator(int buffer_len, int block_bits, bool has_dram) :
      buffer_slots_(buffer_len),
      block_bits_(block_bits),
      free_queue_(buffer_slots_),
      clean_queue_(buffer_slots_),
      dirty_queue_(buffer_slots_),
      hidden_queue_(buffer_slots_) {
    for (int i = 0; i < buffer_len; ++i) {
      free_queue_.PushBack(i);
    }
    stats_.push_back(Stats(buffer_len, 1 << block_bits, has_dram));
    CheckBufferNum();
  }
  
  int block_bits() const { return block_bits_; }
  
  void Put(uint64_t addr, size_t ins_num) {
    Tag tag(addr, block_bits_);
    IndexIterator it = buffer_index_.find(tag);
    if (it != buffer_index_.end()) {
      Transite(it->second);
    } else {
      if (!free_queue_.Empty()) {
        Add(tag);
      } else if (!clean_queue_.Empty()) {
        Revoke();
        Add(tag);
      } else {
        NewEpoch();
        Put(addr, ins_num);
      }
    }
  }
  
  void RegisterStats(Stats &stats) { stats_.push_back(stats); }
  Stats BasicStats() const { return stats_.at(0); }
  
private:
  typedef std::unordered_map<Tag, int, TagHash, TagEqual>::iterator
      IndexIterator;
  
  TraceSimulator(const TraceSimulator &ts) : TraceSimulator(0, 0, false) {
    assert(false);
  }
  
  void Add(Tag tag) {
    const int index = free_queue_.PopFront();
    assert(FREE == buffer_slots_.at(index).data.state);
    
    dirty_queue_.PushBack(index);
    buffer_slots_.at(index).data.state = DIRTY;
    buffer_slots_.at(index).data.tag = tag;
    
    buffer_index_[tag] = index;
    
    for (Stats &s : stats_) {
      s.OnCopy();
    }
    CheckBufferNum();
  }
  
  void Revoke() {
    const int index = clean_queue_.PopFront();
    assert(CLEAN == buffer_slots_.at(index).data.state);
    
    free_queue_.PushBack(index);
    buffer_slots_.at(index).data.state = FREE;
    
    size_t num = buffer_index_.erase(buffer_slots_.at(index).data.tag);
    assert(1 == num);
    
    for (Stats &s : stats_) {
      s.OnCopy(2);
    }
    CheckBufferNum();
  }
  
  void Transite(int index) {
    State from = buffer_slots_.at(index).data.state;
    switch (from) {
      case DIRTY:
        dirty_queue_.Remove(index);
        dirty_queue_.PushBack(index);
        break;
      case HIDDEN:
        break;
      case CLEAN:
        clean_queue_.Remove(index);
        hidden_queue_.PushBack(index);
        buffer_slots_.at(index).data.state = HIDDEN;
        break;
      default:
        assert(false);
        break;
    }
    
    for (Stats &s : stats_) {
      s.OnHit();
    }
    CheckBufferNum();
  }
  
  void NewEpoch() {
    assert(free_queue_.Empty());
    size_t to_ckpt = buffer_slots_.size() - clean_queue_.length();
    
    while (!dirty_queue_.Empty()) {
      int index = dirty_queue_.PopFront();
      assert(DIRTY == buffer_slots_.at(index).data.state);
      
      clean_queue_.PushBack(index);
      buffer_slots_.at(index).data.state = CLEAN;
    }
    
    while (!hidden_queue_.Empty()) {
      int index = hidden_queue_.PopFront();
      assert(HIDDEN == buffer_slots_.at(index).data.state);
      
      free_queue_.PushBack(index);
      buffer_slots_.at(index).data.state = FREE;
      
      size_t num = buffer_index_.erase(buffer_slots_.at(index).data.tag);
      assert(1 == num);
    }
    
    for (Stats &s : stats_) {
      s.OnEpoch(to_ckpt);
    }
    CheckBufferNum();
  }
  
  void CheckBufferNum() {
    if (free_queue_.length() + clean_queue_.length() + dirty_queue_.length() +
        hidden_queue_.length() != buffer_slots_.size()) {
      std::cerr << "Mismatch in buffer numbers: total=" << buffer_slots_.size();
      std::cerr << " free=" << free_queue_.length();
      std::cerr << " clean=" << clean_queue_.length();
      std::cerr << " dirty=" << dirty_queue_.length();
      std::cerr << " hidden=" << hidden_queue_.length() << std::endl;
      assert(false);
    }
  }
  
  const int block_bits_;
  
  std::vector<IndexNode<IndexEntry>> buffer_slots_;
  std::unordered_map<Tag, int, TagHash, TagEqual> buffer_index_;
  
  IndexQueue<IndexEntry> free_queue_;
  IndexQueue<IndexEntry> clean_queue_;
  IndexQueue<IndexEntry> dirty_queue_;
  IndexQueue<IndexEntry> hidden_queue_;
  
  std::vector<Stats> stats_;
};

#endif
