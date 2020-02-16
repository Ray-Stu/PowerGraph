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


#ifndef GRAPHLAB_DC_INIT_FROM_MPI_HPP
#define GRAPHLAB_DC_INIT_FROM_MPI_HPP
#include <graphlab/rpc/dc.hpp>
namespace graphlab {
  /**
   * \ingroup rpc 
   * initializes parameters from MPI. Returns true on success
      MPI must be initialized before calling this function */
  bool init_param_from_mpi(dc_init_param& param, dc_comm_type commtype = TCP_COMM);
}

#endif // GRAPHLAB_DC_INIT_FROM_MPI_HPP


