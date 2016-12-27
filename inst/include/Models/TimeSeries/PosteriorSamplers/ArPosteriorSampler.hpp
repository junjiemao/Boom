/*
  Copyright (C) 2005-2012 Steven L. Scott

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

#ifndef BOOM_ARP_POSTERIOR_SAMPLER_HPP_
#define BOOM_ARP_POSTERIOR_SAMPLER_HPP_

#include <Models/PosteriorSamplers/PosteriorSampler.hpp>
#include <Models/PosteriorSamplers/GenericGaussianVarianceSampler.hpp>
#include <Models/TimeSeries/ArModel.hpp>
#include <Models/GammaModel.hpp>

namespace BOOM{

  // A sampler for an AR(p) process, assuming a uniform prior over the
  // AR coefficients with support over the stationary region, and an
  // inverse Gamma prior on innovation variance.
  class ArPosteriorSampler : public PosteriorSampler{
   public:
    ArPosteriorSampler(ArModel *model,
                       Ptr<GammaModelBase> siginv_prior,
                       RNG &seeding_rng = GlobalRng::rng);

    // The 'draw' method will make several attempts to simulate AR
    // coefficients directly from the posterior distribution of a
    // regression model conditional on sigma.  If the maximum number
    // of proposals is exceeded then a series of univariate draws will
    // be made starting from the current value of the AR coefficients.
    void draw() override;

    void draw_sigma();
    void draw_phi();

    // Uses a univariate slice sampler to draw each component of phi
    // given the others.
    void draw_phi_univariate();

    // Returns -infinity if the coefficients are outside of the legal
    // range.  Returns logp(siginv) otherwise.
    double logpri() const override;

    void set_max_number_of_regression_proposals(int number_of_proposals);

    void set_sigma_upper_limit(double max_sigma);
   private:
    ArModel *model_;
    Ptr<GammaModelBase> siginv_prior_;
    int max_number_of_regression_proposals_;
    GenericGaussianVarianceSampler sigsq_sampler_;
  };

}

#endif //  BOOM_ARP_POSTERIOR_SAMPLER_HPP_
