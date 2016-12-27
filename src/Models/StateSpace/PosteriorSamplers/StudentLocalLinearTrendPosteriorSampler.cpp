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

#include <Models/StateSpace/PosteriorSamplers/StudentLocalLinearTrendPosteriorSampler.hpp>
#include <cpputil/math_utils.hpp>
#include <distributions.hpp>
#include <distributions/trun_gamma.hpp>
#include <Samplers/ScalarSliceSampler.hpp>

namespace {
  // A local namespace for minor implementation details.
  class NuPosterior {
   public:
    NuPosterior(const BOOM::DoubleModel *nu_prior,
                const BOOM::GammaSuf *suf)
        : nu_prior_(nu_prior), suf_(suf) {}

    // Returns the un-normalized log posterior evaulated at nu.
    double operator()(double nu)const{
      double n = suf_->n();
      double sum = suf_->sum();
      double sumlog = suf_->sumlog();
      double nu2 = nu / 2.0;

      double ans = nu_prior_->logp(nu);
      ans += n * (nu2 * log(nu2) - lgamma(nu2));
      ans += (nu2 - 1) * sumlog;
      ans -= nu2 * sum;
      return ans;
    }
   private:
    const BOOM::DoubleModel *nu_prior_;
    const BOOM::GammaSuf *suf_;
  };

}  // namespace

namespace BOOM {
  StudentLocalLinearTrendPosteriorSampler::StudentLocalLinearTrendPosteriorSampler(
      StudentLocalLinearTrendStateModel *model,
      Ptr<GammaModelBase> sigsq_level_prior,
      Ptr<DoubleModel> nu_level_prior,
      Ptr<GammaModelBase> sigsq_slope_prior,
      Ptr<DoubleModel> nu_slope_prior,
      RNG &seeding_rng)
      : PosteriorSampler(seeding_rng),
        model_(model),
        sigsq_level_prior_(sigsq_level_prior),
        nu_level_prior_(nu_level_prior),
        sigsq_slope_prior_(sigsq_slope_prior),
        nu_slope_prior_(nu_slope_prior),
        sigsq_level_sampler_(sigsq_level_prior_),
        sigsq_slope_sampler_(sigsq_slope_prior_)
  {}

  double StudentLocalLinearTrendPosteriorSampler::logpri()const{
    return sigsq_level_prior_->logp(1.0 / model_->sigsq_level())
        + nu_level_prior_->logp(model_->nu_level())
        + sigsq_slope_prior_->logp(1.0 / model_->sigsq_slope())
        + nu_slope_prior_->logp(1.0 / model_->nu_slope());
  }

  void StudentLocalLinearTrendPosteriorSampler::draw(){
    draw_sigsq_level();
    draw_nu_level();
    draw_sigsq_slope();
    draw_nu_slope();
  }

  void StudentLocalLinearTrendPosteriorSampler::set_sigma_level_upper_limit(
      double upper_limit){
    sigsq_level_sampler_.set_sigma_max(upper_limit);
  }

  void StudentLocalLinearTrendPosteriorSampler::set_sigma_slope_upper_limit(
      double upper_limit){
    sigsq_slope_sampler_.set_sigma_max(upper_limit);
  }

  void StudentLocalLinearTrendPosteriorSampler::draw_sigsq_level(){
    const WeightedGaussianSuf &suf(
        model_->sigma_level_complete_data_suf());
    double sigsq = sigsq_level_sampler_.draw(rng(), suf.n(), suf.sumsq());
    model_->set_sigsq_level(sigsq);
  }

  void StudentLocalLinearTrendPosteriorSampler::draw_sigsq_slope(){
    const WeightedGaussianSuf &suf(
        model_->sigma_slope_complete_data_suf());
    double sigsq = sigsq_slope_sampler_.draw(rng(), suf.n(), suf.sumsq());
    model_->set_sigsq_slope(sigsq);
  }

  void StudentLocalLinearTrendPosteriorSampler::draw_nu_level(){
    NuPosterior logpost(nu_level_prior_.get(),
                        &model_->nu_level_complete_data_suf());
    ScalarSliceSampler sampler(logpost, true);
    sampler.set_lower_limit(0.0);
    double nu = sampler.draw(model_->nu_level());
    model_->set_nu_level(nu);
  }

  void StudentLocalLinearTrendPosteriorSampler::draw_nu_slope(){
    NuPosterior logpost(nu_slope_prior_.get(),
                        &model_->nu_slope_complete_data_suf());
    ScalarSliceSampler sampler(logpost, true);
    sampler.set_lower_limit(0.0);
    double nu = sampler.draw(model_->nu_slope());
    model_->set_nu_slope(nu);
  }

} // namespace BOOM
