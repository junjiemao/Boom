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

#include <Models/Glm/RegressionModel.hpp>

#include <Models/PosteriorSamplers/PosteriorSampler.hpp>
#include <Models/GammaModel.hpp>
#include <Models/Glm/MvnGivenXandSigma.hpp>
#include <sstream>
#include <cmath>
#include <distributions.hpp>
#include <Models/SufstatAbstractCombineImpl.hpp>

namespace BOOM{
  inline void incompatible_X_and_y(const Matrix &X, const Vector &y){
    ostringstream out;
    out << "incompatible X and Y" << endl
        << "X = " << endl << X << endl
        << "Y = " << endl << y << endl;
    report_error(out.str());
  };

  inline void index_out_of_bounds(uint i, uint bound){
    ostringstream out;
    out << "requested index " << i << " out of bounds." << endl
        << "bound is " << bound << "."<< endl;
    report_error(out.str());
  };

  Matrix add_intercept(const Matrix &X){
    Vector one(X.nrow(), 1.0);
    return cbind(one, X); }

  Vector add_intercept(const Vector &x){ return concat(1.0, x); }

  //============================================================
  SpdMatrix RegSuf::centered_xtx() const {
    SpdMatrix ans = xtx();
    ans.add_outer(xbar(), -n());
    return ans;
  }

  double RegSuf::relative_sse(const GlmCoefs &coefficients) const {
    double ans = yty();
    const Selector &inc(coefficients.inc());
    if (inc.nvars() == 0) {
      return ans;
    }
    const SpdMatrix xtx(this->xtx(inc));
    const Vector xty(this->xty(inc));
    const Vector beta(coefficients.included_coefficients());
    ans += beta.dot(xtx * beta) - 2 * beta.dot(xty);
    return ans;
  }

  AnovaTable RegSuf::anova()const{
    AnovaTable ans;
    double nobs = n();
    double p = size();  // p+1 really

    ans.SSE = SSE();
    ans.SST = SST();
    ans.SSM = ans.SST - ans.SSE;
    ans.df_total = nobs-1;
    ans.df_error = nobs - p;
    ans.df_model = p-1;
    ans.MSE = ans.SSE/ ans.df_error;
    ans.MSM = ans.SSM/ans.df_model;
    ans.F = ans.MSM/ans.MSE;
    ans.p_value = pf(ans.F, ans.df_model, ans.df_error, false, false);
    return ans;
  }

  ostream & AnovaTable::display(ostream &out)const{
    out << "ANOVA Table:" << endl
        << "\tdf\tSum Sq.\t\tMean Sq.\tF:  " << F << endl
        << "Model\t" << df_model << "\t" <<SSM <<"\t\t" <<MSM<<  endl
        << "Error\t" << df_error << "\t"<< SSE <<"\t\t" <<MSE<<"\t p-value: "
        << p_value << endl
        << "Total\t" << df_total <<"\t"<<SST <<endl;
    return out;
  }
  ostream & operator<<(ostream &out, const AnovaTable &tab){
    tab.display(out);
    return out;}
  //======================================================================
  ostream & RegSuf::print(ostream &out)const{
    out << "sample size: " << n() << endl
        << "xty: " << xty() << endl
        << "xtx: " << endl << xtx();
    return out;
  }

  namespace {
    Vector ColSums(const Matrix &m) {
      Vector one(nrow(m), 1.0);
      return one * m;
    }
  }  // namespace
  //======================================================================
  QrRegSuf::QrRegSuf(const Mat&X, const Vector &y):
    qr(X),
    Qty(),
    sumsqy(0.0),
    current(true)
  {
    Matrix Q(qr.getQ());
    Qty = y*Q;
    sumsqy = y.dot(y);
    x_column_sums_ = ColSums(X);
  }

  uint QrRegSuf::size()const{   // dimension of beta
    //    if(!current) refresh_qr();
    return Qty.size();}

  SpdMatrix QrRegSuf::xtx()const{
    //    if(!current) refresh_qr();
    return RTR(qr.getR());}

  Vector QrRegSuf::xty()const{
    //    if(!current) refresh_qr();
    return Qty*qr.getR(); }

  SpdMatrix QrRegSuf::xtx(const Selector &inc)const{
    //    if(!current) refresh_qr();
    return inc.select(xtx());
  }

  Vector QrRegSuf::xty(const Selector &inc)const{
    //    if(!current) refresh_qr();
    return inc.select(xty());
  }

  double QrRegSuf::yty()const{return sumsqy;}
  void QrRegSuf::clear(){
    sumsqy = 0;
    Qty = 0;
    qr = QR(SpdMatrix(Qty.size(), 0.0));
  }

  QrRegSuf * QrRegSuf::clone()const{
    return new QrRegSuf(*this);}

  Vector QrRegSuf::beta_hat()const{
    //if(!current) refresh_qr();
    return qr.Rsolve(Qty); }

  Vector QrRegSuf::beta_hat(const Vector &y)const{
    //    if(!current) refresh_qr();
    return qr.solve(y);}

  void QrRegSuf::Update(const DataType &dp){
    current=false;
    Ptr<DataType> d= dp.clone();
  }  // QR not built for updating

  void QrRegSuf::add_mixture_data(double , const Vector &, double){
    report_error("use NeRegSuf for regression model mixture components.");
  }

  void QrRegSuf::add_mixture_data(double , const ConstVectorView &, double){
    report_error("use NeRegSuf for regression model mixture components.");
  }

  void QrRegSuf::refresh_qr(const std::vector<Ptr<RegressionData> > &raw_data) const {
    if(current) return;
    int n = raw_data.size();  // number of observations
    if(n==0){
      current=false;
      return;}

    Ptr<RegressionData> rdp = DAT(raw_data[0]);
    uint dim_beta = rdp->xdim();
    Matrix X(n, dim_beta);
    Vector y(n);
    sumsqy=0.0;
    for(int i = 0; i<n; ++i){
      rdp = DAT(raw_data[i]);
      y[i] = rdp->y();
      const Vector & x(rdp->x());
      X.set_row(i,x);
//       X(i,0)=1.0;    // this stuff is no longer needed b/c the intercept is explicit
//       int k=0;
//       for(int j=x.lo(); j<=x.hi(); ++j) X(i,++k) = x[j];
      sumsqy += y[i]*y[i];
    }
    qr.decompose(X);
    X = qr.getQ();
    Qty = y*X;
    current=true;
    x_column_sums_ = ColSums(X);
  }

  double QrRegSuf::SSE()const{
    //    if(!current) refresh_qr();
    return sumsqy - Qty.dot(Qty); }

  double QrRegSuf::ybar()const{
    //    if(!current) refresh_qr();
    return qr.getR()(0,0)*Qty[0]/n(); }

  double QrRegSuf::SST()const{
    //    if(!current) refresh_qr();
    return sumsqy - n()*pow(ybar(),2); }

  Vector QrRegSuf::xbar()const {
    return x_column_sums_ / n();
  }

  double QrRegSuf::n()const{
    //    if(!current) refresh_qr();
    return qr.nrow();
  }

  void QrRegSuf::combine(Ptr<RegSuf>){
    report_error("cannot combine QrRegSuf");
  }

  void QrRegSuf::combine(const RegSuf &){
    report_error("cannot combine QrRegSuf");
  }

  QrRegSuf * QrRegSuf::abstract_combine(Sufstat *s){
    return abstract_combine_impl(this,s); }

  Vector QrRegSuf::vectorize(bool)const{
    Vector ans = qr.vectorize();
    ans.reserve(ans.size() + Qty.size() + 2);
    ans.concat(Qty);
    ans.push_back(sumsqy);
    ans.push_back(current);
    return ans;
  }

  Vector::const_iterator QrRegSuf::unvectorize(Vector::const_iterator &v,
                                  bool){
    const double *dp = &(*v);
    const double *original = dp;
    dp = qr.unvectorize(dp);
    v += (dp - original);
    Qty.resize(qr.ncol());
    std::copy(v, v + Qty.size(), Qty.begin());
    v += Qty.size();
    sumsqy = *v; ++v;
    current = lround(*v);
    ++v;
    return v;
  }

  Vector::const_iterator QrRegSuf::unvectorize(const Vector &v, bool minimal){
    Vector::const_iterator it(v.begin());
    return unvectorize(it, minimal);
  }

  ostream & QrRegSuf::print(ostream &out)const{
    return out << "sumsqy = " << yty() << endl
               << "xty_ = " << xty() << endl
               << "xtx  = " << endl << xtx();
  }
  //---------------------------------------------
  NeRegSuf::NeRegSuf(uint p)
  : xtx_(p),
    needs_to_reflect_(false),
    xty_(p),
    xtx_is_fixed_(false),
    sumsqy(0.0),
    n_(0),
    sumy_(0.0),
    x_column_sums_(p)
  { }

  NeRegSuf::NeRegSuf(const Matrix &X, const Vector &y)
      : needs_to_reflect_(false),
        xtx_is_fixed_(false),
        sumsqy(y.normsq()),
        n_(nrow(X)),
        sumy_(y.sum()),
        x_column_sums_(ColSums(X))
  {
    xty_ =y*X;
    xtx_ = X.inner();
    sumsqy = y.dot(y);
  }

  NeRegSuf::NeRegSuf(const SpdMatrix & XTX,
                     const Vector & XTY,
                     double YTY,
                     double n,
                     const Vector &xbar)
    : xtx_(XTX),
      needs_to_reflect_(true),
      xty_(XTY),
      xtx_is_fixed_(false),
      sumsqy(YTY),
      n_(n),
      sumy_(XTY[0]),
      x_column_sums_(xbar * n)
  {}

  NeRegSuf * NeRegSuf::clone()const{
    return new NeRegSuf(*this);}

  void NeRegSuf::add_mixture_data(double y, const Vector &x, double prob){
    this->add_mixture_data(y, ConstVectorView(x), prob);
  }

  void NeRegSuf::add_mixture_data(double y, const ConstVectorView &x, double prob){
    if(!xtx_is_fixed_) {
      xtx_.add_outer(x, prob, false);
      needs_to_reflect_ = true;
    }
    xty_.axpy(x, y * prob);
    sumsqy+= y * y * prob;
    n_ += prob;
    sumy_ += y * prob;
    x_column_sums_.axpy(x, prob);
  }

  void NeRegSuf::clear(){
    if(!xtx_is_fixed_) xtx_=0.0;
    xty_=0.0;
    sumsqy=0.0;
    n_ = 0;
    sumy_ = 0.0;
    x_column_sums_ = 0.0;
  }

  void NeRegSuf::Update(const RegressionData &rdp){
    ++n_;
    int p = rdp.xdim();
    if(xtx_.nrow()==0 || xtx_.ncol()==0)
      xtx_ = SpdMatrix(p,0.0);
    if(xty_.size()==0) xty_ = Vector(p, 0.0);
    const Vector & tmpx(rdp.x());  // add_intercept(rdp.x());
    double y = rdp.y();
    xty_.axpy(tmpx, y);
    if(!xtx_is_fixed_) {
      xtx_.add_outer(tmpx, 1.0, false);
      needs_to_reflect_ = true;
    }
    sumsqy+= y*y;
    sumy_ += y;
    x_column_sums_.axpy(tmpx, 1.0);
  }

  uint NeRegSuf::size()const{ return xtx_.ncol();}  // dim(beta)
  SpdMatrix NeRegSuf::xtx()const{
    reflect();
    return xtx_;
  }
  Vector NeRegSuf::xty()const{ return xty_;}

  SpdMatrix NeRegSuf::xtx(const Selector &inc)const{
    reflect();
    return inc.select(xtx_);
  }
  Vector NeRegSuf::xty(const Selector &inc)const{
    return inc.select(xty_);}
  double NeRegSuf::yty()const{ return sumsqy;}

  Vector NeRegSuf::beta_hat()const{
    reflect();
    return xtx_.solve(xty_);
  }

  double NeRegSuf::SSE()const{
    SpdMatrix ivar = xtx().inv();
    return yty() - ivar.Mdist(xty()); }
  double NeRegSuf::SST()const{ return sumsqy - n()*pow(ybar(),2); }
  double NeRegSuf::n()const{ return n_; }
  Vector NeRegSuf::xbar()const {
    return x_column_sums_ / n();
  }
  double NeRegSuf::ybar()const{ return sumy_/n_;}

  void NeRegSuf::combine(Ptr<RegSuf> sp){
    Ptr<NeRegSuf> s(sp.dcast<NeRegSuf>());
    xtx_ += s->xtx_;   // Do we want to combine xtx_ if xtx_is_fixed_?
    needs_to_reflect_ = needs_to_reflect_ || s->needs_to_reflect_;
    xty_ += s->xty_;
    sumsqy += s->sumsqy;
    sumy_ += s->sumy_;
    n_ += s->n_;
  }

  void NeRegSuf::combine(const RegSuf & sp){
    const NeRegSuf& s(dynamic_cast<const NeRegSuf &>(sp));
    xtx_ += s.xtx_;   // Do we want to combine xtx_ if xtx_is_fixed_?
    needs_to_reflect_ = needs_to_reflect_ || s.needs_to_reflect_;
    xty_ += s.xty_;
    sumsqy += s.sumsqy;
    sumy_ += s.sumy_;
    n_ += s.n_;
  }

  NeRegSuf * NeRegSuf::abstract_combine(Sufstat *s){
    return abstract_combine_impl(this,s); }

  Vector NeRegSuf::vectorize(bool minimal)const{
    reflect();
    Vector ans = xtx_.vectorize(minimal);
    ans.concat(xty_);
    ans.push_back(sumsqy);
    ans.push_back(n_);
    ans.push_back(sumy_);
    return ans;
  }

  Vector::const_iterator NeRegSuf::unvectorize(Vector::const_iterator &v,
                                  bool minimal){
    // do we want to store xtx_is_fixed_?
    xtx_.unvectorize(v, minimal);
    needs_to_reflect_ = true;
    uint dim = xty_.size();
    xty_.assign(v, v+dim);
    v+=dim;
    sumsqy = *v;  ++v;
    n_ = lround(*v); ++v;
    sumy_ = *v; ++v;
    return v;
  }

  Vector::const_iterator NeRegSuf::unvectorize(const Vector &v, bool minimal){
    // do we want to store xtx_is_fixed_?
    Vector::const_iterator it = v.begin();
    return unvectorize(it, minimal);
  }

  ostream & NeRegSuf::print(ostream &out)const{
    reflect();
    return out << "sumsqy = " << sumsqy << endl
               << "sumy_  = " << sumy_ << endl
               << "n_     = " << n_ << endl
               << "xty_ = " << xty_ << endl
               << "xtx  = " << endl << xtx_;
  }

  void NeRegSuf::fix_xtx(bool fix){
    reflect();
    xtx_is_fixed_ = fix;
  }

  void NeRegSuf::reflect()const{
    if(needs_to_reflect_){
      xtx_.reflect();
      needs_to_reflect_ = false;
    }
  }

  //======================================================================
  typedef RegressionDataPolicy RDP;

  RDP::RegressionDataPolicy(Ptr<RegSuf> s)
    : DPBase(s)
  {}
  RDP::RegressionDataPolicy(Ptr<RegSuf>s, const DatasetType &d)
    : DPBase(s,d)
  {}

  RDP::RegressionDataPolicy(const RegressionDataPolicy &rhs)
    : Model(rhs),
      DPBase(rhs)
  {}

  RegressionDataPolicy & RDP::operator=(const RegressionDataPolicy &rhs){
    if(&rhs!=this) DPBase::operator=(rhs);
    return *this;
  }


  //======================================================================
  typedef RegressionModel RM;

  RM::RegressionModel(uint p)
    : GlmModel(),
      ParamPolicy(new GlmCoefs(p), new UnivParams(1.0)),
      DataPolicy(new NeRegSuf(p))
  {}

  RM::RegressionModel(const Vector &b, double Sigma)
    : GlmModel(),
      ParamPolicy(new GlmCoefs(b), new UnivParams(Sigma*Sigma)),
      DataPolicy(new NeRegSuf(b.size()))
  {}

  RM::RegressionModel(Ptr<GlmCoefs> beta, Ptr<UnivParams> sigsq)
    : GlmModel(),
      ParamPolicy(beta, sigsq),
      DataPolicy(new NeRegSuf(beta->nvars_possible()))
  {}

  RM::RegressionModel(const Matrix &X, const Vector &y)
    : GlmModel(),
      ParamPolicy(new GlmCoefs(X.ncol()), new UnivParams(1.0)),
      DataPolicy(new QrRegSuf(X,y))
  {
    mle();
  }

  RM::RegressionModel(const DatasetType &d, bool include_all_variable)
    : GlmModel(),
      ParamPolicy(new GlmCoefs(d[0]->xdim(), include_all_variable),
                  new UnivParams(1.0)),
      DataPolicy(new NeRegSuf(d.begin(), d.end()))
  {}

  RM::RegressionModel(const RegressionModel &rhs)
    : Model(rhs),
      GlmModel(rhs),
      ParamPolicy(rhs),
      DataPolicy(rhs),
      PriorPolicy(rhs),
      NumOptModel(rhs),
      EmMixtureComponent(rhs)
  {}

  RM * RM::clone()const{return new RegressionModel(*this); }

  uint RM::nvars()const{ return coef().nvars(); }
  uint RM::nvars_possible()const{ return coef().nvars_possible(); }

  SpdMatrix RM::xtx(const Selector &inc)const{ return suf()->xtx(inc);}
  Vector RM::xty(const Selector &inc)const{ return suf()->xty(inc);}

  SpdMatrix RM::xtx()const{ return xtx( coef().inc() ) ;}
  Vector RM::xty()const{ return xty( coef().inc() ) ;}
  double RM::yty()const{ return suf()->yty();  }

  Vector RM::simulate_fake_x()const{
    uint p = nvars_possible();
    Vector x(p-1);
    for(uint i=0; i<p-1; ++i) x[i] = rnorm();
    return x;
  }

  RegressionData * RM::simdat()const{
    Vector x = simulate_fake_x();
    double yhat = predict(x);
    double y = rnorm(yhat, sigma());
    return new RegressionData(y,x);
  }

  RegressionData * RM::simdat(const Vector &X)const{
    double yhat = predict(X);
    double y = rnorm(yhat, sigma());
    return new RegressionData(y,X);
  }

  //======================================================================
  GlmCoefs & RM::coef(){return ParamPolicy::prm1_ref();}
  const GlmCoefs & RM::coef()const{return ParamPolicy::prm1_ref();}
  Ptr<GlmCoefs> RM::coef_prm(){return ParamPolicy::prm1();}
  const Ptr<GlmCoefs> RM::coef_prm()const{return ParamPolicy::prm1();}
  void RM::set_sigsq(double s2){ParamPolicy::prm2_ref().set(s2);}

  Ptr<UnivParams> RM::Sigsq_prm(){return ParamPolicy::prm2();}
  const Ptr<UnivParams> RM::Sigsq_prm()const {return ParamPolicy::prm2();}

  double RM::sigsq()const{return ParamPolicy::prm2_ref().value();}
  double RM::sigma()const{return sqrt(sigsq());}

  void RM::make_X_y(Matrix &X, Vector &Y)const{
    uint p = xdim();
    uint n = dat().size();
    X = Matrix(n,p);
    Y = Vector(n);
    for(uint i=0; i<n; ++i){
      Ptr<RegressionData> rdp = dat()[i];
      const Vector &x(rdp->x());
      assert(x.size()==p);
      X.set_row(i,x);
      Y[i] = rdp->y();
    }
  }

  void RM::mle(){
    add_all();
    set_Beta(suf()->beta_hat());
    set_sigsq(suf()->SSE()/suf()->n());
  }

  double RM::pdf(dPtr dp, bool logscale)const{
    Ptr<RegressionData> rd = DAT(dp);
    const Vector &x = rd->x();
    return dnorm(rd->y(), predict(x), sigma(), logscale);
  }

  double RM::pdf(const Data *dp, bool logscale)const{
    const RegressionData *rd =  dynamic_cast<const RegressionData *>(dp);
    return dnorm(rd->y(), predict(rd->x()), sigma(), logscale);
  }

  double RM::Loglike(const Vector &sigsq_beta,
                     Vector &g, Matrix &h, uint nd)const{
    const double log2pi = 1.83787706640935;
    const double sigsq = sigsq_beta[0];
    const Vector b(ConstVectorView(sigsq_beta, 1));
    double n = suf()->n();
    if(b.size()==0) return empty_loglike(g, h, nd);

    double SSE = yty() - 2*b.dot(xty()) + xtx().Mdist(b);
    double ans =  -.5*(n * log2pi  + n *log(sigsq)+ SSE/sigsq);

    if(nd>0){  // sigsq derivs come first in CP2 vectorization
      SpdMatrix xtx = this->xtx();
      Vector gbeta = (xty() - xtx*b)/sigsq;
      double sig4 = sigsq*sigsq;
      double gsigsq = -n/(2*sigsq) + SSE/(2*sig4);
      g = concat(gsigsq, gbeta);
      if(nd>1){
        double h11 = .5*n/sig4 - SSE/(sig4*sigsq);
        h = unpartition(h11, (-1/sigsq)*gbeta, (-1/sigsq)*xtx);}}
    return ans;
  }

  // Log likelihood when beta is empty, so that xbeta = 0.  In this
  // case the only parameter is sigma^2
  double RM::empty_loglike(Vector &g, Matrix &h, uint nd)const{
    double v = sigsq();
    double n = suf()->n();
    double ss = suf()->yty();
    const double log2pi = 1.83787706640935;
    double ans = -.5*n*(log2pi + log(v)) - .5*ss/v;
    if(nd > 0){
      double v2 = v*v;
      g[0] = -.5*n/v + .5*ss/v2;
      if(nd > 1){
        h(0,0) = .5*n/v2 - ss/(v2*v);
      }
    }
    return ans;
  }

  void RM::use_normal_equations(){
    RegSuf *s = suf().get();
    NeRegSuf *rs = dynamic_cast<NeRegSuf *>(s);
    if (rs) return;
    Ptr<NeRegSuf> ne_reg_suf(new NeRegSuf(
        s->xtx(),
        s->xty(),
        s->yty(),
        s->n(),
        s->xbar()));
    reset_suf_ptr(ne_reg_suf);
  }

  void RM::add_mixture_data(Ptr<Data> dp, double prob){
    Ptr<RegressionData> d(DAT(dp));
    suf()->add_mixture_data(d->y(), d->x(), prob);
  }

  /*
     SSE = (y-Xb)^T (y-Xb)
     = (y - QQTy)^T (y - Q Q^Ty)
     = ((I=QQ^T)y))^T(I-QQ^T)y)
     = y^T (1-QQ^T)(I-QQ^T)y
     =  y^T ( I - QQ^T - QQ^T +  QQ^TQQ^T)y
     = y^T(I -QQ^T)y

     b = xtx-1xty = (rt qt q r)^{-1} rt qt y
     = r^[-1] qt y
  */

} // ends namespace BOOM
