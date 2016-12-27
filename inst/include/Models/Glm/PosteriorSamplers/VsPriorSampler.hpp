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

#ifndef BOOM_VS_PRIOR_SAMPLER_HPP
#define BOOM_VS_PRIOR_SAMPLER_HPP
#include <Models/Glm/VariableSelectionPrior.hpp>
#include <Models/BetaModel.hpp>
#include <vector>
#include <LinAlg/Selector.hpp>

namespace BOOM{

  class VsPriorSampler
    : public PosteriorSampler
  {
    // This sampler is for the parameters of a VariableSelectionPrior,
    // which are a bunch of independent binomial probabilities 'pi'.
    // This sampler updates 'pi'.

    typedef VariableSelectionPrior VSP;
  public:
    VsPriorSampler(VSP *, Ptr<BetaModel>, RNG &seeding_rng = GlobalRng::rng);
    VsPriorSampler(VSP *, std::vector<Ptr<BetaModel> >,
                   RNG &seeding_rng = GlobalRng::rng);
    VsPriorSampler(VSP *, const Vector & pi_guess, const Vector & wgt,
                   RNG &seeding_rng = GlobalRng::rng);

    VsPriorSampler(VSP *, std::vector<Ptr<BetaModel> >,
           const Selector & forced_in, const Selector &forced_out,
       RNG &seeding_rng = GlobalRng::rng);

    void draw() override;
    double logpri()const override;
    uint potential_nvars()const;
  private:

    VSP *vsp;
    Selector forced_in_;
    Selector forced_out_;
    std::vector<Ptr<PosteriorSampler> > sam_;

    bool is_fixed(uint i, double &p)const;
    double logpri(uint i)const;

  };

}
#endif// BOOM_VS_PRIOR_SAMPLER_HPP
