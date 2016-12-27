/*
  Copyright (C) 2005-2014 Steven L. Scott

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

#ifndef BOOM_REGRESSION_MODEL_H
#define BOOM_REGRESSION_MODEL_H

#include <BOOM.hpp>
#include <uint.hpp>
#include <Models/Glm/Glm.hpp>
#include <LinAlg/QR.hpp>
#include <Models/Sufstat.hpp>
#include <Models/ParamTypes.hpp>
#include <Models/Policies/ParamPolicy_2.hpp>
#include <Models/Policies/IID_DataPolicy.hpp>
#include <Models/Policies/SufstatDataPolicy.hpp>
#include <Models/Policies/PriorPolicy.hpp>
#include <Models/EmMixtureComponent.hpp>

namespace BOOM{

  class RegressionConjSampler;
  class DesignMatrix;
  class MvnGivenXandSigma;
  class GammaModel;

  class AnovaTable{
   public:
    double SSE, SSM, SST;
    double MSM, MSE;
    double df_error, df_model, df_total;
    double F, p_value;
    ostream & display(ostream &out) const;
  };

  ostream & operator<<(ostream &out, const AnovaTable &tab);

  Matrix add_intercept(const Matrix &X);
  Vector add_intercept(const Vector &X);

  //------- virtual base for regression sufficient statistics ----
  class RegSuf: virtual public Sufstat{
  public:
    typedef std::vector<Ptr<RegressionData> > dataset_type;
    typedef Ptr<dataset_type, false> dsetPtr;

    RegSuf * clone()const override = 0;

    virtual uint size()const = 0;  // dimension of beta
    virtual double yty()const = 0;
    virtual Vector xty()const = 0;
    virtual SpdMatrix xtx()const = 0;

    virtual Vector xty(const Selector &)const = 0;
    virtual SpdMatrix xtx(const Selector &)const = 0;

    // (X - Xbar)^T * (X - Xbar)
    //  = xtx - n * xbar xbar^T
    SpdMatrix centered_xtx() const;

    // return least squares estimates of regression params
    virtual Vector beta_hat()const = 0;
    virtual double SSE()const = 0;  // SSE measured from ols beta
    virtual double SST()const = 0;
    virtual double ybar()const = 0;
    // Column means of the design matrix.
    virtual Vector xbar()const = 0;
    virtual double n()const = 0;

    // Compute the sum of square errors using the given set of
    // coefficients, taking advantage of sparsity.
    double relative_sse(const GlmCoefs &beta) const;

    AnovaTable anova() const;

    virtual void add_mixture_data(double y, const Vector &x, double prob) = 0;
    virtual void add_mixture_data(double y, const ConstVectorView &x,
                                  double prob) = 0;
    virtual void combine(Ptr<RegSuf>) = 0;

    ostream &print(ostream &out) const override;
  };
  inline ostream & operator<<(ostream &out, const RegSuf &suf){
    return suf.print(out);
  }
  //------------------------------------------------------------------
  class QrRegSuf :
    public RegSuf,
    public SufstatDetails<RegressionData>
  {
  public:
    QrRegSuf(const Matrix &X, const Vector &y);

    QrRegSuf *clone() const override;
    void clear() override;
    void Update(const DataType &) override;
    void add_mixture_data(
        double y, const Vector &x, double prob) override;
    void add_mixture_data(
        double y, const ConstVectorView &x, double prob) override;
    uint size() const override;  // dimension of beta
    double yty() const override;
    Vector xty() const override;
    SpdMatrix xtx() const override;

    Vector xty(const Selector &) const override;
    SpdMatrix xtx(const Selector &) const override;

    Vector beta_hat() const override;
    virtual Vector beta_hat(const Vector &y) const;
    double SSE() const override;
    double SST() const override;
    double ybar() const override;
    Vector xbar() const override;
    double n() const override;
    void refresh_qr(const std::vector<Ptr<DataType> > &) const ;
    //    void check_raw_data(const Matrix &X, const Vector &y);
    void combine(Ptr<RegSuf>) override;
    virtual void combine(const RegSuf &);
    QrRegSuf * abstract_combine(Sufstat *s) override;

    Vector vectorize(bool minimal=true) const override;
    Vector::const_iterator unvectorize(Vector::const_iterator &v,
                                            bool minimal = true) override;
    Vector::const_iterator unvectorize(const Vector &v,
                                            bool minimal = true) override;
    ostream &print(ostream &out) const override;
   private:
    mutable QR qr;
    mutable Vector Qty;
    mutable double sumsqy;
    mutable bool current;
    mutable Vector x_column_sums_;
  };
  //------------------------------------------------------------------
  class NeRegSuf
    : public RegSuf,
      public SufstatDetails<RegressionData>
  {   // directly solves 'normal equations'
  public:
    // An empty, but right-sized set of sufficient statistics.
    NeRegSuf(uint p);

    // Build from the design matrix X and response vector y.
    NeRegSuf(const Matrix &X, const Vector &y);

    // Build from the indiviudal sufficient statistic components.  The
    // 'n' is needed because X might not have an intercept term.
    NeRegSuf(const SpdMatrix &xtx,
             const Vector &xty,
             double yty,
             double n,
             const Vector &xbar);

    // Build from a sequence of Ptr<RegressionData>
    template <class Fwd> NeRegSuf(Fwd b, Fwd e);
    NeRegSuf *clone() const override;

    // If fixed, then xtx will not be changed by a call to clear(),
    // add_mixture_data(), or any of the flavors of Update().
    void fix_xtx(bool tf = true);

    void clear() override;
    void add_mixture_data(
        double y, const Vector &x, double prob) override;
    void add_mixture_data(
        double y, const ConstVectorView &x, double prob) override;
    void Update(const RegressionData & rdp) override;
    uint size() const override;  // dimension of beta
    double yty() const override;
    Vector xty() const override;
    SpdMatrix xtx() const override;
    Vector xty(const Selector &) const override;
    SpdMatrix xtx(const Selector &) const override;
    Vector beta_hat() const override;
    double SSE() const override;
    double SST() const override;
    double ybar() const override;
    Vector xbar() const override;
    double n() const override;
    void combine(Ptr<RegSuf>) override;
    void combine(const RegSuf &) ;
    NeRegSuf * abstract_combine(Sufstat *s) override;

    Vector vectorize(bool minimal=true) const override;
    Vector::const_iterator unvectorize(
        Vector::const_iterator &v, bool minimal=true) override;
    Vector::const_iterator unvectorize(
        const Vector &v, bool minimal=true) override;
    ostream &print(ostream &out) const override;

    // Adding data only updates the upper triangle of xtx_.  Calling
    // reflect() fills the lower triangle as well, if needed.
    void reflect() const;
  private:
    mutable SpdMatrix xtx_;
    mutable bool needs_to_reflect_;
    Vector xty_;
    bool xtx_is_fixed_;
    double sumsqy;
    double n_;
    double sumy_;
    Vector x_column_sums_;
  };

  template <class Fwd>
  NeRegSuf::NeRegSuf(Fwd b, Fwd e){
    Ptr<RegressionData> dp = *b;
    uint p = dp->xdim();
    xtx_ = SpdMatrix(p, 0.0);
    xty_ = Vector(p, 0.0);
    sumsqy = 0.0;
    while(b!=e){
      update(*b);
      ++b;
    }
  }


  //------------------------------------------------------------------
  class RegressionDataPolicy
    : public SufstatDataPolicy<RegressionData, RegSuf>
  {
  public:
    typedef RegressionDataPolicy DataPolicy;
    typedef SufstatDataPolicy<RegressionData, RegSuf> DPBase;

    RegressionDataPolicy(Ptr<RegSuf>);
    RegressionDataPolicy(Ptr<RegSuf>, const DatasetType &d);
    template <class FwdIt>
    RegressionDataPolicy(Ptr<RegSuf>, FwdIt Begin, FwdIt End);

    RegressionDataPolicy(const RegressionDataPolicy &);
    RegressionDataPolicy * clone()const override = 0;
    RegressionDataPolicy & operator=(const RegressionDataPolicy &);

  };
  template <class Fwd>
  RegressionDataPolicy::RegressionDataPolicy(Ptr<RegSuf> s, Fwd b, Fwd e)
    : DPBase(s,b,e)
  {}

  //------------------------------------------------------------------

  class RegressionModel
    : public GlmModel,
      public ParamPolicy_2<GlmCoefs, UnivParams>,
      public RegressionDataPolicy,
      public PriorPolicy,
      public NumOptModel,
      public EmMixtureComponent
  {
 public:
    RegressionModel(uint p);
    RegressionModel(const Vector &b, double Sigma);

    // Use this constructor if the model needs to share parameters
    // with another model.  E.g. a mixture model with shared variance
    // parameter.
    RegressionModel(Ptr<GlmCoefs> beta, Ptr<UnivParams> sigsq);

    // Args:
    //   X: The design matrix of predictor variables.  Must contain an
    //     explicit column of 1's if an intercept term is desired.
    //   y: The vector of responses.  The length of y must match the
    //     number of rows in X.
    // Iniitializes the model with the least squares fit.
    RegressionModel(const Matrix &X, const Vector &y);

    RegressionModel(const DatasetType &d, bool include_all_variables = true);
    RegressionModel(const RegressionModel &rhs);
    RegressionModel * clone() const override;

    // The number of variables currently included in the model,
    // including the intercept, if present.
    uint nvars() const;

    // The number of potential variables, including the intercept.
    uint nvars_possible() const;

    //---- parameters ----
    GlmCoefs & coef() override;
    const GlmCoefs & coef() const override;
    Ptr<GlmCoefs> coef_prm() override;
    const Ptr<GlmCoefs> coef_prm() const override;
    Ptr<UnivParams> Sigsq_prm();
    const Ptr<UnivParams> Sigsq_prm() const;

    void set_sigsq(double s2);

    double sigsq() const;
    double sigma() const;

    //---- simulate regression data  ---
    virtual RegressionData * simdat() const;
    virtual RegressionData * simdat(const Vector &X) const;
    Vector simulate_fake_x() const;  // no intercept

    //---- estimation ---
    SpdMatrix xtx(const Selector &inc) const;
    Vector xty(const Selector &inc) const;
    SpdMatrix xtx() const;      // adjusts for covariate inclusion-
    Vector xty() const;      // exclusion, and includes weights,
    double yty() const;   // if used

    void make_X_y(Matrix &X, Vector &y) const;

    //--- probability calculations ----
    void mle() override;
    // The argument 'sigsq_beta' is a Vector with the first element
    // corresponding to the residual variance parameter, and the
    // remaining elements corresponding to the set of included
    // coefficients.
    double Loglike(const Vector &sigsq_beta,
                           Vector &g, Matrix &h, uint nd) const override;
    virtual double pdf(dPtr, bool) const;
    double pdf(const Data *, bool) const override;

    // The log likelihood when beta is empty (i.e. all coefficients,
    // including the intercept, are zero).
    double empty_loglike(Vector &g, Matrix &h, uint nd) const;

    // If the model was formed using the QR decomposition, switch to
    // using the normal equations.  The normal equations are
    // computationally more efficient when doing variable selection or
    // when the data is changing between MCMC iterations (as in finite
    // mixtures).
    void use_normal_equations();

    void add_mixture_data(Ptr<Data>, double prob) override;

    //--- diagnostics ---
    AnovaTable anova()const{return suf()->anova();}
  };

}  // namespace BOOM

#endif  // BOOM_REGRESSION_MODEL_H
