/*
  Copyright (C) 2007-2012 Steven L. Scott

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

#ifndef BOOM_FINITE_MIXTURE_MODEL_HPP
#define BOOM_FINITE_MIXTURE_MODEL_HPP

#include <Models/ModelTypes.hpp>
#include <Models/EmMixtureComponent.hpp>
#include <Models/ParamTypes.hpp>
#include <Models/Policies/CompositeParamPolicy.hpp>
#include <Models/Policies/MixtureDataPolicy.hpp>
#include <Models/MultinomialModel.hpp>

namespace BOOM{

  class FiniteMixtureModel
      : public LatentVariableModel,
        public CompositeParamPolicy,
        public MixtureDataPolicy,
        public PriorPolicy
  {
  public:
    FiniteMixtureModel(Ptr<MixtureComponent>, uint S);
    FiniteMixtureModel(Ptr<MixtureComponent>, Ptr<MultinomialModel>);

    template <class M>
    FiniteMixtureModel(std::vector<Ptr<M> >, Ptr<MultinomialModel>);

    template <class FwdIt>
    FiniteMixtureModel(FwdIt Beg, FwdIt End, Ptr<MultinomialModel>);

    FiniteMixtureModel(const FiniteMixtureModel &rhs);
    FiniteMixtureModel * clone()const override;

    void clear_component_data();
    void impute_latent_data(RNG &rng) override;
    void class_membership_probability(Ptr<Data>, Vector &ans)const;
    double last_loglike()const;

    double pdf(dPtr dp, bool logscale)const;
    uint number_of_mixture_components()const;

    const Vector & pi()const;
    const Vector & logpi()const;

    Ptr<MultinomialModel> mixing_distribution();
    const MultinomialModel * mixing_distribution()const;

    Ptr<MixtureComponent> mixture_component(int s);
    const MixtureComponent * mixture_component(int s)const;

    // Returns a matrix of class membership probabilities for each
    // observation.  The table of membership probabilities is
    // re-written with each call to impute_latent_data().
    const Matrix & class_membership_probability()const;

    // Returns a vector giving the latent class to which each
    // observation was assigned during the most recent call to
    // impute_latent_data().
    Vector class_assignment()const;

  protected:
    void set_logpi()const;
    mutable Vector wsp_;

    // Save the class membership probabilities for user i.
    void update_class_membership_probabilities(int i, const Vector &probs);
  private:
    std::vector<Ptr<MixtureComponent> > mixture_components_;
    Ptr<MultinomialModel> mixing_dist_;
    mutable Vector logpi_;
    mutable bool logpi_current_;
    void observe_pi()const;
    void set_observers();
    virtual std::vector<Ptr<MixtureComponent> > models();
    virtual const std::vector<Ptr<MixtureComponent> > models()const;
    double last_loglike_;
    Matrix class_membership_probabilities_;
    std::vector<int> which_mixture_component_;
  };
  //----------------------------------------------------------------------
  template <class FwdIt>
  FiniteMixtureModel::FiniteMixtureModel(FwdIt Beg, FwdIt End,
                                         Ptr<MultinomialModel> MixDist)
    : DataPolicy(MixDist->dim()),
      mixture_components_(Beg,End),
      mixing_dist_(MixDist)
  {
    set_observers();
  }

  template <class M>
  FiniteMixtureModel::FiniteMixtureModel(std::vector<Ptr<M> > Models,
                                         Ptr<MultinomialModel> MixDist)
    : DataPolicy(MixDist->dim()),
      mixture_components_(Models.begin(), Models.end()),
      mixing_dist_(MixDist)
  {
    set_observers();
  }

//======================================================================
  class EmFiniteMixtureModel : public FiniteMixtureModel
  {
   public:
    EmFiniteMixtureModel(Ptr<EmMixtureComponent>, uint S);
    EmFiniteMixtureModel(Ptr<EmMixtureComponent>, Ptr<MultinomialModel>);

    template <class M>
    EmFiniteMixtureModel(std::vector<Ptr<M> > mixture_components,
                         Ptr<MultinomialModel> mixing_distribution)
        : FiniteMixtureModel(mixture_components, mixing_distribution),
          em_mixture_components_(mixture_components.begin(),
                                 mixture_components.end())
    {}

    template <class FwdIt>
    EmFiniteMixtureModel(FwdIt Beg, FwdIt End,
                         Ptr<MultinomialModel> mixing_distribution)
        : FiniteMixtureModel(Beg, End, mixing_distribution),
          em_mixture_components_(Beg, End)
    {}

    EmFiniteMixtureModel(const EmFiniteMixtureModel &rhs);
    EmFiniteMixtureModel * clone()const override;

    double loglike()const;
    void mle();

    // The EStep returns the observed data likelihood
    double EStep();
    void MStep(bool posterior_mode);

    Ptr<EmMixtureComponent> em_mixture_component(int s);
    const EmMixtureComponent * em_mixture_component(int s)const;
   private:
    std::vector<Ptr<EmMixtureComponent> > em_mixture_components_;
    void populate_em_mixture_components();
  };

}
#endif// BOOM_FINITE_MIXTURE_MODEL_HPP
