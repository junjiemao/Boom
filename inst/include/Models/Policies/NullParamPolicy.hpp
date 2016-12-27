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
#ifndef BOOM_NULL_PARAM_POLICY_HPP
#define BOOM_NULL_PARAM_POLICY_HPP

#include<cpputil/Ptr.hpp>
#include <Models/ModelTypes.hpp>

namespace BOOM{

  class NullParamPolicy : virtual public Model{
    // for use with models that have no parameters: e.g. uniform
    // distributions.
  public:
    typedef NullParamPolicy ParamPolicy;

    NullParamPolicy();
    NullParamPolicy(const NullParamPolicy &rhs);
    NullParamPolicy * clone()const override =0;
    NullParamPolicy & operator=(const NullParamPolicy &);

    // over-rides for abstract base Model
    ParamVector t() override;
    const ParamVector t()const override;
  };
  //------------------------------------------------------------

}
#endif // BOOM_PARAM_POLICY_1_HPP
