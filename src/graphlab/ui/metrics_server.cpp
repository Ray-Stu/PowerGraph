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

#include <unistd.h>
#include <string>
#include <map>
#include <utility>
#include <sstream>
#include <boost/function.hpp>

#include <graphlab/util/stl_util.hpp>
#include <graphlab/parallel/pthread_tools.hpp>
#include <graphlab/rpc/dc.hpp>
#include <graphlab/rpc/distributed_event_log.hpp>

#include <graphlab/ui/mongoose/mongoose.h>
#include <graphlab/ui/metrics_server.hpp>

#include <graphlab/macros_def.hpp>


namespace graphlab {


static mg_context* metric_context = NULL;

static rwlock& callback_lock() {
  static rwlock clock;
  return clock;
}


static std::map<std::string, http_redirect_callback_type>& callbacks() {
  static std::map<std::string, http_redirect_callback_type> cback;
  return cback;
}




static void* process_request(enum mg_event event,
                             struct mg_connection* conn,
                             const struct mg_request_info* info) {
  if (event == MG_NEW_REQUEST) {

    // get the URL being requested
    std::string url;
    if (info->uri != NULL) url = info->uri;
    // strip the starting /
    if (url.length() >= 1) url = url.substr(1, url.length() - 1);
    // get all the variables
    std::map<std::string, std::string> variable_map;
    if (info->query_string != NULL) {
      std::string qs = info->query_string;
      std::vector<std::string> terms = strsplit(qs, "&", true);
      // now for each term..
      foreach(std::string& term, terms) {
        // get the variable name
        std::vector<std::string> key_val = strsplit(term, "=", true);
        if (key_val.size() > 0) {
          // use mg_get_var to read the actual variable.
          // since mg_get_var does http escape sequence decoding
          std::string key = key_val[0];
          char val_target[8192];
          int ret = mg_get_var(qs.c_str(), qs.length(), 
                               key.c_str(), val_target, 8192);
          if (ret >= 0) variable_map[key] = val_target;
        }
      }
    }
    callback_lock().readlock();
    // now redirect to the callback handlers. if we find one
    std::map<std::string, http_redirect_callback_type>::iterator iter = 
                                                       callbacks().find(url);

    if (iter != callbacks().end()) {
      std::pair<std::string, std::string> returnval = iter->second(variable_map);

      callback_lock().rdunlock();

      std::string ctype = returnval.first;
      std::string body = returnval.second;
      mg_printf(conn,
              "HTTP/1.1 200 OK\r\n"
              "Access-Control-Allow-Origin: *\r\n"
              "Access-Control-Allow-Methods: GET\r\n"
              "Content-Type: %s\r\n"
              "Content-Length: %d\r\n" 
              "\r\n",
              ctype.c_str(), (int) body.length());
      mg_write(conn, body.c_str(), body.length());
    }
    else {
      std::map<std::string, http_redirect_callback_type>::iterator iter404 =
                                                          callbacks().find("404");
      std::pair<std::string, std::string> returnval;
      if (iter404 != callbacks().end()) returnval = iter404->second(variable_map);
      
      callback_lock().rdunlock();

      std::string ctype = returnval.first;
      std::string body = returnval.second;

      mg_printf(conn,
              "HTTP/1.1 404 Not Found\r\n"
              "Access-Control-Allow-Origin: *\r\n"
              "Content-Type: %s\r\n"
              "Content-Length: %d\r\n" 
              "\r\n",
              ctype.c_str(), (int)body.length());
      mg_write(conn, body.c_str(), body.length());
    }

    return (void*)"";
  }
  else {
    return NULL;
  }
}


/*
   Simple 404 handler. Just reuturns a string "Page Not Found"
   */
std::pair<std::string, std::string> 
four_oh_four(std::map<std::string, std::string>& varmap) {
  return std::make_pair(std::string("text/html"), 
                        std::string("Page Not Found"));
}


/*
  Echo handler. Returns a html with get keys and values
 */
std::pair<std::string, std::string> 
echo(std::map<std::string, std::string>& varmap) {
  std::stringstream ret;
  std::map<std::string, std::string>::iterator iter = varmap.begin();
  ret << "<html>\n";
  while (iter != varmap.end()) {
    ret << iter->first << " = " << iter->second << "<br>\n"; 
    ++iter;
  }
  ret << "</html>\n";
  ret.flush();
  return std::make_pair(std::string("text/html"), ret.str());
}

std::pair<std::string, std::string> 
index_page(std::map<std::string, std::string>& varmap) {
  std::stringstream ret;
  ret << "<html>\n";
  ret << "<h3>Registered Handlers:</h3>\n";
  callback_lock().readlock();
  std::map<std::string, http_redirect_callback_type>::const_iterator iter = 
                            callbacks().begin();
  while (iter != callbacks().end()) {
    // don't put in the index page callback
    if (iter->first != "") {
      ret << iter->first << "<br>\n";
    }
    ++iter;
  }
  callback_lock().rdunlock();
  ret << "</html>\n";
  ret.flush();
  return std::make_pair(std::string("text/html"), ret.str());
}


static void fill_builtin_callbacks() {
  callbacks()["404"] = four_oh_four;
  callbacks()["echo"] = echo;
  callbacks()[""] = index_page;
  callbacks()["index.html"] = index_page;
}


void add_metric_server_callback(std::string page, 
                                http_redirect_callback_type callback) {
  callback_lock().writelock();
  callbacks()[page] = callback;
  callback_lock().wrunlock();
}

void launch_metric_server() {
  if (distributed_control::get_instance_procid() == 0) {
    const char *options[] = {"listening_ports", "8090", NULL};
    metric_context = mg_start(process_request, (void*)(&(callbacks())), options);
    if(metric_context == NULL) {
      logstream(LOG_ERROR) << "Unable to launch metrics server on port 8090. "
                           << "Metrics server will not be available" << std::endl;
      return;
    }
    fill_builtin_callbacks();

    char hostname[1024];
    std::string strhostname;
    if (gethostname(hostname, 1024) == 0) strhostname = hostname;
    logstream(LOG_EMPH) << "Metrics server now listening on " 
                   << "http://" << strhostname << ":8090" << std::endl;
  }
}

void stop_metric_server() {
  if (distributed_control::get_instance_procid() == 0 && metric_context != NULL) {
    std::cout << "Metrics server stopping." << std::endl;
    mg_stop(metric_context);
  }
}

void stop_metric_server_on_eof() {
  if (distributed_control::get_instance_procid() == 0 && metric_context != NULL) {
    char buff[128];
    // wait for ctrl-d
    logstream(LOG_EMPH) << "Hit Ctrl-D to stop the metrics server" << std::endl;
    while (fgets(buff, 128, stdin) != NULL );
    stop_metric_server();  
  }
}

} // namespace graphlab
