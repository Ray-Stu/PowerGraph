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


#include <limits>
#include <string>
#include <graphlab/logger/assertions.hpp>
#include <graphlab/util/tracepoint.hpp>
#include <graphlab/parallel/pthread_tools.hpp>
#include <boost/unordered_map.hpp>


namespace graphlab {

void trace_count::print(std::ostream& out, unsigned long long tpersec) const {
  if (tpersec == 0) {
    out << name << ": " << description << "\n";
    out << "Events:\t" << count.value << "\n";
    out << "Total:\t" << total.value << "ticks \n";
    if (count.value > 0) {
      out << "Mean:\t" << (double)total.value / count.value << "ticks \n";
      out << "Min:\t" << minimum << "ticks \n";
      out << "Max:\t" << maximum << "ticks \n";
    }
  }
  else {
    double tperms = (double)tpersec / 1000;
    out << name << ": " << description << "\n";
    out << "Events:\t" << count.value << "\n";
    out << "Total:\t" << (double)total.value / tperms << " ms \n";
    if (count.value > 0) {
      out << "Mean:\t" << (double)total.value / count.value / tperms << " ms \n";
      out << "Min:\t" << (double)minimum / tperms << " ms \n";
      out << "Max:\t" << (double)maximum / tperms << " ms \n";
    }
  }
}


static mutex printlock;

trace_count::~trace_count() {
#ifdef USE_TRACEPOINT
  printlock.lock();
  print(std::cout, estimate_ticks_per_second());
  std::cout.flush();
  printlock.unlock();
#endif
}

} // namespace graphlab

