/*
  Copyright (C) 2013 Steven L. Scott

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

#ifndef BOOM_PROBIT_BART_HPP_
#define BOOM_PROBIT_BART_HPP_

// This code is untested.  TODO(stevescott):  test it.

#include <Models/Bart/Bart.hpp>
#include <Models/Glm/BinomialRegressionData.hpp>
#include <Models/Policies/NonparametricParamPolicy.hpp>
#include <Models/Policies/IID_DataPolicy.hpp>
#include <Models/Policies/PriorPolicy.hpp>

namespace BOOM {

  class ProbitBartModel
      : public BartModelBase,
        public NonparametricParamPolicy,
        public IID_DataPolicy<BinomialRegressionData>,
        public PriorPolicy {
   public:
    ProbitBartModel(int number_of_trees, double mean = 0.0);
    ProbitBartModel(int number_of_trees,
                    const std::vector<int>  &responses,
                    const std::vector<int> &trials,
                    const Matrix &predictors);
    ProbitBartModel(int number_of_trees,
                    const std::vector<bool> &responses,
                    const Matrix &predictors);
    ProbitBartModel(const ProbitBartModel &rhs);
    ProbitBartModel * clone() const override;
    int sample_size()const override;
    void add_data(Ptr<Data>) override;
    void add_data(Ptr<BinomialRegressionData>) override;
   private:
    void check_predictor_dimension(int number_of_observations,
                                   const Matrix &predictors)const ;
  };

}
#endif //  BOOM_PROBIT_BART_HPP_
