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


#include <graphlab/logger/logger.hpp>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <pthread.h>
#include <graphlab/logger/backtrace.hpp>

file_logger& global_logger() {
  static file_logger l;
  return l;
}




void streambuffdestructor(void* v){
  logger_impl::streambuff_tls_entry* t =
    reinterpret_cast<logger_impl::streambuff_tls_entry*>(v);
  delete t;
}

const char* messages[] = {  "DEBUG:    ",
                            "DEBUG:    ",
                            "INFO:     ",
                            "INFO:     ",
                            "WARNING:  ",
                            "ERROR:    ",
                            "FATAL:    "};


file_logger::file_logger() {
  log_file = "";
  log_to_console = true;
  log_level = LOG_EMPH;
  pthread_mutex_init(&mut, NULL);
  pthread_key_create(&streambuffkey, streambuffdestructor);
}

file_logger::~file_logger() {
  if (fout.good()) {
    fout.flush();
    fout.close();
  }

  pthread_mutex_destroy(&mut);
}

bool file_logger::set_log_file(std::string file) {
  // close the file if it is open
  if (fout.good()) {
    fout.flush();
    fout.close();
    log_file = "";
  }
  // if file is not an empty string, open the new file
  if (file.length() > 0) {
    fout.open(file.c_str());
    if (fout.fail()) return false;
    log_file = file;
  }
  return true;
}



#define RESET   0
#define BRIGHT    1
#define DIM   2
#define UNDERLINE   3
#define BLINK   4
#define REVERSE   7
#define HIDDEN    8

#define BLACK     0
#define RED   1
#define GREEN   2
#define YELLOW    3
#define BLUE    4
#define MAGENTA   5
#define CYAN    6
#define WHITE   7

void textcolor(FILE* handle, int attr, int fg)
{
  char command[13];
  /* Command is the control command to the terminal */
  sprintf(command, "%c[%d;%dm", 0x1B, attr, fg + 30);
  fprintf(handle, "%s", command);
}

void reset_color(FILE* handle)
{
  char command[20];
  /* Command is the control command to the terminal */
  sprintf(command, "%c[0m", 0x1B);
  fprintf(handle, "%s", command);
}



void file_logger::_log(int lineloglevel,const char* file,const char* function,
                       int line,const char* fmt, va_list ap ){
  // if the logger level fits
  if (lineloglevel >= log_level){
    // get just the filename. this line found on a forum on line.
    // claims to be from google.
    file = ((strrchr(file, '/') ? : file- 1) + 1);

    char str[1024];

    // write the actual header
    int byteswritten = snprintf(str,1024, "%s%s(%s:%d): ",
                                messages[lineloglevel],file,function,line);
    // write the actual logger

    byteswritten += vsnprintf(str + byteswritten,1024 - byteswritten,fmt,ap);

    str[byteswritten] = '\n';
    str[byteswritten+1] = 0;
    // write the output
    if (fout.good()) {
      pthread_mutex_lock(&mut);
      fout << str;;
      pthread_mutex_unlock(&mut);
    }
    if (log_to_console) {
#ifdef COLOROUTPUT
      if (lineloglevel == LOG_FATAL) {
        textcolor(stderr, BRIGHT, RED);
      }
      else if (lineloglevel == LOG_ERROR) {
        textcolor(stderr, BRIGHT, RED);
      }
      else if (lineloglevel == LOG_WARNING) {
        textcolor(stderr, BRIGHT, MAGENTA);
      }
      else if (lineloglevel == LOG_EMPH) {
        textcolor(stderr, BRIGHT, GREEN);
      }
#endif
      std::cerr << str;;
#ifdef COLOROUTPUT
      reset_color(stderr);
#endif
    }
  }
}



void file_logger::_logbuf(int lineloglevel,const char* file,const char* function,
                          int line,const char* buf, int len) {
  // if the logger level fits
  if (lineloglevel >= log_level){
    // get just the filename. this line found on a forum on line.
    // claims to be from google.
    file = ((strrchr(file, '/') ? : file- 1) + 1);

    // length of the 'head' of the string
    size_t headerlen = snprintf(NULL,0,"%s%s(%s:%d): ",
                                messages[lineloglevel],file,function,line);

    if (headerlen> 2047) {
      std::cerr << "Header length exceed buffer length!";
    }
    else {
      char str[2048];
      const char *newline="\n";
      // write the actual header
      int byteswritten = snprintf(str,2047,"%s%s(%s:%d): ",
                                  messages[lineloglevel],file,function,line);
      _lograw(lineloglevel,str, byteswritten);
      _lograw(lineloglevel,buf, len);
      _lograw(lineloglevel,newline, (int)strlen(newline));
    }
  }
}

void file_logger::_lograw(int lineloglevel, const char* buf, int len) {
  if (fout.good()) {
    pthread_mutex_lock(&mut);
    fout.write(buf,len);
    pthread_mutex_unlock(&mut);
  }
  if (log_to_console) {
#ifdef COLOROUTPUT
    pthread_mutex_lock(&mut);
    if (lineloglevel == LOG_FATAL) {
      textcolor(stderr, BRIGHT, RED);
    }
    else if (lineloglevel == LOG_ERROR) {
      textcolor(stderr, BRIGHT, RED);
    }
    else if (lineloglevel == LOG_WARNING) {
      textcolor(stderr, BRIGHT, MAGENTA);
    }
    else if (lineloglevel == LOG_DEBUG) {
      textcolor(stderr, BRIGHT, YELLOW);
    }
    else if (lineloglevel == LOG_EMPH) {
      textcolor(stderr, BRIGHT, GREEN);
    }
#endif
    std::cerr.write(buf,len);
#ifdef COLOROUTPUT

    pthread_mutex_unlock(&mut);
    reset_color(stderr);
#endif
  }
}

file_logger& file_logger::start_stream(int lineloglevel,const char* file,
                                       const char* function, int line, bool do_start) {
  // get the stream buffer
  logger_impl::streambuff_tls_entry* streambufentry =
        reinterpret_cast<logger_impl::streambuff_tls_entry*>(
                              pthread_getspecific(streambuffkey));
  // create the key if it doesn't exist
  if (streambufentry == NULL) {
    streambufentry = new logger_impl::streambuff_tls_entry;
    pthread_setspecific(streambuffkey, streambufentry);
  }
  std::stringstream& streambuffer = streambufentry->streambuffer;
  bool& streamactive = streambufentry->streamactive;

  if (lineloglevel >= log_level){
    // get the stream buffer
    // if do not start the stream, just quit
    if (do_start == false) {
      streamactive = false;
      return *this;
    }

    file = ((strrchr(file, '/') ? : file- 1) + 1);

    if (streambuffer.str().length() == 0) {
      streambuffer << messages[lineloglevel] << file
                   << "(" << function << ":" <<line<<"): ";
    }
    streamactive = true;
    streamloglevel = lineloglevel;
  } else {
    streamactive = false;
  }
  return *this;
}

