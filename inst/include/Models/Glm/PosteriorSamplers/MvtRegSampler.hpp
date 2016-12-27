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
#ifndef BOOM_MVT_REG_SAMPLER_HPP
#define BOOM_MVT_REG_SAMPLER_HPP

#include <Models/PosteriorSamplers/PosteriorSampler.hpp>
#include <Models/Glm/MvtRegModel.hpp>
#include <Models/Glm/MvReg2.hpp>
#include <Models/Glm/PosteriorSamplers/MvRegSampler.hpp>
#include <Models/ScaledChisqModel.hpp>
#include <Models/ScaledChisqModel.hpp>
#include <Samplers/SliceSampler.hpp>

namespace BOOM{
  class GammaModel;
  class ScalarLogpostTF;
  class SliceSampler;

  class MvtRegSampler
    : public PosteriorSampler
  {
  public:
    // assumes vec(B)|Sigma ~ N( b, kappa * Sigma^{-1} \otimes I_p )

    MvtRegSampler(MvtRegModel *m, const Matrix &B, double kappa,
          double prior_df, const SpdMatrix & Sigma_guess,
          Ptr<DoubleModel> nu_prior, RNG &seeding_rng = GlobalRng::rng);

    void draw() override;
    double logpri()const override;
  private:
    MvtRegModel *mod;

    Ptr<MvReg> reg_model;  // update sufficient statistics but not data
    Ptr<MvRegSampler> reg_sampler;

    Ptr<ScaledChisqModel> nu_model;
    Ptr<DoubleModel> nu_prior;
    Ptr<SliceSampler> nu_sampler;

    Vector yhat;
    void impute_w();
    double impute_w(Ptr<MvRegData> dp);
    void draw_Sigma();
    void draw_Beta();
    void draw_nu();
    void clear_suf();
  };

}
#endif// BOOM_MVT_REG_SAMPLER_HPP
