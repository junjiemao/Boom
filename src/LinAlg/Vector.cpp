/*
  Copyright (C) 2005 Steven L. Scott

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#include <LinAlg/Vector.hpp>
#include <LinAlg/Matrix.hpp>
#include <LinAlg/SpdMatrix.hpp>
#include <LinAlg/VectorView.hpp>
#include <LinAlg/blas.hpp>

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <cmath>
#include <numeric>
#include <functional>

#include <cpputil/math_utils.hpp>
#include <cpputil/string_utils.hpp>
#include <cpputil/report_error.hpp>
#include <distributions.hpp>
#include <sstream>

#include <cstdlib>

using namespace std;

namespace BOOM{
  using namespace blas;
  typedef std::vector<double> dVector;

#ifndef NDEBUG
    inline void check_range(uint n, uint size){
      if(n >= size){
        ostringstream out;
        out << "Vector subscript " << n << " out of bounds in Vector of size "
            << size << endl;
        report_error(out.str());
      }
    }
#else
    inline void check_range(uint, uint){}
#endif

  Vector::Vector() : dVector() {}

  Vector::Vector(uint n, double x) : dVector(n, x) {}

  Vector::Vector(const string &s)
  {
    bool have_comma = s.find(',') < std::string::npos;
    StringSplitter split;
    if(have_comma) split = StringSplitter(",");
    std::vector<string> fields = split(s);
    uint n = fields.size();
    reserve(n);
    for(uint i=0; i<n; ++i){
      double x = atof(fields[i].c_str());
      push_back(x);
    }
  }

  Vector::Vector(const string &s, const string & delim) {
    StringSplitter split(delim);
    std::vector<string> fields = split(s);
    uint n = fields.size();
    reserve(n);
    for(uint i=0; i<n; ++i){
      double x = atof(fields[i].c_str());
      push_back(x);
    }
  }

  Vector::Vector(const std::initializer_list<double> &init)
      : dVector(init)
  {}

  Vector::Vector(std::istream &in)
  {
    double x;
    while(in && (in >> x)) {
      push_back(x);
    }
  }

  Vector::Vector(const dVector &rhs)
      : dVector(rhs)
  {}

  Vector::Vector(const Vector &rhs)
      : dVector(rhs)
  {}

  Vector::Vector(const VectorView &rhs)
      : dVector(rhs.begin(), rhs.end())
  {}

  Vector::Vector(const ConstVectorView &rhs)
      : dVector(rhs.begin(), rhs.end())
  {}

  Vector & Vector::operator=(const Vector &rhs){
    if(&rhs!=this) dVector::operator=(rhs);
    return *this; }

  Vector & Vector::operator=(const VectorView &v){
    assign(v.begin(), v.end()); return *this; }
  Vector & Vector::operator=(const ConstVectorView &v){
    assign(v.begin(), v.end()); return *this; }
  Vector & Vector::operator=(double x){
    uint n = size();
    if(n==0) n=1;
    else dVector::assign(n, x);
    return *this; }

  bool Vector::operator==(const Vector &rhs)const{
    const dVector &tmp1(*this);
    const dVector &tmp2(rhs);
    return tmp1==tmp2;}

  Vector & Vector::swap(Vector &rhs){
    dVector::swap(rhs);
    return *this;
  }

  bool Vector::all_finite() const {
    uint n = size();
    const double *d = data();
    for (uint i = 0; i < n; ++i) {
      if (!std::isfinite(d[i])) {
        return false;
      }
    }
    return true;
  }

  Vector Vector::zero()const{ return Vector(size(), 0.0);}

  Vector Vector::one()const{ return Vector(size(), 1.0);}

  Vector & Vector::randomize(){
    uint n = size();
    double *d(data());
    for(uint i=0; i<n; ++i) d[i] = runif(0,1);
    return *this;
  }

  Vector & Vector::randomize_with_intercept(){
    uint n = size();
    if (n > 0) {
      double *d(data());
      d[0] = 1.0;
      for (uint i = 1; i < n; ++i) {
        d[i] = runif(0,1);
      }
    }
    return *this;
  }

  double * Vector::data(){
    if(empty()) return 0;
    return &((*this)[0]);}

  const double * Vector::data()const{
    if(empty()) return 0;
    return &((*this)[0]);}

  uint Vector::length()const{return size();}

  Vector & Vector::push_back(double x){
    dVector::push_back(x);
    return *this; }

  //------------------------------ deprecated functions ---------------
  const double & Vector::operator()(uint n)const{
    check_range(n, size());
    return (*this)[n];}

  double & Vector::operator()(uint n){
    check_range(n, size());
    return (*this)[n];}

  //------------- input/output -----------------------
  ostream & Vector::write(ostream &out, bool nl)const{
    if(!empty()){
      out << operator[](0);
    }
    for(uint i=1; i<size(); ++i) out<< " " << operator[](i);
    if(nl) out << endl;
    return out; }

  istream & Vector::read(istream &in){
    for(uint i=0; i<size(); ++i) in >> operator[](i);
    return in;
  }

  //-------------------- math
  Vector & Vector::operator+=(double x){
    double *d(data());
    uint n = size();
    for(uint i=0; i<n; ++i) d[i]+=x;
    return *this; }

  Vector & Vector::operator-=(double x){
    return *this += (-x);}

  Vector & Vector::operator*=(double x){
    const int n(size());
    dscal(n, x, data(), stride());
    return *this; }

  Vector & Vector::operator/=(double x){
    assert(x!=0.0 && "divide by zero error in Vector::operator/=");
    return operator*=(1.0/x);}

  Vector & Vector::operator+=(const ConstVectorView &y){
    assert(y.size()==size());
    const int n = size();
    daxpy(n, 1.0, y.data(), y.stride(), data(), stride());
    return *this;
  }

  Vector & Vector::operator+=(const VectorView &y){
    return operator+=(ConstVectorView(y));
  }

  Vector & Vector::operator+=(const Vector &y) {
    return operator+=(ConstVectorView(y));
  }

  Vector & Vector::operator-=(const ConstVectorView &y){
    assert(y.size()==size());
    const int n = size();
    daxpy(n, -1.0, y.data(), y.stride(), data(), stride());
    return *this;
  }

  Vector & Vector::operator-=(const VectorView &y){
    return operator-=(ConstVectorView(y));
  }

  Vector & Vector::operator-=(const Vector &y){
    return operator-=(ConstVectorView(y));
  }

  Vector & Vector::operator*=(const ConstVectorView &y){
    assert(size() == y.size());
    for(uint i=0; i<size(); ++i) (*this)[i] *= y[i];
    return *this;
  }

  Vector & Vector::operator*=(const VectorView &v){
    return operator*=(ConstVectorView(v));
  }

  Vector & Vector::operator*=(const Vector &v){
    return operator*=(ConstVectorView(v));
  }

  Vector & Vector::operator/=(const ConstVectorView &y){
    for(uint i=0; i<size(); ++i) (*this)[i] /= y[i];
    return *this;
  }

  Vector & Vector::operator/=(const VectorView &v) {
    return operator/=(ConstVectorView(v));
  }

  Vector & Vector::operator/=(const Vector &v) {
    return operator/=(ConstVectorView(v));
  }

  Vector & Vector::axpy(const Vector &x, double w){
    assert(x.size()==size());
    const int n =size();
    daxpy(n, w, x.data(), x.stride(), data(), stride());
    return *this;
  }

  Vector & Vector::axpy(const VectorView &x, double w){
    assert(x.size()==size());
    const int n =size();
    daxpy(n, w, x.data(), x.stride(), data(), stride());
    return *this;
  }
  Vector & Vector::axpy(const ConstVectorView &x, double w){
    assert(x.size()==size());
    const int n =size();
    daxpy(n, w, x.data(), x.stride(), data(), stride());
    return *this;
  }

  Vector & Vector::add_Xty(const Matrix &X, const Vector &y, double wgt){
    dgemv(Trans, X.nrow(), X.ncol(), wgt,
          X.data(), X.nrow(), y.data(), y.stride(),
          1.0, data(), stride());
    return *this;
  }


  //------------- linear algebra ----------------

  Vector & Vector::mult(const Matrix &A, Vector &ans)const{
    // v^A == (A^Tv)^T
    assert(ans.size()==A.ncol());
    assert(size()==A.nrow());
    dgemv(Trans, A.nrow(), A.ncol(), 1.0,
          A.data(), A.nrow(), data(), stride(),
          0.0, ans.data(), ans.stride());
    return ans;
  }
  Vector Vector::mult(const Matrix &A)const{
    Vector ans(A.ncol());
    return mult(A,ans);}

  Vector & Vector::mult(const SpdMatrix &A, Vector &ans)const{
    // v^A == (A^Tv)^T
    assert(ans.size()==A.ncol());
    assert(size()==A.nrow());
    dsymv(Upper, A.ncol(), 1.0,
          A.data(), A.nrow(), data(), stride(),
          0.0, ans.data(), ans.stride());
    return ans;
  }
  Vector Vector::mult(const SpdMatrix &S)const{
    Vector ans(S.ncol());
    return mult(S,ans);}


  SpdMatrix Vector::outer()const{
    uint n = size();
    SpdMatrix ans(n, 0.0);
    ans.add_outer(*this);
    return ans;
  }

  Matrix Vector::outer(const Vector &y, double a)const{
    Matrix ans(size(), y.size());
    dger(size(), y.size(),
         a, data(), stride(), y.data(), y.stride(),
         ans.data(), ans.nrow());
    return ans;}

  void Vector::outer(const Vector &y, Matrix &ans, double a)const{
    dger(size(), y.size(),
         a, data(), stride(), y.data(), y.stride(),
         ans.data(), ans.nrow());}

  double Vector::normsq()const{
    double tmp = dnrm2(size(), data(), stride());
    return tmp*tmp;
  }

  Vector & Vector::normalize_prob(){
    const int n(size());
    double s = dasum(n, data(), stride());
    if(s==0) {
      report_error("normalizing constant is zero in Vector::normalize_prob");
    }
    operator/=(s);
    return *this;
  }

  Vector & Vector::normalize_logprob(){
    double nc=0;
    Vector &x= *this;
    double m = max();
    uint n = size();
    for(uint i=0; i<n; ++i){
      x[i] = std::exp(x[i]-m);
      nc+=x[i]; }
    x/=nc;
    return *this;   // might want to change this
  }

  Vector & Vector::normalize_L2(){
    double nc = dnrm2(size(), data(), stride());
    (*this)/=nc;
    return *this;
  }

  double Vector::min()const{ return *min_element(begin(), end());}
  double Vector::max() const { return *std::max_element(begin(), end()); }

  uint Vector::imax()const{
    const_iterator it = std::max_element(begin(), end());
    return it-begin();}

  uint Vector::imin()const{
    const_iterator it = min_element(begin(), end());
    return it-begin();}

  double Vector::abs_norm()const{
    return dasum(size(), data(), stride());}

  double Vector::max_abs()const{
    dVector::size_type n = size();
    const double *d = data();
    double m = -1;
    for (dVector::size_type i = 0; i < n; ++i) {
      m = std::max<double>(m, fabs(d[i]));
    }
    return m;
  }

  double Vector::sum()const{
    // Benchmarks indicate that a straight sum based on pointer
    // arithmatic is MUCH faster than iterator based approaches, so
    // don't replace this with std::accumulate().
    const double *d = data();
    auto n = this->size();
    double ans = 0;
    for (auto i = n * 0; i < n; ++i) {
      ans += d[i];
    }
    return ans;
  }

  inline double mul(double x, double y){return x*y;}
  double Vector::prod()const{
    const auto n = this->size();
    double ans = 1.0;
    const double *d = data();
    for (auto i = n * 0; i < n; ++i) {
      ans *= d[i];
    }
    return ans;
  }

  Vector & Vector::sort(){
    std::sort(begin(), end());
    return *this;
  }

  Vector & Vector::transform(std::function<double(double)> f) {
    const auto n = this->size();
    double *d = data();
    for (auto i = n * 0; i < n; ++i) {
      d[i] = f(d[i]);
    }
    return *this;
  }

  namespace {
    template <class V>
    double dot_impl(const Vector &x, const V & y){
      const int n(x.size());
      if(y.size() != static_cast<uint>(n)){
        ostringstream err;
        err << "Dot product between two vectors of different sizes:"
            << endl
            << "x = " << x << endl
            << "y = " << y << endl;
        report_error(err.str());
      }
      if(y.stride() > 0)
        return ddot(n, x.data(), x.stride(), y.data(), y.stride());
      double ans = 0;
      for(int i = 0; i < n; ++i){
        ans += x[i] * y[i];
      }
      return ans;
    }

    template<class V>
    double affdot_impl(const Vector &x, const V & y){
      uint n = x.size();
      uint m = y.size();
      if(m==n) return x.dot(y);
      double ans=0.0;
      const double *v1=0, *v2=0;
      if(m==n+1){    // y is one unit longer than x
        ans= y.front();
        v1 = y.data()+1;
        v2 = x.data();
      }else if (n==m+1){   // x is one unit longer than y
        ans = x.front();
        v1 = y.data();
        v2 = x.data()+1;
      }else{
        report_error("x and y do not conform in affdot");
      }
      const int i(std::min(m,n));
      return ans + ddot(i, v1, y.stride(), v2, x.stride());
    }
  }
  double Vector::dot(const Vector &y)const{ return dot_impl(*this, y);}
  double Vector::dot(const VectorView &y)const{ return dot_impl(*this,y);}
  double Vector::dot(const ConstVectorView &y)const{ return dot_impl(*this,y);}

  double Vector::affdot(const Vector &y)const{
    return affdot_impl(*this, y);}
  double Vector::affdot(const VectorView &y)const{
    return affdot_impl(*this, y); }
  double Vector::affdot(const ConstVectorView &y)const{
    return affdot_impl(*this, y); }
  //============== non member functions from Vector.hpp =============

  Vector scan_vector(const string &fname){
    ifstream in(fname.c_str());
    Vector ans;
    double x;
    while(in>>x) ans.push_back(x);
    return ans;
  }

  void permute_Vector(Vector &v, const std::vector<uint> &perm){
    uint n = v.size();
    Vector x(n);
    for(uint i = 0; i<n; ++i) x[i] = v[perm[i]];
    v=x;
  }

  typedef std::vector<string> svec;
  Vector str2vec(const string &line){
    StringSplitter split;
    svec sv=split(line);
    return str2vec(sv); }

  Vector str2vec(const svec &sv){
    uint n= sv.size();
    Vector ans(n);
    for(uint i = 0; i<n; ++i){
      istringstream tmp(sv[i]);
      tmp >> ans[i]; }
    return ans; }

   Vector operator/(double a, const ConstVectorView &x){
     Vector ans(x.size(), a);
     ans/=x;
     return ans;
   }
   Vector operator/(double a, const Vector &x){
     ConstVectorView xx(x);
     return a / xx;
   }
   Vector operator/(double a, const VectorView &x){
     ConstVectorView xx(x);
     return a / xx;
   }
  Vector operator/(const ConstVectorView &x, double a){
    Vector ans(x);
    ans /= a;
    return ans;
  }
  Vector operator/(const VectorView &x, double a){
    return ConstVectorView(x) / a;
  }

  Vector operator-(double a, const ConstVectorView &x){
    Vector ans(-x);
    ans+=a;
    return ans;
  }
  Vector operator-(double a, const Vector &x){
    ConstVectorView xx(x);
    return a - xx;
  }
  Vector operator-(double a, const VectorView &x){
    ConstVectorView xx(x);
    return a - xx;
  }
  Vector operator-(const ConstVectorView &x, double a){
    Vector ans(x);
    ans -= a;
    return ans;
  }

  namespace {
    template <class V1, class V2>
    Vector vector_add(const V1 &v1, const V2 &v2) {
      Vector v(v1);
      return v += v2;
    }

    template <class V1, class V2>
    Vector vector_subtract(const V1 &v1, const V2 &v2) {
      Vector v(v1);
      return v -= v2;
    }

    template <class V1, class V2>
    Vector vector_multiply(const V1 &v1, const V2 &v2) {
      Vector v(v1);
      return v *= v2;
    }

    template <class V1, class V2>
    Vector vector_divide(const V1 &v1, const V2 &v2) {
      Vector v(v1);
      return v /= v2;
    }
  }

  Vector operator+(const ConstVectorView &x, const Vector &y) {
    return vector_add(y, x);
  }
  Vector operator+(const Vector &x, const ConstVectorView &y) {
    return vector_add(x, y);
  }
  Vector operator+(const VectorView &x, const Vector &y) {
    return vector_add(y, x);
  }
  Vector operator+(const Vector &x, const VectorView &y) {
    return vector_add(x, y);
  }

  Vector operator-(const ConstVectorView &x, const Vector &y) {
    return vector_subtract(x, y);
  }
  Vector operator-(const Vector &x, const ConstVectorView &y) {
    return vector_subtract(x, y);
  }
  Vector operator-(const VectorView &x, const Vector &y) {
    return vector_subtract(x, y);
  }
  Vector operator-(const Vector &x, const VectorView &y) {
    return vector_subtract(x, y);
  }

  Vector operator*(const ConstVectorView &x, const Vector &y) {
    return vector_multiply(y, x);
  }
  Vector operator*(const Vector &x, const ConstVectorView &y) {
    return vector_multiply(x, y);
  }
  Vector operator*(const VectorView &x, const Vector &y) {
    return vector_multiply(y, x);
  }
  Vector operator*(const Vector &x, const VectorView &y) {
    return vector_multiply(x, y);
  }

  Vector operator/(const ConstVectorView &x, const Vector &y) {
    return vector_divide(x, y);
  }
  Vector operator/(const Vector &x, const ConstVectorView &y) {
    return vector_divide(x, y);
  }
  Vector operator/(const VectorView &x, const Vector &y) {
    return vector_divide(x, y);
  }
  Vector operator/(const Vector &x, const VectorView &y) {
    return vector_divide(x, y);
  }

  // unary transformations
  Vector operator-(const Vector &x){
    Vector ans = x;
    return ans.operator*=(-1);
  }

  namespace {
    typedef double (*DoubleFunction)(double);
    Vector vector_transform(const ConstVectorView &x,
                            std::function<double(double)> f) {
      Vector ans(x.size());
      std::transform(x.begin(), x.end(), ans.begin(), f);
      return ans;
    }

  }  // namespace

  //======================================================================
  // The static_cast in the following transformation functions is
  // there to help with overload resolution.  See b/28857895.
  Vector log(const Vector &x){
    return vector_transform(x, static_cast<DoubleFunction>(::log));
  }
  Vector log(const VectorView &x){
    return vector_transform(x, static_cast<DoubleFunction>(::log));
  }
  Vector log(const ConstVectorView &x){
    return vector_transform(x, static_cast<DoubleFunction>(::log));
  }

  Vector exp(const Vector &x){
    return vector_transform(x, static_cast<DoubleFunction>(::exp));
  }
  Vector exp(const VectorView &x){
    return vector_transform(x, static_cast<DoubleFunction>(::exp));
  }
  Vector exp(const ConstVectorView &x){
    return vector_transform(x, static_cast<DoubleFunction>(::exp));
  }

  Vector sqrt(const Vector &x){
    return vector_transform(x, static_cast<DoubleFunction>(::sqrt));
  }
  Vector sqrt(const VectorView &x){
    return vector_transform(x, static_cast<DoubleFunction>(::sqrt));
  }
  Vector sqrt(const ConstVectorView &x){
    return vector_transform(x, static_cast<DoubleFunction>(::sqrt));
  }

  Vector pow(const Vector &x, double power){
    return vector_transform(x, [power](double x)->double{
        return std::pow(x, power);});
  }
  Vector pow(const VectorView &x, double power){
    return vector_transform(x, [power](double x)->double{
        return std::pow(x, power);});
  }
  Vector pow(const ConstVectorView &x, double power){
    return vector_transform(x, [power](double x)->double{
        return std::pow(x, power);});
  }

  Vector pow(const Vector &x, int power){
    return vector_transform(x, [power](double x)->double{
        return std::pow(x, power);});
  }
  Vector pow(const VectorView &x, int power){
    return vector_transform(x, [power](double x)->double{
        return std::pow(x, power);});
  }
  Vector pow(const ConstVectorView &x, int power){
    return vector_transform(x, [power](double x)->double{
        return std::pow(x, power);});
  }

  Vector abs(const Vector &x) {
    return vector_transform(x, static_cast<DoubleFunction>(::fabs));
  }
  Vector abs(const VectorView &x) {
    return vector_transform(x, static_cast<DoubleFunction>(::fabs));
  }
  Vector abs(const ConstVectorView &x) {
    return vector_transform(x, static_cast<DoubleFunction>(::fabs));
  }

  namespace {
    template <class VECTOR>
    std::pair<double, double> range_impl(const VECTOR &v){
      double lo = infinity();
      double hi = negative_infinity();
      for (int i = 0; i < v.size(); ++i) {
        double x = v[i];
        if (x < lo) {
          lo = x;
        }
        if (x > hi) {
          hi = x;
        }
      }
      return std::make_pair(lo, hi);
    }
  }

  std::pair<double, double> range(const Vector &v) {
    return range_impl(v);
  }

  std::pair<double, double> range(const VectorView &v) {
    return range_impl(v);
  }

  std::pair<double, double> range(const ConstVectorView &v) {
    return range_impl(v);
  }

  Vector cumsum(const Vector &x){
    Vector ans(x);
    std::partial_sum(x.begin(), x.end(), ans.begin());
    return ans;
  }

  ostream & operator<<(ostream & out, const Vector &v){
    return v.write(out, false);}

  void print(const Vector &v) {
    std::cout << v << std::endl;
  }

  void print_vector(const Vector &v) {
    print(v);
  }

  istream & operator>>(istream &in, Vector & v){
    string s;
    do{
      getline(in,s);
    }while(is_all_white(s));
    v = str2vec(s);
    return in; }

  Vector read_Vector(istream &in){
    string line;
    getline(in, line);
    return str2vec(line);}

  namespace {
    template <class VEC1, class VEC2>
    Vector concat_impl(const VEC1 &x, const VEC2 &y){
      Vector ans(x);
      ans.concat(y);
      return ans;
    }

    template<class VEC>
    Vector scalar_concat_impl(double x, const VEC &v){
      Vector ans(1, x);
      return ans.concat(v);
    }
  }

  Vector concat(const Vector &x, const Vector &y){
    return concat_impl(x, y);}
  Vector concat(const Vector &x, const VectorView &y){
    return concat_impl(x, y);}
  Vector concat(const Vector &x, const ConstVectorView &y){
    return concat_impl(x, y);}
  Vector concat(const Vector &x, double y){
    Vector ans(x); ans.push_back(y); return ans;}

  Vector concat(const VectorView &x, const Vector &y){
    return concat_impl(x, y);}
  Vector concat(const VectorView &x, const VectorView &y){
    return concat_impl(x, y);}
  Vector concat(const VectorView &x, const ConstVectorView &y){
    return concat_impl(x, y);}
  Vector concat(const VectorView &x, double y){
    Vector ans(x); ans.push_back(y); return ans;}

  Vector concat(const ConstVectorView &x, const Vector &y){
    return concat_impl(x, y);}
  Vector concat(const ConstVectorView &x, const VectorView &y){
    return concat_impl(x, y);}
  Vector concat(const ConstVectorView &x, const ConstVectorView &y){
    return concat_impl(x, y);}
  Vector concat(const ConstVectorView &x, double y){
    Vector ans(x); ans.push_back(y); return ans;}

  Vector concat(double x, const Vector &y){
    return scalar_concat_impl(x, y);}
  Vector concat(double x, const VectorView &y){
    return scalar_concat_impl(x, y);}
  Vector concat(double x, const ConstVectorView &y){
    return scalar_concat_impl(x, y);}

  Vector select(const Vector &v, const std::vector<bool> & inc,
                uint nvars){
    Vector ans(nvars);
    assert( (v.size()==inc.size()) && (inc.size()>=nvars) );
    uint I=0;
    for(uint i=0; i<nvars; ++i)
      if(inc[i])
        ans[I++] = v[i];
    return ans;
  }

  Vector select(const Vector &v, const std::vector<bool> &inc){
    uint nvars(std::accumulate(inc.begin(), inc.end(), 0));
    return select(v,inc,nvars);
  }

  Vector sort(const Vector &v){
    Vector ans(v);
    return ans.sort();
  }

  Vector sort(const VectorView &v){
    Vector ans(v);
    return ans.sort();
  }

  Vector sort(const ConstVectorView &v){
    Vector ans(v);
    return ans.sort();
  }

  Vector rev(const ConstVectorView &v) {
    // TODO(stevescott):
    // Do this with Vector(v.rbegin(), v.rend().
    // There is a const-related problem with this approach
    // currently.  I think the VectorViewConstIterator class is
    // doing it wrong.
    int n = v.size();
    Vector ans(v.size());
    if (n == 0) {
      return ans;
    }
    for (int i = 0; i < n; ++i) {
      ans[i] = v[n - 1 - i];
    }
    return ans;
  }

  Vector rev(const VectorView &v) {
    return rev(ConstVectorView(v));
  }

  Vector rev(const Vector &v) {
    return rev(ConstVectorView(v));
  }

  namespace {
    template <class VECTOR>
    std::vector<int> round_impl(const VECTOR &v) {
      std::vector<int> ans;
      ans.reserve(v.size());
      for (int i = 0; i < v.size(); ++i) {
        ans.push_back(lround(v[i]));
      }
      return ans;
    }
  }  // namespace

  std::vector<int> round(const Vector &v) { return round_impl(v); }
  std::vector<int> round(const VectorView &v) { return round_impl(v); }
  std::vector<int> round(const ConstVectorView &v) { return round_impl(v); }

} // BOOM
