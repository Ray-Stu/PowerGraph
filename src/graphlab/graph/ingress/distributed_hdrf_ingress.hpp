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

#ifndef GRAPHLAB_DISTRIBUTED_HDRF_INGRESS_HPP
#define GRAPHLAB_DISTRIBUTED_HDRF_INGRESS_HPP


#include <graphlab/graph/graph_basic_types.hpp>
#include <graphlab/graph/ingress/distributed_ingress_base.hpp>
#include <graphlab/graph/ingress/ingress_edge_decision.hpp>
#include <graphlab/graph/distributed_graph.hpp>
#include <graphlab/rpc/buffered_exchange.hpp>
#include <graphlab/rpc/distributed_event_log.hpp>
#include <graphlab/util/dense_bitset.hpp>
#include <graphlab/util/cuckoo_map_pow2.hpp>
#include <graphlab/macros_def.hpp>
namespace graphlab {
  template<typename VertexData, typename EdgeData>
    class distributed_graph;

  template<typename VertexData, typename EdgeData>
  class distributed_hdrf_ingress: 
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
    typedef fixed_dense_bitset<RPC_MAX_N_PROCS> bin_counts_type; 

    /** Type of the replica degree hash table: 
     * a map from vertex id to a bitset of length num_procs.
	 */
    typedef cuckoo_map_pow2<vertex_id_type, bin_counts_type,3,uint32_t> degree_hash_table_type;
    degree_hash_table_type dht;
        
    /** Type of the vertex degree hash table:
     * a map from vertex id to a bitset of length num_procs.
	 */
    typedef cuckoo_map_pow2<vertex_id_type, size_t,3,uint32_t> true_degree_hash_table_type;
    true_degree_hash_table_type degree_dht;

    /** Array of number of edges on each proc. */
    std::vector<size_t> proc_num_edges;

    /** Ingress tratis. */
    bool usehash;
    bool userecent;

  public:
    distributed_hdrf_ingress(distributed_control& dc, graph_type& graph, bool usehash = false, bool userecent = false) :
      base_type(dc, graph),
      dht(-1),degree_dht(-1),proc_num_edges(dc.numprocs()), usehash(usehash), userecent(userecent) {

      //INITIALIZE_TRACER(ob_ingress_compute_assignments, "Time spent in compute assignment");
     }

    ~distributed_hdrf_ingress() { }

    /** Add an edge to the ingress object using hdrf greedy assignment. */
    void add_edge(vertex_id_type source, vertex_id_type target,
                  const EdgeData& edata) {
      dht[source]; dht[target];
      degree_dht[source]; degree_dht[target];

      const procid_t owning_proc = 
        base_type::edge_decision.edge_to_proc_hdrf(source, target, dht[source], dht[target], degree_dht[source], degree_dht[target], proc_num_edges, usehash, userecent);

      typedef typename base_type::edge_buffer_record edge_buffer_record;
      edge_buffer_record record(source, target, edata);
      base_type::edge_exchange.send(owning_proc, record);
    } // end of add edge

    virtual void finalize() {
     dht.clear();
     degree_dht.clear();
     distributed_ingress_base<VertexData, EdgeData>::finalize();
        
        size_t count = 0;
        for(std::vector<size_t>::iterator it = proc_num_edges.begin(); it != proc_num_edges.end(); ++it) {
            count = count + *it;
        }
        
        logstream(LOG_EMPH) << "TOTAL PROCESSED ELEMENTS: " << count << std::endl;
        
    }

  }; // end of distributed_ob_ingress

}; // end of namespace graphlab
#include <graphlab/macros_undef.hpp>


#endif
