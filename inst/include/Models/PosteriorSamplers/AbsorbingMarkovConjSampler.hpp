/*
  Copyright (C) 2005-2009 Steven L. Scott

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

#ifndef BOOM_ABSORBING_MARKOV_CONJUGATE_SAMPLER_HPP
#define BOOM_ABSORBING_MARKOV_CONJUGATE_SAMPLER_HPP

#include <Models/MarkovModel.hpp>
#include <Models/PosteriorSamplers/MarkovConjSampler.hpp>
#include <LinAlg/Selector.hpp>

namespace BOOM{

  class AbsorbingMarkovConjSampler
    : public MarkovConjSampler
  {
  public:
    AbsorbingMarkovConjSampler(MarkovModel * Mod,
                               Ptr<ProductDirichletModel> Q,
                               Ptr<DirichletModel> pi0,
                               std::vector<uint> absorbing_states,
                               RNG &seeding_rng = GlobalRng::rng);
    AbsorbingMarkovConjSampler(MarkovModel * Mod,
                               Ptr<ProductDirichletModel> Q,
                               std::vector<uint> absorbing_states,
                               RNG &seeding_rng = GlobalRng::rng);
    AbsorbingMarkovConjSampler(MarkovModel * Mod,
                               const Matrix & Nu,
                               std::vector<uint> absorbing_states,
                               RNG &seeding_rng = GlobalRng::rng);
    AbsorbingMarkovConjSampler(MarkovModel * Mod,
                               const Matrix & Nu,
                               const Vector & nu,
                               std::vector<uint> absorbing_states,
                               RNG &seeding_rng = GlobalRng::rng);

    double logpri() const override;
    void draw() override;
    void find_posterior_mode(double epsilon = 1e-5) override;
    bool can_find_posterior_mode() const override {
      return true;
    }

  private:
    MarkovModel * mod_;
    Selector abs_;
    Selector trans_;
  };

}  // namespace BOOM

#endif// BOOM_ABSORBING_MARKOV_CONJUGATE_SAMPLER_HPP
