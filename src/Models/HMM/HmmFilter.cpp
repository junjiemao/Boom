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

#include <Models/HMM/HmmFilter.hpp>
#include <cpputil/math_utils.hpp>
#include <Models/HMM/hmm_tools.hpp>

#include <Models/ModelTypes.hpp>
#include <Models/EmMixtureComponent.hpp>
#include <Models/MarkovModel.hpp>
#include <distributions.hpp>
#include <cpputil/report_error.hpp>

namespace BOOM{

  void hmm_recursion_error(const Matrix &p, const Vector &Marg, const Matrix &Tmat,
                           const Vector &Wsp, uint i, Ptr<Data>);
  void hmm_recursion_error(const Matrix &P, const Vector &marg, const Matrix &tmat,
                           const Vector &wsp, uint i, Ptr<Data> dp){
    string str;
    ostringstream s(str);
    s << "error in HMM recursion at step "<< i<< ":" << endl;
    s << "marg:" << endl << marg << endl;
    s << "P: " << endl << P << endl;
    s << "hmm.cpp:  Q = " << endl << tmat << endl;
    s << "hmm.cpp: p(data|state) = " << wsp << endl;
    s << "here is the observed data that caused the error: " << endl
      << *dp << endl;
    report_error(s.str());
  }

  HmmFilter::HmmFilter(std::vector<Ptr<MixtureComponent> > mv,
                       Ptr<MarkovModel> mark)
      : models_(mv),
      P(0),
      pi(mv.size()),
      logp(mv.size()),
      logpi(mv.size()),
      one(mv.size(), 1.0),
      logQ(mv.size(), mv.size()),
      markov_(mark)
      {}

  uint HmmFilter::state_space_size()const{
    return models_.size();}

  double HmmFilter::initialize(const Data * dp){
    uint S = state_space_size();
    pi = markov_->pi0();
    if(dp->missing()) logp = 0;
    else for(uint s=0; s<S; ++s) logp[s] = models_[s]->pdf(dp, true);
    pi = log(pi) + logp;
    double m = max(pi);
    pi = exp(pi-m);
    double nc = sum(pi);
    double loglike = m + log(nc);
    pi/=nc;
    return loglike;
  }

  double HmmFilter::fwd(const std::vector<Ptr<Data> > &dv){
    logQ = log(markov_->Q() );
    uint n = dv.size();
    uint S = state_space_size();
    if(logp.size()!=S) logp.resize(S);
    if(P.size() < n) P.resize(n);
    double loglike = initialize(dv[0].get());
    for(uint i=1; i<n; ++i){
      if(dv[i]->missing()) logp = 0;
      else for(uint s=0; s<S; ++s) logp[s] = models_[s]->pdf(dv[i].get(), true);
      loglike += fwd_1(pi, P[i], logQ, logp, one);
    }
    return loglike;
  }
  //------------------------------------------------------------

  double HmmFilter::loglike(const std::vector<Ptr<Data> > & dv){
    logQ = log(markov_->Q());
    pi = markov_->pi0();
    uint S = pi.size();
    uint n = dv.size();
    Matrix P(logQ);
    double ans = initialize(dv[0].get());
    for(uint i=1; i<n; ++i){
      if(dv[i]->missing()) logp = 0;
      else for(uint s=0; s<S; ++s) logp[s] = models_[s]->pdf(dv[i].get(), true);
      ans += fwd_1(pi, P, logQ, logp, one);
    }
    return ans;
  }
  //------------------------------------------------------------

  void HmmFilter::bkwd_sampling_mt(const std::vector<Ptr<Data> > &dv,
                                   RNG & eng){
    uint n = dv.size();
    // pi was already set by fwd.
    // So the following line would  break things when n=1.
    //      pi = one * P.back();
    uint s = rmulti_mt(eng,pi);
    models_[s]->add_data(dv.back());
    for(uint i=n-1; i!=0; --i){
      pi = P[i].col(s);
      pi.normalize_prob();
      uint r = rmulti_mt(eng,pi);
      models_[r]->add_data(dv[i-1]);
      markov_->suf()->add_transition(r,s);
      s=r;
    }
    markov_->suf()->add_initial_value(s);
  }

  //------------------------------------------------------------
  void HmmFilter::bkwd_sampling(const std::vector<Ptr<Data> > &dv ){
    uint n = dv.size();
    // pi was already set by fwd, so the following line would breaks
    // things when n=1.
    //      pi = one * P.back();
    uint s = rmulti(pi);                // last obs in state s
    allocate(dv.back(), s);             // last data point allocated

    for(uint i=n-1; i!=0; --i){         // start with s=h[i]
      pi = P[i].col(s);                 // compute r = h[i-1]
      uint r = rmulti(pi);
      allocate(dv[i-1], r);
      markov_->suf()->add_transition(r,s);
      s=r;
    }
    markov_->suf()->add_initial_value(s);
    // in last step of loop i = 1, so s=h[0]
  }
  //----------------------------------------------------------------------
  void HmmFilter::allocate(Ptr<Data> dp, uint h){
    models_[h]->add_data(dp);
  }
  Vector HmmFilter::state_probs(Ptr<Data>)const{
    Vector ans;
    report_error("state_probs() cannot be called with this filter.  "
                 "Use an HmmSavePiFilter instead.");
    return ans;
  }

  //======================================================================
  HmmSavePiFilter::HmmSavePiFilter(std::vector<Ptr<MixtureComponent> > mv,
                                   Ptr<MarkovModel> mark,
                                   std::map<Ptr<Data>, Vector> &pi_hist)
      : HmmFilter(mv, mark),
        pi_hist_(pi_hist)
  {}
  //----------------------------------------------------------------------
  void HmmSavePiFilter::allocate(Ptr<Data> dp, uint h){
    models_[h]->add_data(dp);
    Vector & v(pi_hist_[dp]);
    if(v.size()==0) v.resize(pi.size());
    v += pi;
  }

  Vector HmmSavePiFilter::state_probs(Ptr<Data> dp)const{
   std::map<Ptr<Data>, Vector>::const_iterator it = pi_hist_.find(dp);
    if(it==pi_hist_.end()){
      ostringstream err;
      err << "could not compute state probability distribution "
          << "for data point " << *dp << endl;
      report_error(err.str());
    }
    Vector ans(it->second);
    ans.normalize_prob();
    return ans;
  }
  //======================================================================

  HmmEmFilter::HmmEmFilter(std::vector<Ptr<EmMixtureComponent> > mix,
                           Ptr<MarkovModel> mark)
      : HmmFilter(std::vector<Ptr<MixtureComponent> >(mix.begin(), mix.end()),
                  mark),
      models_(mix)
      {}
  //------------------------------------------------------------
  void HmmEmFilter::bkwd_smoothing(const std::vector<Ptr<Data> > & dv){
    // pi was set by fwd;
    uint n = dv.size();
    uint S = state_space_size();
    for(uint i=n-1; i!=0; --i){
      for(uint s=0; s<S; ++s) models_[s]->add_mixture_data(dv[i], pi[s]);
      markov_->suf()->add_transition_distribution(P[i]);
      bkwd_1(pi, P[i], logp, one);
    }
    pi = P[1] * one;
    for(uint s=0; s<S; ++s) models_[s]->add_mixture_data(dv[0], pi[s]);
    markov_->suf()->add_initial_distribution(pi);
  }

}  // namespace BOOM
