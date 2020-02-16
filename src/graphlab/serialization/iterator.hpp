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


#ifndef GRAPHLAB_SERIALIZE_ITERATOR_HPP
#define GRAPHLAB_SERIALIZE_ITERATOR_HPP

#include <iterator>
#include <graphlab/serialization/oarchive.hpp>
#include <graphlab/serialization/iarchive.hpp>

namespace graphlab {

  /**
   * \ingroup group_serialization
    \brief Serializes the contents between the iterators begin and end.

    This function prefers random access iterators since it needs
    a distance between the begin and end iterator.
    This function as implemented will work for other input iterators
    but is extremely inefficient.
    
    \tparam OutArcType The output archive type. This should not need to be
                       specified. The compiler will typically infer this
                       correctly.
    \tparam RandomAccessIterator The iterator type. This should not need to be
                       specified. The compiler will typically infer this
                       correctly.
    
    \param oarc A reference to the output archive to write to.
    \param begin The start of the iterator range to write.
    \param end The end of the iterator range to write.
   */
  template <typename OutArcType, typename RandomAccessIterator>
  void serialize_iterator(OutArcType& oarc, RandomAccessIterator begin,
                                            RandomAccessIterator end){
    const size_t vsize = std::distance(begin, end);
    oarc << vsize;
    // store each element
    for(; begin != end; ++begin) oarc << *begin;
  }


  /**
    \ingroup group_serialization
    \brief Serializes the contents between the iterators begin and end.

    This functions takes all iterator types, but takes a "count" for
    efficiency. This count is checked and will return failure if the number
    of elements serialized does not match the count
 
    \tparam OutArcType The output archive type. This should not need to be
                       specified. The compiler will typically infer this
                       correctly.
    \tparam InputIterator The iterator type. This should not need to be
                       specified. The compiler will typically infer this
                       correctly.

    \param oarc A reference to the output archive to write to.
    \param begin The start of the iterator range to write.
    \param end The end of the iterator range to write.
    \param vsize The distance between the iterators begin and end. Must match
                 std::distance(begin, end);
   */
  template <typename OutArcType, typename InputIterator>
  void serialize_iterator(OutArcType& oarc, InputIterator begin,
                                            InputIterator end, size_t vsize){
    oarc << vsize;
    //store each element
    size_t count = 0;
    for(; begin != end; ++begin) { oarc << *begin;  ++count; }
    // fail if count does not match
    ASSERT_EQ(count, vsize);
  }

  /**
    \ingroup group_serialization
    \brief The accompanying function to serialize_iterator()
    Reads elements from the stream and writes it to the output iterator.
    
    Note that this requires an additional template parameter T which is the
    "type of object to deserialize"
    This is necessary for instance for the map type. The 
    <code>map<T,U>::value_type</code>
    is <code>pair<const T,U></code>which is not useful since I cannot assign to
    it.  In this case, <code>T=pair<T,U></code>

    \tparam OutArcType The output archive type. 
    \tparam T The type of values to deserialize
    \tparam OutputIterator The type of the output iterator to be written to.
                           This should not need to be specified. The compiler
                           will typically infer this correctly.

    \param iarc A reference to the input archive
    \param result The output iterator to write to

   */
  template <typename InArcType, typename T, typename OutputIterator>
  void deserialize_iterator(InArcType& iarc, OutputIterator result) {
    // get the number of elements to deserialize
    size_t length = 0;
    iarc >> length;
    
    // iterate through and send to the output iterator
    for (size_t x = 0; x < length ; ++x){
      /**
       * A compiler error on this line means that one of the user
       * defined types currently trying to be serialized (e.g.,
       * vertex_data, edge_data, messages, gather_type, or
       * vertex_programs) does not have a default constructor.
       */
      T v;
      iarc >> v;
      (*result) = v;
      result++;
    }
  }
  
 
} 
#endif 

