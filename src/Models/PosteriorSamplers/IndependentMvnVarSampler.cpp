/*
  Copyright (C) 2005-2015 Steven L. Scott

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

#include <Models/PosteriorSamplers/IndependentMvnVarSampler.hpp>
#include <distributions.hpp>
#include <cpputil/report_error.hpp>
#include <cpputil/math_utils.hpp>

namespace BOOM {

  IndependentMvnVarSampler::IndependentMvnVarSampler(
      IndependentMvnModel *model,
      const std::vector<Ptr<GammaModelBase>> &priors,
      Vector sd_max_values,
      RNG &seeding_rng)
      : PosteriorSampler(seeding_rng),
        model_(model),
        priors_(priors)
  {
    if (priors.size() != model->dim()) {
      report_error("Prior dimension does not match model in "
                   "IndependentMvnVarSampler");
    }
    if (sd_max_values.empty()) {
      sd_max_values.resize(model->dim(), infinity());
    }
    if (sd_max_values.size() != model->dim()) {
      report_error("sd_max_values.size() != model->dim() in "
                   "IndependentMvnVarSampler");
    }
    for (int i = 0; i < model->dim(); ++i) {
      samplers_.push_back(GenericGaussianVarianceSampler(
          priors_[i], sd_max_values[i]));
    }
  }

  double IndependentMvnVarSampler::logpri() const {
    const Vector &sigsq(model_->sigsq());
    double ans = 0;
    for (int i = 0; i < sigsq.size(); ++i) {
      ans += priors_[i]->logp(1.0 / sigsq[i]);
    }
    return ans;
  }

  void IndependentMvnVarSampler::draw() {
    Ptr<IndependentMvnSuf> suf = model_->suf();
    for (int i = 0; i < model_->dim(); ++i) {
      double sigsq = samplers_[i].draw(
          rng(), suf->n(), suf->centered_sumsq(i, model_->mu()[i]));
      model_->set_sigsq_element(sigsq, i);
    }
  }

}  // namespace BOOM
