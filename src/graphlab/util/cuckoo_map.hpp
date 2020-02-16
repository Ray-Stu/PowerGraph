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

#ifndef GRAPHLAB_UTIL_CUCKOO_MAP_HPP
#define GRAPHLAB_UTIL_CUCKOO_MAP_HPP

#include <vector>
#include <iterator>
#include <boost/random.hpp>
#include <boost/unordered_map.hpp>
#include <ctime>
#include <graphlab/serialization/serialization_includes.hpp>

namespace graphlab {



/**
 * A cuckoo hash map which requires the user to
 * provide an "illegal" value thus avoiding the need
 * for a seperate bitmap. More or less similar
 * interface as boost::unordered_map, not necessarily
 * entirely STL compliant.
 */
template <typename Key, typename Value,
          size_t CuckooK = 3,
          typename IndexType = size_t,
          typename Hash = boost::hash<Key>,
          typename Pred = std::equal_to<Key> >
class cuckoo_map {

public:
  // public typedefs
  typedef Key                                      key_type;
  typedef std::pair<Key const, Value>              value_type;
  typedef Value                                    mapped_type;
  typedef Hash                                     hasher;
  typedef Pred                                     key_equal;
  typedef IndexType                                index_type;
  typedef value_type* pointer;
  typedef value_type& reference;
  typedef const value_type* const_pointer;
  typedef const value_type& const_reference;
private:
  // internal typedefs
  typedef std::pair<key_type, mapped_type> non_const_value_type;
  typedef value_type* map_container_type;
  typedef value_type* map_container_iterator;
  typedef const value_type* map_container_const_iterator;
  typedef boost::unordered_map<Key, Value, Hash, Pred> stash_container_type;

  key_type illegalkey;
  index_type numel;
  index_type maxstash;
  map_container_type data;
  size_t datalen;
  stash_container_type stash;
  boost::rand48  drng;
  boost::uniform_int<index_type> kranddist;
  hasher hashfun;
  key_equal keyeq;

  map_container_iterator data_begin() {
    return data;
  }

  map_container_iterator data_end() {
    return data + datalen;
  }

  map_container_const_iterator data_begin() const {
    return data;
  }

  map_container_const_iterator data_end() const {
    return data + datalen;
  }
  // bypass the const key_type with a placement new
  void replace_in_vector(map_container_iterator iter,
                         const key_type& key,
                         const mapped_type& val) {
    // delete
    iter->~value_type();
    // placement new
    new(iter) value_type(key, val);
  }

  void destroy_all() {
    // call ze destructors
    for(size_t i = 0; i < datalen; ++i) {
      data[i].~value_type();
    }
    free(data);
    stash.clear();
    data = NULL;
    datalen = 0;
    numel = 0;
  }

public:
  struct insert_iterator{
    cuckoo_map* cmap;
    typedef std::forward_iterator_tag iterator_category;
    typedef typename cuckoo_map::value_type value_type;

    insert_iterator(cuckoo_map* c):cmap(c) {}

    insert_iterator operator++() {
      return (*this);
    }
    insert_iterator operator++(int) {
      return (*this);
    }

    insert_iterator& operator*() {
      return *this;
    }
    insert_iterator& operator=(const insert_iterator& i) {
      cmap = i.cmap;
      return *this;
    }

    insert_iterator& operator=(const value_type& v) {
      cmap->insert(v);
      return *this;
    }
  };

  struct const_iterator {
    const cuckoo_map* cmap;
    bool in_stash;
    typename cuckoo_map::map_container_const_iterator vec_iter;
    typename cuckoo_map::stash_container_type::const_iterator stash_iter;

    typedef std::forward_iterator_tag iterator_category;
    typedef typename cuckoo_map::value_type value_type;
    typedef size_t difference_type;
    typedef const value_type* pointer;
    typedef const value_type& reference;

    friend class cuckoo_map;

    const_iterator(): cmap(NULL), in_stash(false) {}

    const_iterator operator++() {
      if (!in_stash) {
        ++vec_iter;
        // we are in the main vector. try to advance the
        // iterator until I hit another data element
        while(vec_iter != cmap->data_end() &&
              cmap->key_eq()(vec_iter->first, cmap->illegal_key())) ++vec_iter;
        if (vec_iter == cmap->data_end()) {
          in_stash = true;
          stash_iter = cmap->stash.begin();
        }
      }
      else if (in_stash) {
        if (stash_iter != cmap->stash.end())  ++stash_iter;
      }
      return *this;
    }

    const_iterator operator++(int) {
      const_iterator cur = *this;
      ++(*this);
      return cur;
    }


    reference operator*() {
      if (!in_stash) return *vec_iter;
      else return *stash_iter;
    }

    pointer operator->() {
      if (!in_stash) return &(*vec_iter);
      else return &(*stash_iter);
    }

    bool operator==(const const_iterator iter) const {
     return in_stash == iter.in_stash &&
             (in_stash==false ?
                  vec_iter == iter.vec_iter :
                  stash_iter == iter.stash_iter);
    }

    bool operator!=(const const_iterator iter) const {
      return !((*this) == iter);
    }

    private:
    const_iterator(const cuckoo_map* cmap, typename cuckoo_map::map_container_const_iterator vec_iter):
      cmap(cmap), in_stash(false), vec_iter(vec_iter), stash_iter(cmap->stash.begin()) { }

    const_iterator(const cuckoo_map* cmap, typename cuckoo_map::stash_container_type::const_iterator stash_iter):
      cmap(cmap), in_stash(true), vec_iter(cmap->data_begin()), stash_iter(stash_iter) { }
  };


  struct iterator {
    cuckoo_map* cmap;
    bool in_stash;
    typename cuckoo_map::map_container_iterator vec_iter;
    typename cuckoo_map::stash_container_type::iterator stash_iter;

    typedef std::forward_iterator_tag iterator_category;
    typedef typename cuckoo_map::value_type value_type;
    typedef size_t difference_type;
    typedef value_type* pointer;
    typedef value_type& reference;

    friend class cuckoo_map;

    iterator(): cmap(NULL), in_stash(false) {}


    operator const_iterator() const {
      const_iterator iter;
      iter.cmap = cmap;
      iter.in_stash = in_stash;
      iter.vec_iter = vec_iter;
      iter.stash_iter = stash_iter;
      return iter;
    }

    iterator operator++() {
      if (!in_stash) {
        ++vec_iter;
        // we are in the main vector. try to advance the
        // iterator until I hit another data element
        while(vec_iter != cmap->data_end() &&
              cmap->key_eq()(vec_iter->first, cmap->illegal_key())) ++vec_iter;
        if (vec_iter == cmap->data_end()) {
          in_stash = true;
          stash_iter = cmap->stash.begin();
        }
      }
      else if (in_stash) {
        if (stash_iter != cmap->stash.end())  ++stash_iter;
      }
      return *this;
    }

    iterator operator++(int) {
      iterator cur = *this;
      ++(*this);
      return cur;
    }


    reference operator*() {
      if (!in_stash) return *vec_iter;
      else return *stash_iter;
    }

    pointer operator->() {
      if (!in_stash) return &(*vec_iter);
      else return &(*stash_iter);
    }

    bool operator==(const iterator iter) const {
      return in_stash == iter.in_stash &&
             (in_stash==false ?
                  vec_iter == iter.vec_iter :
                  stash_iter == iter.stash_iter);
    }

    bool operator!=(const iterator iter) const {
      return !((*this) == iter);
    }


    private:
    iterator(cuckoo_map* cmap, typename cuckoo_map::map_container_iterator vec_iter):
      cmap(cmap), in_stash(false), vec_iter(vec_iter) { }

    iterator(cuckoo_map* cmap, typename cuckoo_map::stash_container_type::iterator stash_iter):
      cmap(cmap), in_stash(true), stash_iter(stash_iter) { }

  };

private:

  // the primary inserting logic.
  // this assumes that the data is not already in the array.
  // caller must check before performing the insert
  iterator do_insert(const value_type& v_) {
    non_const_value_type v(v_.first, v_.second);
    if (stash.size() > maxstash) {
      // resize
      reserve(datalen * 1.5);
    }

    index_type insertpos = (index_type)(-1); // tracks where the current
                                     // inserted value went
    ++numel;

    // take a random walk down the tree
    for (int i = 0;i < 100; ++i) {
      // first see if one of the hashes will work
      index_type idx = 0;
      bool found = false;
      size_t hash_of_k = hashfun(v.first);
      for (size_t j = 0; j < CuckooK; ++j) {
        idx = compute_hash(hash_of_k, j);
        if (keyeq(data[idx].first, illegalkey)) {
          found = true;
          break;
        }
      }
      if (!found) idx = compute_hash(hash_of_k, kranddist(drng));
      // if insertpos is -1, v holds the current value. and we
      //                     are inserting it into idx
      // if insertpos is idx, we are bumping v again. and v will hold the
      //                      current value once more. so revert
      //                      insertpos to -1
      if (insertpos == (index_type)(-1)) insertpos = idx;
      else if (insertpos == idx) insertpos = (index_type)(-1);
      // there is room here
      if (found || keyeq(data[idx].first, illegalkey)) {
        replace_in_vector(data_begin() + idx, v.first, v.second);
        // success!
        return iterator(this, data_begin() + insertpos);
      }
      // failed to insert!
      // try again!

      non_const_value_type tmp = data[idx];
      replace_in_vector(data_begin() + idx, v.first, v.second);
      v = tmp;
    }
    // ok. tried and failed 100 times.
    //stick it in the stash

    typename stash_container_type::iterator stashiter = stash.insert(v).first;
    // if insertpos is -1, current value went into stash
    if (insertpos == (index_type)(-1)) {
      return iterator(this, stashiter);
    }
    else {
      return iterator(this, data_begin() + insertpos);
    }
  }

public:

  cuckoo_map(key_type illegalkey,
             index_type stashsize = 8,
             hasher const& h = hasher(),
            key_equal const& k = key_equal()):
              illegalkey(illegalkey),
              numel(0),maxstash(stashsize),
              data(NULL), datalen(0),
              drng(time(NULL)),
              kranddist(0, CuckooK - 1), hashfun(h), keyeq(k) {
    stash.max_load_factor(1.0);
    reserve(128);
  }



  cuckoo_map& operator=(const cuckoo_map& other) {
    destroy_all();
    // copy the data
    data = (map_container_type)malloc(sizeof(value_type) * other.datalen);
    datalen = other.datalen;
    std::uninitialized_copy(other.data_begin(), other.data_end(), data_begin());

    // copy the stash
    stash = other.stash;

    // copy all the other extra stuff
    illegalkey = other.illegalkey;
    numel = other.numel;
    hashfun = other.hashfun;
    keyeq = other.keyeq;
    return *this;
  }
  
  const key_type& illegal_key() const {
    return illegalkey;
  }
  
  ~cuckoo_map() {
    destroy_all();
  }

  index_type size() {
    return numel;
  }

  iterator begin() {
    iterator iter;
    iter.cmap = this;
    iter.in_stash = false;
    iter.vec_iter = data_begin();

    while(iter.vec_iter != data_end() &&
          keyeq(iter.vec_iter->first, illegalkey)) ++iter.vec_iter;


    if (iter.vec_iter == data_end()) {
      iter.in_stash = true;
      iter.stash_iter = stash.begin();
    }
      
    return iter;
  }

  iterator end() {
    return iterator(this, stash.end());
  }


  const_iterator begin() const {
    const_iterator iter;
    iter.cmap = this;
    iter.in_stash = false;
    iter.vec_iter = data_begin();

    while(iter.vec_iter != data_end() &&
          keyeq(iter.vec_iter->first, illegalkey)) ++iter.vec_iter;

    if (iter.vec_iter == data_end()) {
      iter.in_stash = true;
      iter.stash_iter = stash.begin();
    }

    return iter;
  }

  const_iterator end() const {
    return const_iterator(this, stash.end());

  }

  /*
   * Bob Jenkin's 32 bit integer mix function from
   * http://home.comcast.net/~bretm/hash/3.html
   */
  static size_t mix(size_t state) {
    state += (state << 12);
    state ^= (state >> 22);
    state += (state << 4);
    state ^= (state >> 9);
    state += (state << 10);
    state ^= (state >> 2);
    state += (state << 7);
    state ^= (state >> 12);
    return state;
  }

  index_type compute_hash(size_t k , const uint32_t seed) const {
    // a bunch of random numbers
#if (__SIZEOF_PTRDIFF_T__ == 8)
      static const size_t a[8] = {0x6306AA9DFC13C8E7,
                                  0xA8CD7FBCA2A9FFD4,
                                  0x40D341EB597ECDDC,
                                  0x99CFA1168AF8DA7E,
                                  0x7C55BCC3AF531D42,
                                  0x1BC49DB0842A21DD,
                                  0x2181F03B1DEE299F,
                                  0xD524D92CBFEC63E9};
#else
      static const size_t a[8] = {0xFC13C8E7,
                                  0xA2A9FFD4,
                                  0x597ECDDC,
                                  0x8AF8DA7E,
                                  0xAF531D42,
                                  0x842A21DD,
                                  0x1DEE299F,
                                  0xBFEC63E9};
#endif

    index_type s = mix(a[seed] ^ k);
    return s % datalen;
  }

  void rehash() {
    stash_container_type stmp;
    stmp.swap(stash);
    // effectively, stmp elements are deleted
    numel -= stmp.size();
    for (size_t i = 0;i < datalen; ++i) {
      // if there is an element here. erase it and reinsert
      if (!keyeq(data[i].first, illegalkey)) {
        if (count(data[i].first)) continue;
        non_const_value_type v = data[i];
        replace_in_vector(data_begin() + i, illegalkey, mapped_type());
        numel--;
        //erase(iterator(this, data_begin() + i));
        insert(v);
      }
    }
    typename stash_container_type::const_iterator iter = stmp.begin();
    while(iter != stmp.end()) {
      insert(*iter);
      ++iter;
    }
  }



  void reserve(size_t newlen) {
    //data.reserve(newlen);
    //data.resize(newlen, std::make_pair<Key, Value>(illegalkey, Value()));
    data = (map_container_type)realloc(data, newlen * sizeof(value_type));
    std::uninitialized_fill(data_end(), data+newlen, non_const_value_type(illegalkey, mapped_type()));
    datalen = newlen;
    rehash();
  }

  std::pair<iterator, bool> insert(const value_type& v_) {
    iterator i = find(v_.first);
    if (i != end()) return std::make_pair(i, false);
    else return std::make_pair(do_insert(v_), true);
  }



  iterator insert(const_iterator hint, value_type const& v) {
    return insert(v).first;
  }
  
  iterator find(key_type const& k) {
    size_t hash_of_k = hashfun(k);
    for (uint32_t i = 0;i < CuckooK; ++i) {
      index_type idx = compute_hash(hash_of_k, i);
      if (keyeq(data[idx].first, k)) return iterator(this, data_begin() + idx);
    }
    return iterator(this, stash.find(k));
  }

  const_iterator find(key_type const& k) const {
    size_t hash_of_k = hashfun(k);
    for (uint32_t i = 0;i < CuckooK; ++i) {
      index_type idx = compute_hash(hash_of_k, i);
      if (keyeq(data[idx].first, k)) return const_iterator(this, data_begin() + idx);
    }
    return const_iterator(this, stash.find(k));
  }

  size_t count(key_type const& k) const {
    size_t hash_of_k = hashfun(k);
    for (uint32_t i = 0;i < CuckooK; ++i) {
      index_type idx = compute_hash(hash_of_k, i);
      if (keyeq(data[idx].first, k)) return true;
    }
    return stash.count(k);
  }


  void erase(iterator iter) {
    if (iter.in_stash == false) {
      if (!keyeq(iter.vec_iter->first, illegalkey)) {

        replace_in_vector(&(*(iter.vec_iter)), illegalkey, mapped_type());

        --numel;
      }
    }
    else if (iter.stash_iter != stash.end()) {
      --numel;
      stash.erase(iter.stash_iter);
    }
  }

  void erase(key_type const& k) {
    iterator iter = find(k);
    if (iter != end()) erase(iter);
  }

  void swap(cuckoo_map& other) {
    std::swap(illegalkey, other.illegalkey);
    std::swap(numel, other.numel);
    std::swap(maxstash, other.maxstash);
    std::swap(data, other.data);
    std::swap(datalen, other.datalen);
    std::swap(stash, other.stash);
    std::swap(drng, other.drng);
    std::swap(kranddist, other.kranddist);
    std::swap(hashfun, other.hashfun);
    std::swap(keyeq, other.keyeq);
  }

  mapped_type& operator[](const key_type& i) {
    iterator iter = find(i);
    value_type tmp(i, mapped_type());
    if (iter == end()) iter = do_insert(tmp);
    return iter->second;
  }

  key_equal key_eq() const {
    return keyeq;
  }

  void clear() {
    destroy_all();
    reserve(128);
  }


  float load_factor() const {
    return (float)numel / (datalen + stash.size());
  }

  void save(oarchive &oarc) const {
    oarc << numel << illegalkey;
    serialize_iterator(oarc, begin(), end(), numel);
  }


  void load(iarchive &iarc) {
    clear();
    size_t tmpnumel = 0;
    iarc >> tmpnumel >> illegalkey;
    reserve(tmpnumel * 1.5);
    deserialize_iterator<iarchive, non_const_value_type>
      (iarc, insert_iterator(this));
  }
  
};

}

#endif
