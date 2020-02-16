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


#ifndef GRAPHLAB_SMALL_MAP_HPP
#define GRAPHLAB_SMALL_MAP_HPP


#include <iostream>
#include <set>
#include <iterator>
#include <algorithm>

#include <graphlab/serialization/iarchive.hpp>
#include <graphlab/serialization/oarchive.hpp>

#include <graphlab/util/small_set.hpp>

#include <graphlab/macros_def.hpp>
namespace graphlab {

  template<size_t MAX_DIM, typename KeyT, typename ValueT>
  class small_map {
    template< size_t T1, size_t T2 >
    struct max_type { enum max_value { value = T1 < T2? T2 : T1 }; };
    struct less {
      bool operator()(const std::pair<KeyT, ValueT>& a, 
                      const std::pair<KeyT, ValueT>& b) const {
        return a.first < b.first;
      } 
    };
  


  public: 
    
    typedef small_set< MAX_DIM, std::pair<KeyT, ValueT>, less > 
    small_set_type;
    typedef typename small_set_type::value_type value_type;
    typedef typename small_set_type::iterator   iterator;
    typedef typename small_set_type::const_iterator const_iterator;
    typedef KeyT   key_type;
    typedef ValueT mapped_type;


  public:
    //! construct an empty map
    small_map() { }
    
    //! Construct a map with a single element
    small_map(const key_type& key, const mapped_type& value) :
      set(std::make_pair(key, value)) {  }
    

    //! Get the begin iterator
    inline iterator begin() { return set.begin(); }

    //! get the end iterator
    inline iterator end() { return set.end(); }


    //! Get the begin iterator
    inline const_iterator begin() const { return set.begin(); }

    //! Get the end iterator
    inline const_iterator end() const { return set.end(); }

    //! get the size of the set
    inline size_t size() const { return set.size(); }

    //! determine if there are any elements in the set
    inline bool empty() const { return set.empty(); }

    
    //! test whether the set contains the given element
    bool contains(const value_type& pair) const {
      return set.contains(pair);
    }

    //! test whether the set contains the given element
    bool contains(const key_type& key) const {
      const_iterator iter =
        std::lower_bound(set.begin(), 
                         set.end(), 
                         std::make_pair(key, mapped_type()));
      return (iter != set.end()) && (iter->first == key);
    }

    //! test whether the set has the given key
    inline bool has_key(const key_type& key) {
      return contains(key);
    }


    //! test whether the set contains the given set of element
    template<size_t OtherDim>
    bool contains(const small_map<OtherDim, KeyT, ValueT>& other) const {
      return set.contains(other.set);
    }

    template<size_t OtherDim>
    bool operator==(const small_map<OtherDim, KeyT, ValueT>& other) const {
      return set == other.set;
    }

    //! Lookup an element in the map
    inline const mapped_type& operator[](const key_type& key) const {
      value_type* const ptr = 
        std::lower_bound(set.begin(), set.end(), 
                         std::make_pair(key, mapped_type()),
                         less());
      ASSERT_NE(ptr, set.end());
      ASSERT_TURE(ptr->first == key);
      return ptr->second;
    }

    //! Lookup an element in the map
    inline mapped_type& operator[](const key_type& key) {
      value_type* ptr = 
        std::lower_bound(set.begin(), set.end(), 
                         std::make_pair(key, mapped_type()),
                         less());
      if(ptr != end() && (key == ptr->first) ) { return ptr->second; }
      // Add the entry to the map
      set += std::make_pair(key, ValueT());
      ptr = 
        std::lower_bound(set.begin(), set.end(),
                         std::make_pair(key, mapped_type()),
                         less());
      ASSERT_NE(ptr, set.end());
      ASSERT_TRUE(ptr->first == key);
      return ptr->second;
    }

    inline mapped_type& safe_find(const key_type& key) {
      value_type* const ptr = 
        std::lower_bound(set.begin(), set.end(), 
                         std::make_pair(key, mapped_type()),
                         less());
      ASSERT_NE(ptr, set.end());
      ASSERT_TRUE(ptr->first == key);
      return ptr->second;
    }

    //! Take the union of two maps
    template<size_t OtherDim>
    inline small_map<max_type<OtherDim, MAX_DIM>::value, KeyT, ValueT> 
    operator+(const small_map<OtherDim, KeyT, ValueT>& other) const {      
      typedef small_map<max_type<OtherDim, MAX_DIM>::value, KeyT, ValueT >
        result_type;
      result_type result;
      result.set = set + other.set;
      return result;
    }



  private:
    small_set_type set;
    


  }; // end of small map


  template<size_t MAX_DIM, typename KeyT, typename ValueT>
  std::ostream&
  operator<<(std::ostream& out,
            const graphlab::small_map<MAX_DIM, KeyT, ValueT>& map) {
    typedef std::pair<KeyT, ValueT> pair_type;
    size_t index = 0;
    out << '{';
    foreach(const pair_type& pair, map) {
      out << pair.first << "->" << pair.second;
      if(++index < map.size()) out << ", ";
    }
    return out << '}';
  }

}; // end graphlab

#include <graphlab/macros_undef.hpp>
#endif

