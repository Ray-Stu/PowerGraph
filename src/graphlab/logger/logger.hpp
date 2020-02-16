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


/**
 * @file logger.hpp
 * Usage:
 * First include logger.hpp. To logger, use the logger() function
 * There are 2 output levels. A "soft" output level which is
 * set by calling global_logger.set_log_level(), as well as a "hard" output
 * level OUTPUTLEVEL which is set in the source code (logger.h).
 *
 * when you call "logger()" with a loglevel and if the loglevel is greater than
 * both of the output levels, the string will be written.
 * written to a logger file. Otherwise, logger() has no effect.
 *
 * The difference between the hard level and the soft level is that the
 * soft level can be changed at runtime, while the hard level optimizes away
 * logging calls at compile time.
 */

#ifndef GRAPHLAB_LOG_LOG_HPP
#define GRAPHLAB_LOG_LOG_HPP
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <cassert>
#include <cstring>
#include <cstdarg>
#include <pthread.h>
#include <graphlab/util/timer.hpp>
#include <graphlab/logger/fail_method.hpp>
#include <graphlab/logger/backtrace.hpp>

/**
 * \def LOG_FATAL
 *   Used for fatal and probably irrecoverable conditions
 * \def LOG_ERROR
 *   Used for errors which are recoverable within the scope of the function
 * \def LOG_WARNING
 *   Logs interesting conditions which are probably not fatal
 * \def LOG_EMPH
 *   Outputs as LOG_INFO, but in LOG_WARNING colors. Useful for
 *   outputting information you want to emphasize.
 * \def LOG_INFO
 *   Used for providing general useful information
 * \def LOG_DEBUG
 *   Debugging purposes only
 * \def LOG_EVERYTHING
 *   Log everything 
 */
// sgr - needed additional debug levels. I can undo this change if 
// necessary. although it seems to me that log levels should count 
// up and saturate so the messages label array can always be used.
#define LOG_NONE 7
#define LOG_FATAL 6
#define LOG_ERROR 5
#define LOG_WARNING 4
#define LOG_EMPH 3
#define LOG_INFO 2
#define LOG_DEBUG 1
#define LOG_EVERYTHING 0 // technically unsigned int

/**
 * \def OUTPUTLEVEL
 *  The minimum level to logger at
 * \def LOG_NONE
 *  OUTPUTLEVEL to LOG_NONE to disable logging
 */

#ifndef OUTPUTLEVEL
#define OUTPUTLEVEL LOG_DEBUG
#endif
/// If set, logs to screen will be printed in color
#define COLOROUTPUT


/**
 * \def logger(lvl,fmt,...)
 *    extracts the filename, line number
 *     and function name and calls _log. It will be optimized
 *     away if LOG_NONE is set
 *     This relies on a few compiler macros. As far as I know, these
 *     macros are pretty standard among most other C++ compilers.
 */
#if OUTPUTLEVEL == LOG_NONE
// totally disable logging
#define logger(lvl,fmt,...)
#define logbuf(lvl,fmt,...)
#define logstream(lvl) if(0) null_stream()

#define logger_once(lvl,fmt,...)
#define logstream_once(lvl) if(0) null_stream()

#define logger_ontick(sec,lvl,fmt,...)
#define logstream_ontick(sec, lvl) if(0) null_stream()

#else

#define logger(lvl,fmt,...)                 \
    (log_dispatch<(lvl >= OUTPUTLEVEL)>::exec(lvl,__FILE__, __func__ ,__LINE__,fmt,##__VA_ARGS__))


#define logbuf(lvl,buf,len)                 \
    (log_dispatch<(lvl >= OUTPUTLEVEL)>::exec(lvl,__FILE__,     \
                        __func__ ,__LINE__,buf,len))

#define logstream(lvl)                      \
    if(lvl >= global_logger().get_log_level()) (log_stream_dispatch<(lvl >= OUTPUTLEVEL)>::exec(lvl,__FILE__, __func__ ,__LINE__) )

#define logger_once(lvl,fmt,...)                 \
{    \
  static bool __printed__ = false;    \
  if (!__printed__) {                 \
    __printed__ = true;               \
    (log_dispatch<(lvl >= OUTPUTLEVEL)>::exec(lvl,__FILE__, __func__ ,__LINE__,fmt,##__VA_ARGS__)); \
  }  \
}

#define logstream_once(lvl)                      \
(*({    \
  static bool __printed__ = false;    \
  bool __prev_printed__ = __printed__; \
  if (!__printed__) __printed__ = true;  \
  &(log_stream_dispatch<(lvl >= OUTPUTLEVEL)>::exec(lvl,__FILE__, __func__ ,__LINE__, !__prev_printed__) ); \
}))

#define logger_ontick(sec,lvl,fmt,...)                 \
{    \
  static float last_print = -sec - 1;        \
  float curtime = graphlab::timer::approx_time_seconds(); \
  if (last_print + sec <= curtime) {                 \
    last_print = curtime;                     \
    (log_dispatch<(lvl >= OUTPUTLEVEL)>::exec(lvl,__FILE__, __func__ ,__LINE__,fmt,##__VA_ARGS__)); \
  }  \
}

#define logstream_ontick(sec,lvl)                      \
(*({    \
  static float last_print = -sec - 1;        \
  float curtime = graphlab::timer::approx_time_seconds();        \
  bool print_now = false;             \
  if (last_print + sec <= curtime) {                 \
    last_print = curtime;                 \
    print_now = true;                \
  }                    \
  &(log_stream_dispatch<(lvl >= OUTPUTLEVEL)>::exec(lvl,__FILE__, __func__ ,__LINE__, print_now) ); \
}))


#endif

namespace logger_impl {
struct streambuff_tls_entry {
  std::stringstream streambuffer;
  bool streamactive;
};
}


extern void __print_back_trace();

/**
  logging class.
  This writes to a file, and/or the system console.
*/
class file_logger{
 public:
  /** Default constructor. By default, log_to_console is on,
      there is no logger file, and logger level is set to LOG_EMPH
  */
  file_logger();

  ~file_logger();   /// destructor. flushes and closes the current logger file

  /** Closes the current logger file if one exists.
      if 'file' is not an empty string, it will be opened and
      all subsequent logger output will be written into 'file'.
      Any existing content of 'file' will be cleared.
      Return true on success and false on failure.
  */
  bool set_log_file(std::string file);

  /// If consolelog is true, subsequent logger output will be written to stderr
  void set_log_to_console(bool consolelog) {
    log_to_console = consolelog;
  }

  /// Returns the current logger file.
  std::string get_log_file(void) {
    return log_file;
  }

  /// Returns true if output is being written to stderr
  bool get_log_to_console() {
    return log_to_console;
  }

  /// Returns the current logger level
  int get_log_level() {
    return log_level;
  }

  file_logger& start_stream(int lineloglevel,const char* file,const char* function, int line, bool do_start = true);

  template <typename T>
  file_logger& operator<<(T a) {
    // get the stream buffer
    logger_impl::streambuff_tls_entry* streambufentry = reinterpret_cast<logger_impl::streambuff_tls_entry*>(
                                          pthread_getspecific(streambuffkey));
    if (streambufentry != NULL) {
      std::stringstream& streambuffer = streambufentry->streambuffer;
      bool& streamactive = streambufentry->streamactive;

      if (streamactive) streambuffer << a;
    }
    return *this;
  }

  inline file_logger& operator<<(const char* a) {
    // get the stream buffer
    logger_impl::streambuff_tls_entry* streambufentry = reinterpret_cast<logger_impl::streambuff_tls_entry*>(
                                          pthread_getspecific(streambuffkey));
    if (streambufentry != NULL) {
      std::stringstream& streambuffer = streambufentry->streambuffer;
      bool& streamactive = streambufentry->streamactive;

      if (streamactive) {
        streambuffer << a;
        if (a[strlen(a)-1] == '\n') {
          stream_flush();
        }
      }
    }
    return *this;
  }

  inline file_logger& operator<<(std::ostream& (*f)(std::ostream&)){
    // get the stream buffer
    logger_impl::streambuff_tls_entry* streambufentry = reinterpret_cast<logger_impl::streambuff_tls_entry*>(
                                          pthread_getspecific(streambuffkey));
    if (streambufentry != NULL) {
      std::stringstream& streambuffer = streambufentry->streambuffer;
      bool& streamactive = streambufentry->streamactive;

      typedef std::ostream& (*endltype)(std::ostream&);
      if (streamactive) {
        if (endltype(f) == endltype(std::endl)) {
          streambuffer << "\n";
          stream_flush();
          if(streamloglevel == LOG_FATAL) {
            __print_back_trace();
            GRAPHLAB_LOGGER_FAIL_METHOD("LOG_FATAL encountered");
          }
        }
      }
    }
    return *this;
  }



  /** Sets the current logger level. All logging commands below the current
      logger level will not be written. */
  void set_log_level(int new_log_level) {
    log_level = new_log_level;
  }

  /**
  * logs the message if loglevel>=OUTPUTLEVEL
  * This function should not be used directly. Use logger()
  *
  * @param loglevel Type of message \see LOG_DEBUG LOG_INFO LOG_WARNING LOG_ERROR LOG_FATAL
  * @param file File where the logger call originated
  * @param function Function where the logger call originated
  * @param line Line number where the logger call originated
  * @param fmt printf format string
  * @param arg var args. The parameters that match the format string
  */
  void _log(int loglevel,const char* file,const char* function,
                int line,const char* fmt, va_list arg );


  void _logbuf(int loglevel,const char* file,const char* function,
                int line,  const char* buf, int len);

  void _lograw(int loglevel, const char* buf, int len);

  inline void stream_flush() {
    // get the stream buffer
    logger_impl::streambuff_tls_entry* streambufentry = reinterpret_cast<logger_impl::streambuff_tls_entry*>(
                                          pthread_getspecific(streambuffkey));
    if (streambufentry != NULL) {
      std::stringstream& streambuffer = streambufentry->streambuffer;

      streambuffer.flush();
      _lograw(streamloglevel,
              streambuffer.str().c_str(),
              (int)(streambuffer.str().length()));
      streambuffer.str("");
    }
  }
 private:
  std::ofstream fout;
  std::string log_file;

  pthread_key_t streambuffkey;

  int streamloglevel;
  pthread_mutex_t mut;

  bool log_to_console;
  int log_level;

};


file_logger& global_logger();

/**
Wrapper to generate 0 code if the output level is lower than the log level
*/
template <bool dostuff>
struct log_dispatch {};

template <>
struct log_dispatch<true> {
  inline static void exec(int loglevel,const char* file,const char* function,
                          int line,const char* fmt, ... ) {
    va_list argp;
    va_start(argp, fmt);
    global_logger()._log(loglevel, file, function, line, fmt, argp);
    va_end(argp);
    if(loglevel == LOG_FATAL) {
      __print_back_trace();
      GRAPHLAB_LOGGER_FAIL_METHOD("LOG_FATAL encountered");
    }
  }
};

template <>
struct log_dispatch<false> {
  inline static void exec(int loglevel,const char* file,const char* function,
                int line,const char* fmt, ... ) {}
};


struct null_stream {
  template<typename T>
  inline null_stream operator<<(T t) { return null_stream(); }
  inline null_stream operator<<(const char* a) { return null_stream(); }
  inline null_stream operator<<(std::ostream& (*f)(std::ostream&)) { return null_stream(); }
};


template <bool dostuff>
struct log_stream_dispatch {};

template <>
struct log_stream_dispatch<true> {
  inline static file_logger& exec(int lineloglevel,const char* file,const char* function, int line, bool do_start = true) {
    return global_logger().start_stream(lineloglevel, file, function, line, do_start);
  }
};

template <>
struct log_stream_dispatch<false> {
  inline static null_stream exec(int lineloglevel,const char* file,const char* function, int line, bool do_start = true) {
    return null_stream();
  }
};

void textcolor(FILE* handle, int attr, int fg);
void reset_color(FILE* handle);

#endif

