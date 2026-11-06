#pragma once
#include <stdint.h>
#include <string.h>
#include <assert.h>

template<typename T> class min_heap
{
public:
  min_heap():heap(0),heap_size(0), heap_capacity(0) { }
  ~min_heap() { clear(); delete[] heap; }

  struct node
  {
    node(const T &value, int key):value(value),key_(key) {}
    T value; ///< Stores the actual object itself.
    int key() const { return key_; }
    int key_; ///< priority key of this element, lower value == higher priority
    int idx; ///< Index of this element in the heap.
private:
    friend class min_heap;
  };

  /// Adds a new element to the heap along with the key value that represents
  /// the element's priority.
  /// Returns a pointer to a container object for the object on the
  /// heap. This node pointer is valid as long as the given element still
  /// resides on the heap.
  node *push(const T &value, int key);

  /// Returns the node corresponding to the highest priority item, without
  /// removing it from the heap. Returns a null pointer if the heap is empty.
  node *front_node() { return heap_size > 0 ? heap[0] : 0; }

  /// Returns the highest priority value on the heap.
  T &front() { assert(heap_size > 0); return heap[0]->value; }

  /// Removes the highest priority item on the heap.
  T pop_front();

  /// Returns true if there are no elements left on the heap.
  bool empty() const { return heap_size == 0; }

  /// Shrinks the key value of the given element on the heap. Call this
  /// to update the key value of a node with a higher priority key.
  void decrease_key(node *n, int new_key);

  /// Removes all elements in the heap.
  void clear();

  /// Runs an internal consistency check on the data structure. Requires assertions enabled.
  void debug_check();
  node **heap;
  int heap_size, heap_capacity;
private:
  static inline int heap_parent_index(int i) { return (i-1) >> 1; }
  static inline int heap_left_child_index(int i) { return (i<<1) + 1; }
  inline void assign(node *n, int i) { heap[i] = n; n->idx = i; }
  min_heap(const min_heap &) = delete;            // Delete operators that would copy the data structure,
  min_heap &operator=(const min_heap &) = delete; // since that is very likely not desirable.
};

template<typename T> typename min_heap<T>::node *min_heap<T>::push(const T &value, int key)
{
  if (heap_size >= heap_capacity)
  {
    heap_capacity = (heap_capacity<<1)+16;
    heap = (node**)realloc(heap, heap_capacity*sizeof(node*));
  }
  node *n = new node(value, key);
  int i = heap_size++;
  while(i > 0)
  {
    int p = heap_parent_index(i);
    node *parent = heap[p];
    if (key >= parent->key_) break;
    assign(parent, i);
    i = p;
  }
  assign(n, i);
  return n;
}

template<typename T> T min_heap<T>::pop_front()
{
  assert(heap_size > 0 && "Attempted min_heap::pop_front() on an empty heap!");
  node *front = heap[0];
  if (--heap_size > 0)
  {
    int first = 0;
    node *n = heap[heap_size]; // sink last node in heap down the root
    int key = n->key_;
    for(int i = 0;; i = first)
    {
      int l = heap_left_child_index(i);
      int r = l + 1;
      node **c = &heap[l];

      // Determine which node should come first
      if      (r < heap_size && c[1]->key_ < key) first = c[0]->key_ < c[1]->key_ ? l : r;
      else if (l < heap_size && c[0]->key_ < key) first = l;
      else { assign(n, i); break; } // we are done, settle the sunk node to its place
      assign(heap[first], i); // bubble the intermediate node up to its parent
    }
  }
  T value = front->value;
  delete front;
  return value;
}

template<typename T> void min_heap<T>::decrease_key(node *e, int new_key)
{
  assert(new_key < e->key_ && "min_heap::decrease_key() cannot be used to increase a key!");
  e->key_ = new_key;
  int i = e->idx;
  while(i > 0)
  {
    int p = heap_parent_index(i);
    node *parent = heap[p];
    if (new_key >= parent->key_) break;
    assign(parent, i);
    i = p;
  }
  assign(e, i);
}


template<typename T> void min_heap<T>::clear()
{
  for(int i = 0; i < heap_size; ++i)
    delete heap[i];
  heap_size = 0;
}

#ifndef NDEBUG
#include <unordered_set>
#endif

template<typename T> void min_heap<T>::debug_check()
{
#ifndef NDEBUG
  std::unordered_set<node*> nodes;
  for(int i = 0; i < heap_size; ++i)
  {
    node *n = heap[i];
    assert(n); // heap cannot contain nulls
    assert(nodes.find(n) == nodes.end()); // each node must only appear once
    nodes.insert(n);
    assert(n->idx == i); // each node should point back to its index in the heap
    int l = heap_left_child_index(i);
    for(int c = 0; c < 2; ++c)
    {
      int j = l + c;
      if (j < heap_size)
      {
        node *c = heap[j];
        assert(c);
        assert(n->key_ <= c->key_); // minheap property
      }
    }
  }
#endif
}
