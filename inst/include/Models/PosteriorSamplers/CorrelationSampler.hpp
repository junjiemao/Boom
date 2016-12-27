/*
  Copyright (C) 2006 Steven L. Scott

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
#ifndef BOOM_CORRELATION_SAMPLER_HPP
#define BOOM_CORRELATION_SAMPLER_HPP
#include <Samplers/Sampler.hpp>
#include <Models/PosteriorSamplers/PosteriorSampler.hpp>
#include <TargetFun/TargetFun.hpp>
#include <Models/ParamTypes.hpp>
#include <Models/ModelTypes.hpp>
#include <Models/MvnModel.hpp>
namespace BOOM{

  class CorrTF
    : public TargetFun
  {
  public:
    CorrTF(Ptr<SpdParams>,
       Ptr<LoglikeModel>,
       Ptr<CorrelationModel>);
    CorrTF(const CorrTF &rhs);
    //    CorrTF * clone()const;
    double operator()(const Vector &x)const override;
  private:
    mutable CorrelationMatrix R, R2;
    mutable Ptr<SpdParams> spd;
    mutable Ptr<LoglikeModel> mod;
    mutable Vector wsp;
    Ptr<CorrelationModel> pri;
    bool refresh;
  };

  // Draws from the posterior distribution of the correlation matrix
  // in a Gaussian model with known means and variances given a
  // CorrelationModel prior distribution on the correlation matrix
  class MvnCorrelationSampler
    : public PosteriorSampler
  {
  public:
    MvnCorrelationSampler(MvnModel *,
              Ptr<CorrelationModel>,
              bool refresh_suf = false,
        RNG &seeding_rng = GlobalRng::rng);
    void draw() override;
    double logpri()const override;
  private:
    double logp(double r);
    void find_limits();
    void draw_one();
    double Rdet(double r);
    void set_r(double r);
    void check_limits(double oldr, double eps);

    MvnModel *mod_;              // supplies likelihood
    Ptr<CorrelationModel> pri_;  // prior for R
    CorrelationMatrix R_;        // workspace
    SpdMatrix Sumsq_;
    double df_;
    int i_, j_;
    double lo_, hi_;
  };

}
#endif// BOOM_CORRELATION_SAMPLER_HPP
