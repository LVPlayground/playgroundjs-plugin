// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

// A priority queue is a container type that keeps its entries sorted in order of relative priority.
// This implementation takes an external comperator and offers the ability to enqueue, peek and
// dequeue entries. Additionally, existing entries can be filtered using a predicate.
class PriorityQueue {
  constructor(comperator) {
    this.comperator_ = comperator;
    this.entries_ = [];
  }

  // Enqueues |entry| in the priority queue.
  Enqueue(entry) {
    this.entries_.push(entry);
    this.Bubble(this.entries_.length - 1);
  }

  // Removes the element with the highest priority. Throws if the priority queue is empty.
  Dequeue() {
    if (!this.entries_.length)
      throw new Error('Cannot dequeue from an empty PriorityQueue.')

    this.entries_.shift();
  }

  // Returns the entry with the highest priority. Throws if the priority queue is empty.
  Peek() {
    if (!this.entries_.length)
      throw new Error('Cannot peek in an empty PriorityQueue.');

    return this.entries_[0];
  }

  // Filters items in m the priority queue for which |predicate| returns false. This method will
  // iterate over all entries in the queue, and therefore has O(n) performance.
  Filter(predicate) {
    this.entries_ = this.entries_.filter(predicate);
  }

  // Returns the number of entries currently in this priority queue.
  Size() {
    return this.entries_.length;
  }

  // Logarithmically bubbles the element at |index| up to the position where it should be. Used
  // to find the position within the queue for newly added elements.
  Bubble(index) {
    while (index >= 1) {
      let candidate_index = index >> 1;
      if (this.Compare(index, candidate_index) > 0)
        break;

      this.Swap(index, candidate_index);
      index = candidate_index;
    }
  }

  // Compares the entries at indices |lhs| and |rhs| with each other.
  Compare(lhs, rhs) {
    return this.comperator_(this.entries_[lhs], this.entries_[rhs]);
  }

  // Swaps the entries at indices |lhs| and |rhs| with each other.
  Swap(lhs, rhs) {
    let temp = this.entries_[lhs];
    this.entries_[lhs] = this.entries_[rhs];
    this.entries_[rhs] = temp;
  }

};

exports = PriorityQueue;
