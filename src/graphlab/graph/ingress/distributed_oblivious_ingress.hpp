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

#ifndef GRAPHLAB_DISTRIBUTED_OBLIVIOUS_INGRESS_HPP
#define GRAPHLAB_DISTRIBUTED_OBLIVIOUS_INGRESS_HPP


#include <graphlab/graph/graph_basic_types.hpp>
#include <graphlab/graph/ingress/distributed_ingress_base.hpp>
#include <graphlab/graph/ingress/ingress_edge_decision.hpp>
#include <graphlab/graph/distributed_graph.hpp>
#include <graphlab/rpc/buffered_exchange.hpp>
#include <graphlab/rpc/distributed_event_log.hpp>
#include <graphlab/util/dense_bitset.hpp>
#include <graphlab/util/cuckoo_map_pow2.hpp>
#include <graphlab/parallel/pthread_tools.hpp>
#include <graphlab/macros_def.hpp>
namespace graphlab {
  template<typename VertexData, typename EdgeData>
    class distributed_graph;

  /**
   * \brief Ingress object assigning edges using randoming hash function.
   */
  template<typename VertexData, typename EdgeData>
  class distributed_oblivious_ingress: 
    public distributed_ingress_base<VertexData, EdgeData> {
  public:
    typedef distributed_graph<VertexData, EdgeData> graph_type;
    /// The type of the vertex data stored in the graph 
    typedef VertexData vertex_data_type;
    /// The type of the edge data stored in the graph 
    typedef EdgeData   edge_data_type;
    
    typedef typename graph_type::vertex_record vertex_record;
    typedef typename graph_type::mirror_type mirror_type;

    typedef distributed_ingress_base<VertexData, EdgeData> base_type;
    // typedef typename boost::unordered_map<vertex_id_type, std::vector<size_t> > degree_hash_table_type;
    typedef fixed_dense_bitset<RPC_MAX_N_PROCS> bin_counts_type; 

    /** Type of the degree hash table: 
     * a map from vertex id to a bitset of length num_procs. */
    typedef cuckoo_map_pow2<vertex_id_type, bin_counts_type,3,uint32_t> degree_hash_table_type;
    degree_hash_table_type dht;

    /** Array of number of edges on each proc. */
    std::vector<size_t> proc_num_edges;
    simple_spinlock obliv_lock;
    
    /** Ingress traits. */
    bool usehash;
    bool userecent;

  public:
    distributed_oblivious_ingress(distributed_control& dc, graph_type& graph, bool usehash = false, bool userecent = false) :
      base_type(dc, graph),
      dht(-1),proc_num_edges(dc.numprocs()), usehash(usehash), userecent(userecent) { 

      //INITIALIZE_TRACER(ob_ingress_compute_assignments, "Time spent in compute assignment");
     }

    ~distributed_oblivious_ingress() { }

    /** Add an edge to the ingress object using oblivious greedy assignment. */
    void add_edge(vertex_id_type source, vertex_id_type target,
                  const EdgeData& edata) {
      obliv_lock.lock();
      dht[source]; dht[target];
      const procid_t owning_proc = 
        base_type::edge_decision.edge_to_proc_greedy(source, target, dht[source], dht[target], proc_num_edges, usehash, userecent);
      obliv_lock.unlock();

      typedef typename base_type::edge_buffer_record edge_buffer_record;
      edge_buffer_record record(source, target, edata);
#ifdef _OPENMP
      base_type::edge_exchange.send(owning_proc, record, omp_get_thread_num());
#else      
      base_type::edge_exchange.send(owning_proc, record);
#endif
    } // end of add edge

    virtual void finalize() {
     dht.clear();
     distributed_ingress_base<VertexData, EdgeData>::finalize(); 
      
    }

  }; // end of distributed_ob_ingress

}; // end of namespace graphlab
#include <graphlab/macros_undef.hpp>


#endif
