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
#ifndef BOOM_METROPOLIS_HASTINGS_HPP_
#define BOOM_METROPOLIS_HASTINGS_HPP_
#include <Samplers/Sampler.hpp>
#include <Samplers/MH_Proposals.hpp>
#include <boost/function.hpp>

namespace BOOM{

  class MetropolisHastings : public Sampler{
   public:
    typedef boost::function<double(const Vector &)> Target;
    MetropolisHastings(const Target & target, Ptr<MH_Proposal> prop,
                       RNG *rng = 0);
    Vector draw(const Vector & old) override;
    virtual double logp(const Vector &x)const;
    bool last_draw_was_accepted()const;
   protected:
    void set_proposal(Ptr<MH_Proposal>);
    void set_target(Target f);
   private:
    Target f_;
    Ptr<MH_Proposal> prop_;
    Vector cand_;
    bool accepted_;
  };

  class ScalarMetropolisHastings : public ScalarSampler{
   public:
    typedef boost::function<double(double)> ScalarTarget;
    ScalarMetropolisHastings(const ScalarTarget &f,
                             Ptr<MH_ScalarProposal> prop,
                             RNG *rng = 0);
    double draw(double old) override;
    virtual double logp(double x)const;
    bool last_draw_was_accepted()const;
   private:
    ScalarTarget f_;
    Ptr<MH_ScalarProposal> prop_;
    bool accepted_;
  };

}
#endif // BOOM_METROPOLIS_HASTINGS_HPP_
