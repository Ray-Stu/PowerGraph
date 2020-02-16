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


#ifndef GRAPHLAB_SERIALIZE_UNORDERED_SET_HPP
#define GRAPHLAB_SERIALIZE_UNORDERED_SET_HPP

#include <boost/unordered_set.hpp>
#include <graphlab/serialization/iarchive.hpp>
#include <graphlab/serialization/oarchive.hpp>
#include <graphlab/serialization/iterator.hpp>

namespace graphlab {
namespace archive_detail {
  /** serializes a set  */
  template <typename OutArcType, typename T>
  struct serialize_impl<OutArcType, boost::unordered_set<T>, false > {
  static void exec(OutArcType& oarc, const boost::unordered_set<T>& vec){
    serialize_iterator(oarc,
                       vec.begin(), vec.end(), vec.size());
  }
  };

  /** deserializes a set  */
  template <typename InArcType, typename T>
  struct deserialize_impl<InArcType, boost::unordered_set<T>, false > {
  static void exec(InArcType& iarc, boost::unordered_set<T>& vec){
    vec.clear();
    // get the number of elements to deserialize
    size_t length = 0;
    iarc >> length;    
    // iterate through and send to the output iterator
    for (size_t x = 0; x < length ; ++x){
      T v;
      iarc >> v;
      vec.insert(v);
    }
  }
  };

} // archive_detail  
} // graphlab

#endif 

