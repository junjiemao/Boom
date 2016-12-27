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
#ifndef BOOM_DATA_POLICIES_HPP
#define BOOM_DATA_POLICIES_HPP

#include <Models/ModelTypes.hpp>
#include <Models/Policies/DataInfoPolicy.hpp>
#include <functional>

namespace BOOM {
  template <class D>
  class IID_DataPolicy : public DefaultDataInfoPolicy<D> {
   public:
    typedef D DataType;
    typedef IID_DataPolicy<D> DataPolicy;
    typedef std::vector<Ptr<DataType> > DatasetType;
    typedef DefaultDataInfoPolicy<D> Info;

    IID_DataPolicy();
    IID_DataPolicy(const DatasetType &d);
    template <class FwdIt>
    IID_DataPolicy(FwdIt Begin, FwdIt End);

    IID_DataPolicy(const IID_DataPolicy &);
    IID_DataPolicy & operator=(const IID_DataPolicy &);

    // Each observer will be called whenever data is added or cleared.
    void add_observer(std::function<void(void)> observer) {
      observers_.push_back(observer);
    }

    virtual void clear_data();
    virtual void set_data(const DatasetType &d);
    virtual void add_data(Ptr<Data> dp);
    virtual void add_data(Ptr<DataType> dp);

    DatasetType & dat() {return dat_;}
    const DatasetType & dat()const{return dat_;}

    template <class FwdIt>
    void set_data(FwdIt Beg, FwdIt End);

    // for automatic conversions from raw data types, e.g. double data
    template <class FwdIt>
    void set_data_raw(FwdIt Beg, FwdIt End);

    template <class FwdIt>
    void add_data_seq(FwdIt Beg, FwdIt End);

    template <class Cont>
    void add_data_seq(const Cont &c) {
      this->add_data_seq(c.begin(), c.end());
    }

    virtual void combine_data(const Model &mod, bool just_suf=true);

    void signal() {
      for (int i = 0; i < observers_.size(); ++i) {
        observers_[i]();
      }
    }
   private:
    DatasetType dat_;
    std::vector<std::function<void(void)> > observers_;
  };
  //======================================================================
  // take care to call virtual function add_data instead of adding
  // things directly to dat_.  doing so allows models to overload
  // add_data, instead of having to modify the whole thing.

  // Except, you shouldn't call a virtual function from a constructor,
  // because the object is not yet fully formed.  Thus, when an
  // IID_DataPolicy is copied, the default action is to simply copy
  // over the data.  If a class overloads add_data, then the data
  // should be cleared and recopied from that class's copy
  // constructor.

  template <class D>
  IID_DataPolicy<D>::IID_DataPolicy() {}

  template <class D>
  IID_DataPolicy<D>::IID_DataPolicy(const DatasetType &d)
    : dat_(d)
  {}

  template <class D>
  template <class FwdIt>
  IID_DataPolicy<D>::IID_DataPolicy(FwdIt Beg, FwdIt End)
    : dat_(Beg,End)
  {}

  template <class D>
  IID_DataPolicy<D>::IID_DataPolicy(const IID_DataPolicy &rhs)
    : Model(rhs),
      DefaultDataInfoPolicy<D>(rhs),
      dat_(rhs.dat_)
  {}

  template <class D>
  IID_DataPolicy<D> & IID_DataPolicy<D>::operator=(const IID_DataPolicy &rhs) {
    if (&rhs != this) set_data(rhs.dat_);
    return *this;
  }

  template<class D>
  void IID_DataPolicy<D>::clear_data() {
    dat_.clear();
    signal();
  }

  template<class D>
  void IID_DataPolicy<D>::set_data(const DatasetType & d) {
    clear_data();
    size_t n = d.size();
    for (size_t i = 0; i < n; ++i) add_data(d[i]);
  }

  template<class D>
  template<class FwdIt>
  void IID_DataPolicy<D>::set_data(FwdIt Beg, FwdIt End) {
    clear_data();
    while (Beg!=End) {
      add_data(*Beg);
      ++Beg;
    }
  }

  template<class D>
  template<class FwdIt>
  void IID_DataPolicy<D>::add_data_seq(FwdIt Beg, FwdIt End) {
    while (Beg!=End) {
      add_data(*Beg);
      ++Beg;
    }
  }

  template<class D>
  void IID_DataPolicy<D>::combine_data(const Model &  other, bool) {
    const DataPolicy & d(dynamic_cast<const DataPolicy &>(other));
    //    const IID_DataPolicy<D> & d(dynamic_castother.dcast<IID_DataPolicy<D> >());
    add_data_seq(d.dat_.begin(), d.dat_.end());
  }

  template<class D>
  template<class FwdIt>
  void IID_DataPolicy<D>::set_data_raw(FwdIt Beg, FwdIt End) {
    clear_data();
    for (FwdIt it = Beg; it != End; ++it) {
      NEW(DataType, dp)(*it);
      add_data(dp);
    }
  }

  template<class D>
  void IID_DataPolicy<D>::add_data(Ptr<DataType> d) {
    dat_.push_back(d);
    signal();
  }

  template<class D>
  void IID_DataPolicy<D>::add_data(Ptr<Data> d) {
    add_data(Info::DAT(d));
  }

}  // namespace BOOM
#endif //BOOM_DATA_POLICIES_HPP
