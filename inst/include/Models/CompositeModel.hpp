/*
  Copyright (C) 2007 Steven L. Scott

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

#ifndef BOOM_COMPOSITE_MODEL_HPP
#define BOOM_COMPOSITE_MODEL_HPP
#include <Models/ModelTypes.hpp>
#include <Models/CompositeData.hpp>
#include <Models/Policies/CompositeParamPolicy.hpp>
#include <Models/Policies/IID_DataPolicy.hpp>
#include <Models/Policies/PriorPolicy.hpp>

namespace BOOM{

  // A composite model assumes that y1, y2, ... are independent with
  // y1 ~ m1, y2 ~ m2, etc.  y1,y2... are stored in CompositeData,
  // and m1, m2, etc. are stored here
  //
  // The learning method should be set (using set_method) for the
  // component models before they are loaded in to the CompositeModel.
  class CompositeModel
    : virtual public MixtureComponent,
      public CompositeParamPolicy,
      public IID_DataPolicy<CompositeData>,
      public PriorPolicy
  {
  public:
    // The default constructor can be used in conjuction with
    // add_model if you want to build the mode up incrementally.
    CompositeModel();

    // This constructor can be used if you've already got a vector of
    // model pointers.  Note that they have to be of the same type but
    // it can be any type that inherits from MixtureComponent.
    template <class MOD>
    CompositeModel(const std::vector<Ptr<MOD> > &models)
        : m_(models.begin(), models.end())
    {
      setup();
    }

    CompositeModel(const CompositeModel &rhs);
    CompositeModel * clone()const override;

    virtual void add_model(Ptr<MixtureComponent>);

    void add_data(Ptr<CompositeData>) override;
    void add_data(Ptr<Data>) override;
    void clear_data() override;

    double pdf(const CompositeData &, bool logscale)const;
    double pdf(Ptr<Data>, bool logscale)const;
    double pdf(const Data *, bool logscale)const override;

    std::vector<Ptr<MixtureComponent> > &components();
    const std::vector<Ptr<MixtureComponent> > &components()const;

  protected:
    template <class Fwd>
    void set_models(Fwd b, Fwd e){   // to be called by constructors of
      m_.assign(b,e);                // derived classes
      setup(); }
  private:
    std::vector<Ptr<MixtureComponent> > m_;
    void setup();
  };

}

#endif // BOOM_COMPOSITE_MODEL_HPP
