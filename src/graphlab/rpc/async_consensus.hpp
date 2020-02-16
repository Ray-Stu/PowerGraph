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


#ifndef ASYNC_TERMINATOR_HPP
#define ASYNC_TERMINATOR_HPP

#include <graphlab/parallel/pthread_tools.hpp>

#include <graphlab/rpc/dc.hpp>
#include <graphlab/rpc/dc_dist_object_base.hpp>
#include <graphlab/rpc/dc_dist_object.hpp>


namespace graphlab {
  /**
   * \ingroup rpc
   * \brief This implements a distributed consensus algorithm which waits
   * for global completion of all computation/RPC events on a given object.
   *
   * The use case is as follows:
   * 
   * A collection of threads on a collection of distributed machines, each
   * running the following
   * 
   * \code
   * while (work to be done) {
   *    Do_stuff
   * }
   * \endcode
   * 
   * However, <tt>Do_stuff</tt> will include RPC calls which may introduce
   * work to other threads/machines.
   * Figuring out when termination is possible is complex. For instance RPC calls 
   * could be in-flight and not yet received. This async_consensus class 
   * implements a solution built around the algorithm in
   * <i>Misra, J.: Detecting Termination of Distributed Computations Using Markers, SIGOPS, 1983</i>
   * extended to handle the mixed parallelism (distributed with threading) case.
   * 
   * The main loop of the user has to be modified to:
   * 
   * \code
   * done = false;
   * while (!done) {
   *    Do_stuff
   *    // if locally, I see that there is no work to be done
   *    // we begin the consensus checking
   *    if (no work to be done) {
   *      // begin the critical section and try again
   *      consensus.begin_done_critical_section();
   *      // if still no work to be done
   *      if (no work to be done) {
   *        done = consensus.end_done_critical_section();
   *      }
   *      else {
   *        consensus.cancel_critical_section();
   *      }
   *    }
   * }
   * 
   * \endcode
   * 
   * Additionally, incoming RPC calls which create work must ensure there are
   * active threads which are capable of processing the work. An easy solution 
   * will be to simply cancel_one(). Other more optimized solutions
   * include keeping a counter of the number of active threads, and only calling
   * cancel() or cancel_one() if all threads are asleep. (Note that the optimized
   * solution does require some care to ensure dead-lock free execution).
   *
   * The async_consensus works with regular kernel threads. See 
   * \ref graphlab::fiber_async_consensus for a version which works with
   * fibers.
   *
   * \see graphlab::fiber_async_consensus
   */
  class async_consensus {
  public:
    /** \brief Constructs an asynchronous consensus object
      *
      * The consensus procedure waits till all threads have no work to do and are 
      * waiting in consensus, and there is no communication going on which
      * could wake up a thread. The consensus object is therefore associated
      * with a communication context, either a graphlab::dc_dist_object,
      * or the global context (the root distributed_control).
      * 
      * \param dc The distributed control object to use for communication
      * \param required_threads_in_done local consensus is achieved if this many
      *                                 threads are waiting for consensus locally.
      * \param attach The context to associate with. If NULL, we associate with
      *               the global context. 
      */
    async_consensus(distributed_control &dc, size_t required_threads_in_done = 1,
                    const dc_impl::dc_dist_object_base* attach = NULL);


    /**
     * \brief A thread enters the critical section by calling
     * this function. 
     *  
     * After which it should check its termination 
     * condition locally. If termination condition
     * is still fulfilled, end_done_critical_section() should be called.
     * Otherwise cancel_critical_section() should be called.
     *
     * \param cpuid Thread ID of the thread.
     */
    void begin_done_critical_section(size_t cpuid);

    /**
     * \brief Leaves the critical section because termination condition
     * is not fullfilled.
     *
     * See begin_done_critical_section()
     * \param cpuid Thread ID of the thread.
     */
    void cancel_critical_section(size_t cpuid);

    /**
     * \brief Thread must call this function if termination condition
     * is fullfilled while in the critical section. 
     *
     * See begin_done_critical_section()
     * 
     * \param cpuid Thread ID of the thread.
     * \returns True if global consensus is achieved. False otherwise. 
     */
    bool end_done_critical_section(size_t cpuid);

    /**
     * \brief Forces the consensus to be set
     */
    void force_done();
  
    
    /// \brief Wakes up all local threads waiting in done()
    void cancel();

    /// \brief Wakes up a specific thread waiting in done()
    void cancel_one(size_t cpuid);

    /// \brief Returns true if consensus is achieved.
    bool is_done() const {
      return done;
    }
    /** \brief Resets the consensus object. Must be called simultaneously by
     * exactly one thread on each machine.
     * This function is not safe to call while consensus is being achieved.
     */
    void reset();
 
  private:

    /**
     * The token object that is passed around the machines.
     * It counts the total number of RPC calls that has been sent
     * or received, as well as the machine which last changed the value.
     * When the token goes one full round with no change, consensus is
     * achieved.
     */
    struct token {
      size_t total_calls_sent;
      size_t total_calls_received;
      procid_t last_change;
      void save(oarchive &oarc) const {
        oarc << total_calls_sent << total_calls_received << last_change;
      }

      void load(iarchive &iarc) {
        iarc >> total_calls_sent >> total_calls_received >> last_change;
      }
    };

    
    dc_dist_object<async_consensus> rmi;
    const dc_impl::dc_dist_object_base* attachedobj;
  
    size_t last_calls_sent;
    size_t last_calls_received;

 
    
    /// counts the number of threads which are not sleeping
    /// protected by the mutex
    size_t numactive; 
    
    /// Total number of CPUs
    size_t ncpus;
    
    /// once flag is set, the terminator is invalid, and all threads
    /// should leave
    bool done;
    
    /// set if abort() is called
    //  bool forced_abort;    
    
    /// Number of threads which have called
    /// begin_critical_section(), and have not left end_critical_section()
    /// This is an atomic counter and is not protected.
    atomic<size_t> trying_to_sleep;
    
    /// critical[i] is set if thread i has called 
    /// begin_critical_section(), but has not left end_critical_section()
    /// sum of critical should be the same as trying_to_sleep
    std::vector<char> critical;
    
    /// sleeping[i] is set if threads[i] is in cond.wait()
    std::vector<char> sleeping;
    
    
    bool hastoken;
    /// If I have the token, the value of the token
    token cur_token;

    mutex m;
    std::vector<conditional> cond;
      

    void receive_the_token(token &tok);
    void pass_the_token();
  };

}
#endif



