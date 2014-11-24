//
// index_queue.h
// TraceSimulator
//
// Copyright (c) 2014 Jinglei Ren <jinglei@ren.systems>.
//

#ifndef TraceSimulator_index_queue_h
#define TraceSimulator_index_queue_h

#include <cerrno>
#include <cassert>
#include <vector>
#include <algorithm>

template <typename V>
struct IndexNode {
  V value;
  int prev;
  int next;

  IndexNode() {
    prev = next = -EINVAL;
  }
  
  IndexNode(V v) : IndexNode() {
    value = v;
  }
};

class QueueVisitor {
public:
  virtual void Visit(int i) = 0;
};

template <typename V>
class IndexQueue {
public:
  IndexQueue(std::vector<IndexNode<V>> &arr);
  int Front() const { return head_.prev; }
  int Back() const { return head_.next; }
  bool Empty() const;
  void Remove(int i);
  int PopFront();
  void PushBack(int i);
  
  int Accept(QueueVisitor* visitor);
  int length() const { return length_; }
private:
  IndexNode<V> &FrontNode();
  IndexNode<V> &BackNode();
  void SetFront(int i) { head_.prev = i; }
  void SetBack(int i) { head_.next = i; }
  
  IndexNode<V> head_;
  std::vector<IndexNode<V>> &array_;
  int length_;
};

template <typename V>
inline IndexQueue<V>::IndexQueue(std::vector<IndexNode<V>> &arr) : array_(arr) {
  SetFront(-EINVAL);
  SetBack(-EINVAL);
  length_ = 0;
}

template <typename V>
inline IndexNode<V> &IndexQueue<V>::FrontNode() {
  assert(Front() >= 0);
  return array_[Front()];
}

template <typename V>
inline IndexNode<V> &IndexQueue<V>::BackNode() {
  assert(Back() >= 0);
  return array_[Back()];
}

template <typename V>
inline bool IndexQueue<V>::Empty() const {
  assert((Front() == -EINVAL) == (Back() == -EINVAL));
  assert((length_ == 0) == (Front() == -EINVAL));
  return Front() == -EINVAL;
}

template <typename V>
inline int IndexQueue<V>::PopFront() {
  if (Empty()) return -EINVAL;
  const int front = Front();
  Remove(front);
  return front;
}

template <typename V>
inline void IndexQueue<V>::Remove(int i) {
  assert(i >= 0);
  const int prev = array_[i].prev;
  const int next = array_[i].next;
  
  if (prev == -EINVAL) {
    assert(Front() == i);
    SetFront(next);
  } else {
    array_[prev].next = next;
  }
  
  if (next == -EINVAL) {
    assert(Back() == i);
    SetBack(prev);
  } else {
    array_[next].prev = prev;
  }
  
  array_[i].prev = -EINVAL;
  array_[i].next = -EINVAL;
  
  --length_;
}

template <typename V>
inline void IndexQueue<V>::PushBack(int i) {
  assert(i >= 0);
  assert(array_[i].prev == -EINVAL && array_[i].next == -EINVAL);
  if (Empty()) {
    SetFront(i);
    SetBack(i);
  } else {
    array_[i].prev = Back();
    BackNode().next = i;
    SetBack(i);
  }
  
  ++length_;
}

template <typename V>
inline int IndexQueue<V>::Accept(QueueVisitor* visitor) {
  int num = 0, tmp;
  for (int i = Front(); i != -EINVAL; ++num) {
    tmp = array_[i].next;
    visitor->Visit(i);
    i = tmp;
  }
  return num;
}

#endif // TraceSimulator_index_queue_h
