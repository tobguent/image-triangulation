/*
 * MatlabInterface.hpp
 *
 *  Created on: Sep 10, 2012
 *      Author: Janick Martinez Esturo
 *  Edited on: May 08, 2014
 *      Author: Tobias Günther
 */

#ifndef _MATLABINTERFACE_HEADER_
#define _MATLABINTERFACE_HEADER_

#include <stdlib.h>		// include before the matlab engine!


#pragma warning(push)
#pragma warning(disable: 4996)

namespace mat {
	/* WAR CUDA <-> MATLAB */
	//Not needed anymore since Matlab r2011b, yeah! :)
	//  #define CHAR16_T
	//  #if defined(__CUDACC__)
	//  typedef wchar_t char16_t;
	//  #endif
#include <engine.h>
}

#include <Eigen/Sparse>


#include <string>
#include <sstream>
#include <array>
#include <vector>

namespace util {

template<typename T> struct MatlabType               { enum { TYPE=-1 }; };
template<>           struct MatlabType<double>       { enum { TYPE=mat::mxDOUBLE_CLASS }; };
template<>           struct MatlabType<float>        { enum { TYPE=mat::mxSINGLE_CLASS }; };
template<>           struct MatlabType<int>          { enum { TYPE=mat::mxINT32_CLASS }; };
template<>           struct MatlabType<unsigned int> { enum { TYPE=mat::mxUINT32_CLASS }; };

class MatlabInterface {
public:
  static void startMatlabEngine();
  static void closeMatlabEngine();

  static std::string meval(const std::string& cmd);


  static void putString(const std::string& name_, const std::string& str_) {
    if(!useMatlabEngine())
      return;

    mat::mxArray* var = mat::mxCreateString(str_.c_str());
    mat::engPutVariable((mat::Engine*)ep, name_.c_str(), var);
    mat::mxDestroyArray(var);
  }

  template <typename T>
  static void putScalar(const std::string& name_, const T& value_, const bool make_double_ = true) {
    if(!useMatlabEngine())
      return;

    mat::mxArray *var;

    if(make_double_)
      var = mat::mxCreateDoubleScalar(value_);
    else {
      const mat::mwSize dims[] = {1};
      var = mxCreateNumericArray(1, dims, getMxClassID<T>(), mat::mxREAL);
      *(T*)mat::mxGetPr(var) = value_;
    }

    mat::engPutVariable((mat::Engine*)ep, name_.c_str(), var);
    mat::mxDestroyArray(var);
  }

  template <typename C>
  static void putVar(const std::string& name_, const C& values_, const bool make_double_ = true) {
    assert(values_.size());

    if(!useMatlabEngine())
      return;

    const mat::mwSize dims[] = {values_.size()};
    mat::mxArray* var = mat::mxCreateNumericArray(1, dims, getMxClassID<typename C::value_type>(), mat::mxREAL);
    std::copy(values_.begin(), values_.end(), (typename C::value_type*)mat::mxGetPr(var));
    mat::engPutVariable((mat::Engine*)ep, name_.c_str(), var);
    if(make_double_)
      mat::engEvalString((mat::Engine*)ep,(name_+"=double("+name_+");").c_str());
    mat::mxDestroyArray(var);
  }

  template <typename CC>
  static void put2DVar(const std::string& name_, const CC& values_, const bool make_double_ = true) {
    typedef typename CC::value_type Container;
    typedef typename Container::value_type value_type;

    assert(values_.size() > 0);

    if(!useMatlabEngine())
      return;

    size_t m = values_.front().size(), n = values_.size();
    const mat::mwSize dims[] = {m, n};
    mat::mxArray* var = mat::mxCreateNumericArray(2, dims, getMxClassID<value_type>(), mat::mxREAL);

    for(size_t i=0;i<values_.size();++i) {
      const Container &vv = values_[i];
      assert(vv.size() == m);
      std::copy(vv.begin(), vv.end(), (value_type*)mxGetPr(var) + i*m);
    }

    mat::engPutVariable((mat::Engine*)ep, name_.c_str(), var);
    if(make_double_)
      mat::engEvalString((mat::Engine*)ep,(name_+"=double("+name_+");").c_str());
    mat::mxDestroyArray(var);
  }

  template <typename MATLABTYPE, typename C>
  static void getVar(const std::string& name_, C& v_out_) {
    if(!useMatlabEngine())
      return;

    mat::mxArray* v = mat::engGetVariable((mat::Engine*)ep, name_.c_str());

    const mat::mwSize numelements = mat::mxGetNumberOfElements(v);

    v_out_.clear();
    v_out_.resize(numelements);

    MATLABTYPE *p = mat::mxGetPr(v);

    std::copy(p, p+numelements, v_out_.begin());

    mat::mxDestroyArray(v);
  }

  template <typename MATLABTYPE, typename T>
  static void getScalar(const std::string& name_, T& v_out_) {
    if(!useMatlabEngine())
      return;

    mat::mxArray* v = mat::engGetVariable((mat::Engine*)ep, name_.c_str());

    assert(mat::mxGetM(v) == 1 && mat::mxGetN(v) == 1);

    MATLABTYPE *p = mat::mxGetPr(v);

    v_out_ = *p;

    mat::mxDestroyArray(v);
  }

  template <typename SM>
  static void putEigenSparseMatrix(const std::string& name_, const SM& sm_) {
    if(!useMatlabEngine())
      return;

    typedef typename SM::Scalar value_type;
    typedef typename SM::Index  index_type;

    static_assert(!SM::IsRowMajor, "Eigen::SparseMatrix has to be column major!");    //CCS

    if(!sm_.isCompressed())
      const_cast<SM&>(sm_).makeCompressed();

    const auto nnz = sm_.nonZeros();

    mat::mxArray* sparse = mat::mxCreateSparse(sm_.rows(), sm_.cols(), nnz,
                                               mat::mxREAL);

    std::copy(sm_.valuePtr(), sm_.valuePtr() + nnz, mat::mxGetPr(sparse));
    std::copy(sm_.innerIndexPtr(), sm_.innerIndexPtr() + nnz,
              mat::mxGetIr(sparse));
    std::copy(sm_.outerIndexPtr(), sm_.outerIndexPtr() + sm_.outerSize() + 1,
              mat::mxGetJc(sparse));

    mat::engPutVariable((mat::Engine*)ep, name_.c_str(), sparse);

    mat::mxDestroyArray(sparse);
  }

  template <typename M>
  static void putEigenMatrix(const std::string& name_, const M& m_, bool make_double_ = true) {
    if(!useMatlabEngine())
      return;

    std::array<mat::mwSize,2> dims;
    dims[0] = static_cast<mat::mwSize>(m_.rows());
    dims[1] = static_cast<mat::mwSize>(m_.cols());

    mat::mxArray* mat = mat::mxCreateNumericArray(2, dims.data(),
                                                  getMxClassID<typename M::Scalar>(),
                                                  mat::mxREAL);

    std::copy(m_.data(), m_.data() + dims[0]*dims[1],
              (typename M::Scalar*)mat::mxGetPr(mat));

    mat::engPutVariable((mat::Engine*)ep, name_.c_str(), mat);
    mat::mxDestroyArray(mat);

    if(make_double_)
      mat::engEvalString((mat::Engine*)ep,(name_+"=double("+name_+");").c_str());
  }

  template <typename MATLABTYPE, typename VT>
  static Eigen::Matrix<VT,Eigen::Dynamic,Eigen::Dynamic>
  getEigenMatrix(const std::string& name_) {
    typedef Eigen::Matrix<VT,Eigen::Dynamic,Eigen::Dynamic> RET_T;

    if(!useMatlabEngine())
      return RET_T(0,0);

    mat::mxArray* v = mat::engGetVariable((mat::Engine*)ep, name_.c_str());

    assert(mxGetNumberOfDimensions(v) == 2);

    const mat::mwSize *dims = mxGetDimensions(v);
    const mat::mwSize numelements = mat::mxGetNumberOfElements(v);

    MATLABTYPE *p = mat::mxGetPr(v);

    RET_T ret;
    ret.resize(dims[0], dims[1]);

    std::copy(p, p + numelements, ret.data());

    mat::mxDestroyArray(v);

    return ret;
  }

protected:
  static bool useMatlabEngine();

private:
  static mat::Engine *ep;
  static char output[];
  static std::vector<std::string> paths;

  template<typename T>
  inline static mat::mxClassID getMxClassID() {
    return (mat::mxClassID)MatlabType<T>::TYPE;
  }
};

} //namespace util

#pragma warning(pop)

#endif //_MATLABINTERFACE_HEADER_
