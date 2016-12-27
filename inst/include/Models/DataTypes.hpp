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

#ifndef DATA_TYPES_H
#define DATA_TYPES_H

#include <BOOM.hpp>
#include <map>     // for STL's map container
#include <vector>
#include <string>
#include <cmath>

#include <LinAlg/Vector.hpp>
#include <LinAlg/Matrix.hpp>
#include <LinAlg/SpdMatrix.hpp>
#include <LinAlg/CorrelationMatrix.hpp>  // for VectorData

#include <cpputil/Ptr.hpp>
#include <cpputil/RefCounted.hpp>
#include <boost/function.hpp>


namespace BOOM{

  class Data{  // abstract base class
  public:
    RefCounted rc_;
    void up_count(){rc_.up_count();}
    void down_count(){rc_.down_count();}
    unsigned int ref_count(){return rc_.ref_count();}

    enum missing_status{
      observed=0,
      completely_missing,
      partly_missing};
  private:
    missing_status missing_flag;
    mutable std::vector<boost::function<void(void)> > signals_;
  public:
    Data() : missing_flag(observed){}
    Data(const Data &rhs)
      : missing_flag(rhs.missing_flag)
    {}
    virtual Data * clone()const=0;
    virtual ~Data(){}
    virtual ostream & display(ostream &)const =0;
    missing_status missing()const;
    void set_missing_status(missing_status m);
    void signal()const{
      uint n = signals_.size();
      for(uint i=0; i<n; ++i) signals_[i]();
    }
    void add_observer(boost::function<void(void)> f){
      signals_.push_back(f); }
    friend void intrusive_ptr_add_ref(Data *d);
    friend void intrusive_ptr_release(Data *d);
  };
  //----------------------------------------------------------------------
  void intrusive_ptr_add_ref(Data *d);
  void intrusive_ptr_release(Data *d);
  //----------------------------------------------------------------------
  typedef Ptr<Data> dPtr;
  //----------------------------------------------------------------------
  ostream & operator<<(ostream & out, const Data &d);
  ostream & operator<<(ostream &out , const dPtr dp);
  void print_data(const Data &d);

  //----------------------------------------------------------------------

  template <class DAT>
  class DataTraits : virtual public Data{
  public:
    DataTraits(){}
    DataTraits(const DataTraits &rhs) : Data(rhs) {}
    typedef DAT value_type;
    typedef DataTraits<DAT> Traits;
    virtual void set(const value_type &, bool)=0;
    virtual const value_type &value()const=0;
  };
  //----------------------------------------------------------------------
  template <class T>
  class UnivData : public DataTraits<T>{  // univariate data
  public:
    typedef typename DataTraits<T>::Traits Traits;
    // constructors
    UnivData() : value_() {}
    UnivData(T y) : value_(y){}
    UnivData(const UnivData &rhs): Data(rhs), Traits(rhs), value_(rhs.value_) {}
    UnivData<T> * clone()const{return new UnivData<T>(*this);}

    const T& value()const{return value_;}
    virtual void set(const T &rhs, bool Signal=true){
      value_ = rhs;
      if(Signal) this->signal();
    }
    ostream & display(ostream &out) const{out << value_ ; return out;}
  private:
    T value_;
  };

  //----------------------------------------------------------------------//
  typedef UnivData<unsigned int> IntData;
  typedef UnivData<double> DoubleData;
  typedef UnivData<bool> BinaryData;
  //----------------------------------------------------------------------//

  class VectorData : public DataTraits<Vector>{
  public:
    explicit VectorData(uint n, double x=0);
    VectorData(const Vector &y);
    VectorData(const VectorData &d);
    VectorData * clone()const override;

    uint dim()const {return x.size();}
    ostream & display(ostream &out) const override;

    const Vector & value()const override{return x;}
    void set(const Vector &rhs, bool signal=true) override;
    virtual void set_element(double value, int position, bool signal = true);

    double operator[](uint)const;
    double & operator[](uint);
  private:
    Vector x;
  };
  //----------------------------------------------------------------------//
  class MatrixData : public DataTraits<Mat>{
  public:
    MatrixData(int r, int c, double x=0.0);
    MatrixData(const Matrix &y);
    MatrixData(const MatrixData &rhs);
    MatrixData * clone()const override;

    uint nrow()const{return x.nrow();}
    uint ncol()const{return x.ncol();}

    ostream & display(ostream &out) const override;

    const Matrix & value()const override{return x;}
    void set(const Matrix &rhs, bool signal=true) override;
    virtual void set_element(double value, int row, int col, bool signal=true);
  private:
    Matrix x;
  };
}  // namespace BOOM

#endif // DATA_TYPES_H
