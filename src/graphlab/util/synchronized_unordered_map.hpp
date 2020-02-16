/**  
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


#ifndef GRAPHLAB_SYNCHRONIZED_UNORDERED_MAP
#define GRAPHLAB_SYNCHRONIZED_UNORDERED_MAP
#include <vector>
#include <boost/unordered_map.hpp>
#include <graphlab/parallel/pthread_tools.hpp>

namespace graphlab {

/// \ingroup util_internal
template <typename Data>
class synchronized_unordered_map {
 public:
  typedef boost::unordered_map<size_t, Data> container;
  typedef typename container::iterator iterator;
  typedef typename container::const_iterator const_iterator;

  typedef std::pair<bool, Data*> datapointer;
  typedef std::pair<bool, const Data*> const_datapointer;
  typedef Data value_type;
  typedef size_t key_type;
  
 private:
   std::vector<container> data;
   std::vector<rwlock> lock; 
   size_t nblocks;
 public:
   synchronized_unordered_map(size_t numblocks):data(numblocks), 
                                                lock(numblocks),
                                                nblocks(numblocks) { 
    for (size_t i = 0;i < numblocks; ++i) {
      data[i].max_load_factor(1.0);
    }
  }

   std::pair<bool, Data*> find(size_t key) {
     size_t b = key % nblocks;
     lock[b].readlock();
     iterator iter = data[b].find(key);
     std::pair<bool, Data*> ret = std::make_pair(iter != data[b].end(), &(iter->second));
     lock[b].rdunlock();
     return ret;
   }
   
   /**
   return std::pair<found, iterator>
   if not found, iterator is invalid
   */
   std::pair<bool, const Data*> find(size_t key) const {
     size_t b = key % nblocks;
     lock[b].readlock();
     const_iterator iter = data[b].find(key);
     std::pair<bool, const Data*> ret = std::make_pair(iter != data[b].end(), &(iter->second));
     lock[b].rdunlock();
     return ret;
   }
   
   // care must be taken that  you do not access an erased iterator
   void erase(size_t key) {
     size_t b = key % nblocks;
     lock[b].writelock();
     data[b].erase(key);
     lock[b].wrunlock();
   }

   template<typename Predicate>
   void erase_if(size_t key, Predicate pred) {
     size_t b = key % nblocks;
     lock[b].writelock();
     iterator iter = data[b].find(key); 
      
     if (iter != data[b].end() && pred(iter->second))  data[b].erase(key);
     lock[b].wrunlock();
   }

   value_type& insert(size_t key, const value_type &val) {
     size_t b = key % nblocks;
     lock[b].writelock();
     data[b][key] = val;
     value_type& ret = data[b][key];
     lock[b].wrunlock();
     return ret;
   }

    /**
    returns std::pair<success, iterator>
    on success, iterator will point to the entry
    on failure, iterator will point to an existing entry
    */
   std::pair<bool, Data*> insert_with_failure_detect(size_t key, const value_type &val) {
     std::pair<bool, Data*> ret ;
     
     size_t b = key % nblocks;
     lock[b].writelock();
     //search for it
     iterator iter = data[b].find(key);
     // if it not in the table, write and return
     if (iter == data[b].end()) {
      data[b][key] = val;
      ret = std::make_pair(true,  &(data[b].find(key)->second));
     }
     else {
      ret = std::make_pair(false,  &(iter->second));
     }
     lock[b].wrunlock();
     return ret;
   }

   void clear() {
     for (size_t i = 0;i < data.size(); ++i) {
       data[i].clear();
     }
   }
};
}
#endif

