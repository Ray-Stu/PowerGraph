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


#ifndef GRAPHLAB_INTEGER_SELECTOR_HPP
#define GRAPHLAB_INTEGER_SELECTOR_HPP
#include <stdint.h>
namespace graphlab {
  
  template <int len>
  struct u_integer_selector {
    // invalid
  };

  template <>
  struct u_integer_selector<1> {
    typedef uint8_t integer_type;
  };

  template <>
  struct u_integer_selector<2> {
    typedef uint16_t integer_type;
  };

  template <>
  struct u_integer_selector<4> {
    typedef uint32_t integer_type;
  };

  template <>
  struct u_integer_selector<8> {
    typedef uint64_t integer_type;
  };

}
#endif


