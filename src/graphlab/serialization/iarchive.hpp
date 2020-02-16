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


#ifndef GRAPHLAB_SERIALIZE_HPP
#include <graphlab/serialization/serialize.hpp>

#else


#ifndef GRAPHLAB_IARCHIVE_HPP
#define GRAPHLAB_IARCHIVE_HPP

#include <iostream>
#include <graphlab/logger/assertions.hpp>
#include <graphlab/serialization/is_pod.hpp>
#include <graphlab/serialization/has_load.hpp>
namespace graphlab {

  /**
   * \ingroup group_serialization
   * \brief The serialization input archive object which, provided
   * with a reference to an istream, will read from the istream,
   * providing deserialization capabilities.
   *
   * Given a source of serialized bytes (written by an graphlab::oarchive),
   * in the form of a standard input stream, you can construct an iarchive
   * object by:
   * \code
   *   // where strm is an istream object
   *   graphlab::iarchive iarc(strm);
   * \endcode
   *
   * For instance, to deserialize from a file,
   * \code
   *   std::ifstream fin("inputfile.bin");
   *   graphlab::iarchive iarc(fin);
   * \endcode
   *
   * Once the iarc object is constructed, \ref sec_serializable
   * objects can be read from it using the >> stream operator.
   *
   * \code
   *    iarc >> a >> b >> c;
   * \endcode
   *
   * Alternatively, data can be directly read from the stream using
   * the iarchive::read() and iarchive::read_char() functions.
   *
   * For more usage details, see \ref serialization
   *
   * The iarchive object should not be used once the associated stream
   * object is closed or is destroyed.
   *
   * To use this class, include
   * graphlab/serialization/serialization_includes.hpp
   */
  class iarchive {
  public:
    std::istream* in;
    const char* buf;
    size_t off;
    size_t len;

    /// Directly reads a single character from the input stream
    inline char read_char() {
      char c;
      if (buf) {
        c = buf[off];
        ++off;
      } else {
        in->get(c);
      }
      return c;
    }

    /**
     *  Directly reads a sequence of "len" bytes from the
     *  input stream into the location pointed to by "c"
     */
    inline void read(char* c, size_t l) {
      if (buf) {
        memcpy(c, buf + off, l);
        off += l;
      } else {
        in->read(c, l);
      }
    }


    /// Returns true if the underlying stream is in a failure state
    inline bool fail() {
      return in == NULL ? off > len : in->fail();
    }

    /**
     * Constructs an iarchive object.
     * Takes a reference to a generic std::istream object and associates
     * the archive with it. Reads from the archive will read from the
     * assiciated input stream.
     */
    inline iarchive(std::istream& instream)
      : in(&instream), buf(NULL), off(0), len(0) { }

    inline iarchive(const char* buf, size_t len)
      : in(NULL), buf(buf), off(0), len(len) { }

    ~iarchive() {}
  };


  /**
   * \ingroup group_serialization
   * \brief
   * When this archive is used to deserialize an object,
   * and the object does not support serialization,
   * failure will only occur at runtime. Otherwise equivalent to
   * graphlab::iarchive.
   */
  class iarchive_soft_fail{
  public:

    iarchive *iarc;
    bool mine;

    /// Directly reads a single character from the input stream
    inline char read_char() {
      return iarc->read_char();
    }

    /**
     *  Directly reads a sequence of "len" bytes from the
     *  input stream into the location pointed to by "c"
     */
    inline void read(char* c, size_t len) {
      iarc->read(c, len);
    }

    /// Returns true if the underlying stream is in a failure state
    inline bool fail() {
      return iarc->fail();
    }

    /**
     * Constructs an iarchive_soft_fail object.
     * Takes a reference to a generic std::istream object and associates
     * the archive with it. Reads from the archive will read from the
     * assiciated input stream.
     */
    inline iarchive_soft_fail(std::istream &instream)
      : iarc(new iarchive(instream)), mine(true) {}

    /**
     * Constructs an iarchive_soft_fail object from an iarchive.
     * Both will share the same input stream
     */
    inline iarchive_soft_fail(iarchive &iarc)
      : iarc(&iarc), mine(false) {}

    inline ~iarchive_soft_fail() { if (mine) delete iarc; }
  };


  namespace archive_detail {

    /// called by the regular archive The regular archive will do a hard fail
    template <typename InArcType, typename T>
    struct deserialize_hard_or_soft_fail {
      inline static void exec(InArcType& iarc, T& t) {
        t.load(iarc);
      }
    };

    /// called by the soft fail archive
    template <typename T>
    struct deserialize_hard_or_soft_fail<iarchive_soft_fail, T> {
      inline static void exec(iarchive_soft_fail& iarc, T& t) {
        load_or_fail(*(iarc.iarc), t);
      }
    };


    /**
       Implementation of the deserializer for different types.  This is the
       catch-all. If it gets here, it must be a non-POD and is a class.  We
       therefore call the .save function.  Here we pick between the archive
       types using serialize_hard_or_soft_fail
    */
    template <typename InArcType, typename T, bool IsPOD>
    struct deserialize_impl {
      inline static void exec(InArcType& iarc, T& t) {
        deserialize_hard_or_soft_fail<InArcType, T>::exec(iarc, t);
      }
    };

    // catch if type is a POD
    template <typename InArcType, typename T>
    struct deserialize_impl<InArcType, T, true>{
      inline static void exec(InArcType& iarc, T &t) {
        iarc.read(reinterpret_cast<char*>(&t),
                  sizeof(T));
      }
    };

  } //namespace archive_detail

  /// \cond GRAPHLAB_INTERNAL

  /**
     Allows Use of the "stream" syntax for serialization
  */
  template <typename T>
  inline iarchive& operator>>(iarchive& iarc, T &t) {
    archive_detail::deserialize_impl<iarchive,
                                     T,
                                     gl_is_pod<T>::value >::exec(iarc, t);
    return iarc;
  }



  /**
     Allows Use of the "stream" syntax for serialization
  */
  template <typename T>
  inline iarchive_soft_fail& operator>>(iarchive_soft_fail& iarc, T &t) {
    archive_detail::deserialize_impl<iarchive_soft_fail,
                                     T,
                                     gl_is_pod<T>::value >::exec(iarc, t);
    return iarc;
  }


  /**
     deserializes an arbitrary pointer + length from an archive
  */
  inline iarchive& deserialize(iarchive& iarc,
                               void* str,
                               const size_t length) {
    iarc.read(reinterpret_cast<char*>(str), (std::streamsize)length);
    assert(!iarc.fail());
    return iarc;
  }



  /**
     deserializes an arbitrary pointer + length from an archive
  */
  inline iarchive_soft_fail& deserialize(iarchive_soft_fail& iarc,
                                         void* str,
                                         const size_t length) {
    iarc.read(reinterpret_cast<char*>(str), (std::streamsize)length);
    assert(!iarc.fail());
    return iarc;
  }

  /// \endcond GRAPHLAB_INTERNAL

  /**
     \ingroup group_serialization

     \brief Macro to make it easy to define out-of-place loads

     In the event that it is impractical to implement a save() and load()
     function in the class one wnats to serialize, it is necessary to define
     an "out of save" save and load.

     See \ref sec_serializable_out_of_place for an example

     \note important! this must be defined in the global namespace!
  */
#define BEGIN_OUT_OF_PLACE_LOAD(arc, tname, tval)       \
  namespace graphlab{ namespace archive_detail {        \
  template <typename InArcType>                           \
  struct deserialize_impl<InArcType, tname, false>{       \
  static void exec(InArcType& arc, tname & tval) {

#define END_OUT_OF_PLACE_LOAD() } }; } }




} // namespace graphlab


#endif

#endif








