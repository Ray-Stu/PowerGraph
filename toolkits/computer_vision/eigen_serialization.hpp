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
 *
 */

#ifndef EIGEN_SERIALIZATION_HPP
#define EIGEN_SERIALIZATION_HPP



#include <Eigen/Dense>

#include <graphlab.hpp>



BEGIN_OUT_OF_PLACE_SAVE(arc, Eigen::VectorXd, vec) {
  typedef Eigen::VectorXd::Index index_type;
  typedef Eigen::VectorXd::Scalar scalar_type;
  const index_type size = vec.size();
  arc << size;
  graphlab::serialize(arc, vec.data(), size * sizeof(scalar_type));
} END_OUT_OF_PLACE_SAVE()


BEGIN_OUT_OF_PLACE_LOAD(arc, Eigen::VectorXd, vec) {
  typedef Eigen::VectorXd::Index index_type;
  typedef Eigen::VectorXd::Scalar scalar_type;
  index_type size = 0;
  arc >> size;
  vec.resize(size);
  graphlab::deserialize(arc, vec.data(), size * sizeof(scalar_type));
} END_OUT_OF_PLACE_LOAD()


BEGIN_OUT_OF_PLACE_SAVE(arc, Eigen::MatrixXd, mat) {
  typedef Eigen::MatrixXd::Index index_type;
  typedef Eigen::MatrixXd::Scalar scalar_type;
  const index_type rows = mat.rows();
  const index_type cols = mat.cols();
  arc << rows << cols;
  graphlab::serialize(arc, mat.data(), rows*cols*sizeof(scalar_type));
} END_OUT_OF_PLACE_SAVE()


BEGIN_OUT_OF_PLACE_LOAD(arc, Eigen::MatrixXd, mat) {
  typedef Eigen::MatrixXd::Index index_type; 
  typedef Eigen::MatrixXd::Scalar scalar_type;
  index_type rows=0, cols=0;
  arc >> rows >> cols;
  mat.resize(rows,cols);
  graphlab::deserialize(arc, mat.data(), rows*cols*sizeof(scalar_type));
} END_OUT_OF_PLACE_LOAD()





// inline graphlab::oarchive& operator<<(graphlab::oarchive& arc, const Eigen::VectorXd& vec) {
//   typedef Eigen::VectorXd::Index index_type;
//   typedef Eigen::VectorXd::Scalar scalar_type;
//   const index_type size = vec.size();
//   arc << size;
//   graphlab::serialize(arc, vec.data(), size * sizeof(scalar_type));
//   return arc;
// } // end of save vector

// inline graphlab::iarchive& operator>>(graphlab::iarchive& arc, Eigen::VectorXd& vec) {
//   typedef Eigen::VectorXd::Index index_type;
//   typedef Eigen::VectorXd::Scalar scalar_type;
//   index_type size = 0;
//   arc >> size;
//   vec.resize(size);
//   graphlab::deserialize(arc, vec.data(), size * sizeof(scalar_type));
//   return arc;
// } // end of save vector


// inline graphlab::oarchive& operator<<(graphlab::oarchive& arc, const Eigen::MatrixXd& mat) {
//   typedef Eigen::MatrixXd::Index index_type;
//   typedef Eigen::MatrixXd::Scalar scalar_type;
//   const index_type rows = mat.rows();
//   const index_type cols = mat.cols();
//   arc << rows << cols;
//   graphlab::serialize(arc, mat.data(), rows*cols*sizeof(scalar_type));
//   return arc;
// } // end of save matrix

// inline graphlab::iarchive& operator>>(graphlab::iarchive& arc,  Eigen::MatrixXd& mat) {
//   typedef Eigen::MatrixXd::Index index_type; 
//   typedef Eigen::MatrixXd::Scalar scalar_type;
//   index_type rows=0, cols=0;
//   arc >> rows >> cols;
//   mat.resize(rows,cols);
//   graphlab::deserialize(arc, mat.data(), rows*cols*sizeof(scalar_type));
//   return arc;
// } // end of load matrix


#endif
