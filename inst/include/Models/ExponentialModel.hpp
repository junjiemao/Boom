/*
  Copyright (C) 2005 Steven L. Scott

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

#ifndef EXPONENTIAL_MODEL_H
#define EXPONENTIAL_MODEL_H
#include <iosfwd>
#include <cpputil/Ptr.hpp>
#include <Models/ModelTypes.hpp>
#include <Models/DoubleModel.hpp>
#include <Models/EmMixtureComponent.hpp>
#include <Models/Sufstat.hpp>
#include <Models/Policies/SufstatDataPolicy.hpp>
#include <Models/Policies/ParamPolicy_1.hpp>
#include <Models/Policies/PriorPolicy.hpp>


//======================================================================
namespace BOOM{
  class ExpSuf: public SufstatDetails<DoubleData>{
    double sum_, n_;
  public:
    ExpSuf();
    ExpSuf(const ExpSuf &);
    ExpSuf *clone() const override;

    void clear() override;
    void Update(const DoubleData &dat) override;
    void add_mixture_data(double y, double prob);
    double sum()const;
    double n()const;
    void combine(Ptr<ExpSuf>);
    void combine(const ExpSuf &);
    ExpSuf * abstract_combine(Sufstat *s) override;
    Vector vectorize(bool minimal=true)const override;
    Vector::const_iterator unvectorize(Vector::const_iterator &v,
					    bool minimal=true) override;
    Vector::const_iterator unvectorize(const Vector &v,
					    bool minimal=true) override;
    ostream &print(ostream &out)const override;
  };
  //======================================================================
  class GammaModel;
  class ExponentialGammaSampler;

  class ExponentialModel:
    public ParamPolicy_1<UnivParams>,
    public SufstatDataPolicy<DoubleData,ExpSuf>,
    public PriorPolicy,
    public DiffDoubleModel,
    public NumOptModel,
    public EmMixtureComponent
  {
  public:
    ExponentialModel();
    ExponentialModel(double lam);
    ExponentialModel(const ExponentialModel &m);
    ExponentialModel *clone() const override;

    Ptr<UnivParams> Lam_prm();
    const Ptr<UnivParams> Lam_prm()const;
    const double& lam() const;
    void set_lam(double);

    void set_conjugate_prior(double a, double b);
    void set_conjugate_prior(Ptr<GammaModel>);
    void set_conjugate_prior(Ptr<ExponentialGammaSampler>);

    // probability calculations
    double pdf(Ptr<Data> dp, bool logscale)const override;
    double pdf(const Data * dp, bool logscale)const override;
    double Loglike(const Vector &lambda_as_vector,
                   Vector &g, Matrix &h, uint nd) const override ;
    double Logp(double x, double &g, double &h, const uint lev) const override ;
    void mle() override;

    double sim() const override;
    void add_mixture_data(Ptr<Data>, double prob) override;
  };



}
#endif  // EXPONENTIALMODEL_H
