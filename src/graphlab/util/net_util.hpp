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


#ifndef GRAPHLAB_NET_UTIL_HPP
#define GRAPHLAB_NET_UTIL_HPP
#include <string>
#include <stdint.h>

namespace graphlab {
  /**
  * \ingroup util
  * Returns the first non-localhost ipv4 address 
  */
  uint32_t get_local_ip(bool print = true);

  /**
  * \ingroup util
  * Returns the first non-localhost ipv4 address as a standard dot delimited string
  */
  std::string get_local_ip_as_str(bool print = true);
  /** \ingroup util 
   * Find a free tcp port and binds it. Caller must release the port.
   * Returns a pair of [port, socket handle]
   */
  std::pair<size_t, int> get_free_tcp_port();
};

#endif

