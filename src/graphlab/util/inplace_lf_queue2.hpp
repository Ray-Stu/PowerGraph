/*  
 * Copyright (c) 2009 Carnegie Mellon University. 
 *     All rights reserved.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing,
 *  software distributed under the License is distributed on an "AS
 *  IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 *  express or implied.  See the License for the specific language
 *  governing permissions and limitations under the License.
 *
 * For more about this software visit:
 *
 *      http://www.graphlab.ml.cmu.edu
 *
 */


#ifndef GRAPHLAB_INPLACE_LOCKFREE_QUEUE2_HPP
#define GRAPHLAB_INPLACE_LOCKFREE_QUEUE2_HPP
#include <stdint.h>
#include <cstring>
#include <graphlab/parallel/atomic.hpp>
#include <graphlab/parallel/atomic_ops.hpp>
#include <utility>
namespace graphlab {

/*
 * A lock free queue which requires the stored element to have a
 * next pointer.
 *
 * head is the head of the queue. Always sentinel.
 * tail is current last element of the queue.
 * completed is the last element that is completely inserted.
 * There can only be one thread dequeueing.
 *
 * On dequeue_all, the dequeu-er should use get_next() to get the
 * next element in the list. If get_next() returns NULL, it should spin
 * until not null, and quit only when end_of_dequeue_list() evaluates to true
 */
template <typename T>
class inplace_lf_queue2 {
 public:
   inline inplace_lf_queue2():sentinel(new T), head(sentinel), tail(sentinel) {
     sentinel->next = NULL;
   }

   ~inplace_lf_queue2() {
     delete sentinel;
   }

   void enqueue(T* c) {
     // clear the next pointer
     (*get_next_ptr(c)) = NULL;
     // atomically,
     // swap(tail, c)
     // tail->next = c;
     T* prev = c;
     atomic_exchange(tail, prev);
     (*get_next_ptr(prev)) = c;
     numel.inc();
     asm volatile ("" : : : "memory");
   }

   size_t approx_size() {
    return numel;
   }

   bool empty() const {
     return head->next == NULL;
   }

   T* dequeue_all() {
     // head is the sentinel
     T* ret_head = get_next(head);
     if (ret_head == NULL) return NULL;
     // now, the sentinel is not actually part of the queue.
     // by the time get_next(sentinel) is non-empty, enqueue must have completely
     // finished at least once, since the next ptr is only connected in line 11.
     // enqueue the sentinel. That will be the new head of the queue.
     // Anything before the sentinel is "returned". And anything after is part
     // of the queue
     enqueue(sentinel);
     numel = 0;
     // The last element in the returned queue
     // will point to the sentinel.
     return ret_head;
   }

   static inline T* get_next(T* ptr) {
     return ptr->next;
   }

   static inline T** get_next_ptr(T* ptr) {
     return &(ptr->next);
   }

   T* end_of_dequeue_list() {
     return sentinel;
   }

   inline const bool end_of_dequeue_list(T* ptr) {
     return ptr == (sentinel);
   }

 private:

   atomic<size_t> numel;
   T* sentinel;
   T* head;
   T* tail;
};


} // namespace graphlab

#endif
