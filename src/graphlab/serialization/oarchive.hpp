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


// This file should not be included directly. use serialize.hpp
#ifndef GRAPHLAB_SERIALIZE_HPP
#include <graphlab/serialization/serialize.hpp>

#else

#ifndef GRAPHLAB_OARCHIVE_HPP
#define GRAPHLAB_OARCHIVE_HPP

#include <iostream>
#include <string>
#include <graphlab/logger/assertions.hpp>
#include <graphlab/serialization/is_pod.hpp>
#include <graphlab/serialization/has_save.hpp>
#include <graphlab/util/branch_hints.hpp>
namespace graphlab {

  /**
   * \ingroup group_serialization
   * \brief The serialization output archive object which, provided
   * with a reference to an ostream, will write to the ostream,
   * providing serialization capabilities.
   *
   * Given a standard output stream, you can construct an oarchive
   * object by:
   * \code
   *   // where strm is an ostream object
   *   graphlab::oarchive oarc(strm);
   * \endcode
   *
   * For instance, to serialize to a file,
   * \code
   *   std::ofstream fout("outputfile.bin");
   *   graphlab::oarchive oarc(fout);
   * \endcode
   *
   * Once the oarc object is constructed, \ref sec_serializable objects can be
   * written to it using the << stream operator.
   *
   * \code
   *    oarc << a << b << c;
   * \endcode
   *
   * Alternatively, data can be directly written to the stream
   * using the oarchive::write() function.
   *
   * Written data can be deserialized using graphlab::iarchive.
   * For more usage details, see \ref serialization
   *
   * The oarchive object should not be used once the associated stream
   * object is closed or is destroyed.
   *
   * The oarc object
   * does <b> not </b> flush the associated stream, and the user may need to
   * manually flush the associated stream to clear any stream buffers.
   * For instance, while the std::stringstream may be used for both output
   * and input, it is necessary to flush the stream before all bytes written to
   * the stringstream are available for input.
   *
   * To use this class, include
   * graphlab/serialization/serialization_includes.hpp
   */
  class oarchive{
  public:
    std::ostream* out;
    char* buf;
    size_t off;
    size_t len;
    /// constructor. Takes a generic std::ostream object
    inline oarchive(std::ostream& outstream)
      : out(&outstream),buf(NULL),off(0),len(0) {}

    inline oarchive(void)
      : out(NULL),buf(NULL),off(0),len(0) {}

    inline void expand_buf(size_t s) {
        if (__unlikely__(off + s > len)) {
          len = 2 * (s + len);
          buf = (char*)realloc(buf, len);
        }
     }
    /** Directly writes "s" bytes from the memory location
     * pointed to by "c" into the stream.
     */
    inline void write(const char* c, std::streamsize s) {
      if (out == NULL) {
        expand_buf(s);
        memcpy(buf + off, c, s);
        off += s;
      } else {
        out->write(c, s);
      }
    }
    template <typename T>
    inline void direct_assign(const T& t) {
      if (out == NULL) {
        expand_buf(sizeof(T));
        (*reinterpret_cast<T*>(buf + off)) = t;
        off += sizeof(T);
      }
      else {
        out->write(reinterpret_cast<const char*>(&t), sizeof(T));
      }
    }

    inline void advance(size_t s) {
      if (out == NULL) {
        expand_buf(s);
        off += s;
      } else {
        out->seekp(s, std::ios_base::cur);
      }
    }

    /// Returns true if the underlying stream is in a failure state
    inline bool fail() {
      return out == NULL ? false : out->fail();
    }

    inline ~oarchive() { }
  };

  /**
   * \ingroup group_serialization
   * \brief
   * When this archive is used to serialize an object,
   * and the object does not support serialization,
   * failure will only occur at runtime. Otherwise equivalent to
   * graphlab::oarchive
   */
  class oarchive_soft_fail {
  public:
    oarchive* oarc;
    bool mine;

    /// constructor. Takes a generic std::ostream object
    inline oarchive_soft_fail(std::ostream& outstream)
      : oarc(new oarchive(outstream)), mine(true) { }

    inline oarchive_soft_fail(oarchive& oarc):oarc(&oarc), mine(false) {
    }

    inline oarchive_soft_fail(void)
      : oarc(new oarchive) {}

    /** Directly writes "s" bytes from the memory location
     * pointed to by "c" into the stream.
     */
    inline void write(const char* c, std::streamsize s) {
      oarc->write(c, s);
    }
    template <typename T>
    inline void direct_assign(const T& t) {
      oarc->direct_assign(t);
    }

    inline bool fail() {
      return oarc->fail();
    }

    inline ~oarchive_soft_fail() {
     if (mine) delete oarc;
    }
  };

  namespace archive_detail {

    /// called by the regular archive The regular archive will do a hard fail
    template <typename OutArcType, typename T>
    struct serialize_hard_or_soft_fail {
      inline static void exec(OutArcType& oarc, const T& t) {
        t.save(oarc);
      }
    };

    /// called by the soft fail archive
    template <typename T>
    struct serialize_hard_or_soft_fail<oarchive_soft_fail, T> {
      inline static void exec(oarchive_soft_fail& oarc, const T& t) {
        // create a regular oarchive and
        // use the save_or_fail function which will
        // perform a soft fail
        save_or_fail(*(oarc.oarc), t);
      }
    };


    /**
       Implementation of the serializer for different types.
       This is the catch-all. If it gets here, it must be a non-POD and is a class.
       We therefore call the .save function.
       Here we pick between the archive types using serialize_hard_or_soft_fail
    */
    template <typename OutArcType, typename T, bool IsPOD>
    struct serialize_impl {
      static void exec(OutArcType& oarc, const T& t) {
        serialize_hard_or_soft_fail<OutArcType, T>::exec(oarc, t);
      }
    };

    /** Catch if type is a POD */
    template <typename OutArcType, typename T>
    struct serialize_impl<OutArcType, T, true> {
      inline static void exec(OutArcType& oarc, const T& t) {
        oarc.direct_assign(t);
        //oarc.write(reinterpret_cast<const char*>(&t), sizeof(T));
      }
    };

    /**
       Re-dispatch if for some reasons T already has a const
    */
    template <typename OutArcType, typename T>
    struct serialize_impl<OutArcType, const T, true> {
      inline static void exec(OutArcType& oarc, const T& t) {
        serialize_impl<OutArcType, T, true>::exec(oarc, t);
      }
    };

    /**
       Re-dispatch if for some reasons T already has a const
    */
    template <typename OutArcType, typename T>
    struct serialize_impl<OutArcType, const T, false> {
      inline static void exec(OutArcType& oarc, const T& t) {
        serialize_impl<OutArcType, T, false>::exec(oarc, t);
      }
    };
  }// archive_detail


  /// \cond GRAPHLAB_INTERNAL

  /**
     Overloads the operator<< in the oarchive to
     allow the use of the stream syntax for serialization.
     It simply re-dispatches into the serialize_impl classes
  */
  template <typename T>
  inline oarchive& operator<<(oarchive& oarc, const T& t) {
    archive_detail::serialize_impl<oarchive,
                                   T,
                                   gl_is_pod<T>::value >::exec(oarc, t);
    return oarc;
  }

  /**
     Overloads the operator<< in the oarchive_soft_fail to
     allow the use of the stream syntax for serialization.
     It simply re-dispatches into the serialize_impl classes
  */
  template <typename T>
  inline oarchive_soft_fail& operator<<(oarchive_soft_fail& oarc,
                                        const T& t) {
    archive_detail::serialize_impl<oarchive_soft_fail,
                                  T,
                                  gl_is_pod<T>::value >::exec(oarc, t);
    return oarc;
  }


  /**
     Serializes an arbitrary pointer + length to an archive
  */
  inline oarchive& serialize(oarchive& oarc,
                             const void* str,
                             const size_t length) {
    // save the length
    oarc.write(reinterpret_cast<const char*>(str),
                    (std::streamsize)length);
    assert(!oarc.fail());
    return oarc;
  }


  /**
     Serializes an arbitrary pointer + length to an archive
  */
  inline oarchive_soft_fail& serialize(oarchive_soft_fail& oarc,
                                       const void* str,
                                       const size_t length) {
    // save the length
    oarc.write(reinterpret_cast<const char*>(str),
                    (std::streamsize)length);
    assert(!oarc.fail());
    return oarc;
  }

  /// \endcond GRAPHLAB_INTERNAL

}
  /**
     \ingroup group_serialization
     \brief Macro to make it easy to define out-of-place saves

     In the event that it is impractical to implement a save() and load()
     function in the class one wnats to serialize, it is necessary to define
     an "out of save" save and load.

     See \ref sec_serializable_out_of_place for an example

     \note important! this must be defined in the global namespace!
  */
#define BEGIN_OUT_OF_PLACE_SAVE(arc, tname, tval)                       \
  namespace graphlab{ namespace archive_detail {                        \
  template <typename OutArcType> struct serialize_impl<OutArcType, tname, false> { \
  static void exec(OutArcType& arc, const tname & tval) {

#define END_OUT_OF_PLACE_SAVE() } }; } }


#endif

#endif









