/*
  Copyright (C) 2008-2011 Steven L. Scott

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

#include <Models/StateSpace/StateModels/SeasonalStateModel.hpp>
#include <distributions.hpp>
#include <cpputil/math_utils.hpp>

namespace BOOM{

  typedef SeasonalStateModel SSM;
  SSM::SeasonalStateModel(int nseasons, int season_duration)
      : ZeroMeanGaussianModel(1.0),  // initial value for sigsq
        nseasons_(nseasons),
        duration_(season_duration),
        time_of_first_observation_(0),
        T0_(new SeasonalStateSpaceMatrix(nseasons)),
        RQR0_(new UpperLeftCornerMatrix(state_dimension(), 1.0)),
        state_error_variance_at_new_season_(
            new UpperLeftCornerMatrix(1, 1.0)),
        T1_(new IdentityMatrix(state_dimension())),
        RQR1_(new ZeroMatrix(state_dimension())),
        state_error_variance_in_season_interior_(new ZeroMatrix(1)),
        state_error_expander_(new FirstElementSingleColumnMatrix(
            state_dimension())),
        initial_state_mean_(state_dimension(), 0.0),
        initial_state_variance_(0)
  {
    if(nseasons_ <= 0 || duration_ <= 0){
      ostringstream err;
      err << "'nseasons' and 'season_duration' must both be positive in "
          << "constructor for SeasonalStateModel" << endl
          << "nseasons = " << nseasons_ << endl
          << "season_duration = " << duration_ << endl
          ;
      report_error(err.str());
    }
    this->only_keep_sufstats(true);
  }

  SSM::SeasonalStateModel(const SeasonalStateModel &rhs)
      : ZeroMeanGaussianModel(rhs),
        nseasons_(rhs.nseasons_),
        duration_(rhs.duration_),
        time_of_first_observation_(rhs.time_of_first_observation_),
        T0_(rhs.T0_),
        RQR0_(rhs.RQR0_->clone()),
        state_error_variance_at_new_season_(
            rhs.state_error_variance_at_new_season_->clone()),
        T1_(rhs.T1_),
        RQR1_(rhs.RQR1_),
        state_error_variance_in_season_interior_(
            rhs.state_error_variance_in_season_interior_->clone()),
        state_error_expander_(
            new FirstElementSingleColumnMatrix(state_dimension())),
        initial_state_mean_(rhs.initial_state_mean_),
        initial_state_variance_(rhs.initial_state_variance_)
  {
    this->only_keep_sufstats(true);
  }

  SSM * SSM::clone()const{return new SSM(*this);}

  bool SSM::new_season(int t)const{
    t -= time_of_first_observation_;
    if (t < 0) {
      // If t is negative then move an integer number of full seasons
      // into the future.  Because t is negative, _subtracting_ t *
      // duration_ will move us |t| cycles into the future.
      t -= duration_ * t;
    }
    return ((t % duration_) == 0);
  }

  void SSM::observe_state(const ConstVectorView then,
                          const ConstVectorView now,
                          int t){
    if(new_season(t)){
      if(then.size() != now.size() || then.size() != state_dimension()){
        report_error("wrong size vector given to "
                     "SeasonalStateModel::observe_state");
      }
      double mu = -1 * (then.sum());
      double delta = now[0] - mu;
      suf()->update_raw(delta);
    }
  }

  Ptr<SparseMatrixBlock> SSM::state_transition_matrix(int t)const{
    if(new_season(t+1)) return T0_;
    return T1_;
  }

  Ptr<SparseMatrixBlock> SSM::state_variance_matrix(int t)const{
    if(new_season(t+1)) return RQR0_;
    return RQR1_;
  }

  Ptr<SparseMatrixBlock> SSM::state_error_expander(int t) const {
    return state_error_expander_;
  }

  Ptr<SparseMatrixBlock> SSM::state_error_variance(int t) const {
    if (new_season(t + 1)) {
      return state_error_variance_at_new_season_;
    } else {
      return state_error_variance_in_season_interior_;
    }
  }

  uint SSM::state_dimension()const{
    return nseasons_ - 1;
  }

  void SSM::simulate_state_error(VectorView state_error, int t)const{
    if(initial_state_mean_.size() != state_dimension()
       || initial_state_variance_.nrow() != state_dimension()){
      ostringstream err;
      err << "initial state mean and/or variance not properly set in "
          << "seasonal_state_model" << endl
          << "required dimension: " << state_dimension() << endl
          << "length(mean)      : " << length(initial_state_mean_) << endl
          << "nrow(variance)    : " << nrow(initial_state_variance_) << endl
          ;
      report_error(err.str());
    }
    state_error = 0;
    assert(state_error.size() == state_dimension());
    if(new_season(t+1)){
      // If next time period is the start of a new season, then an
      // update is needed.  Otherwise, the state error is zero.
      state_error[0] = rnorm(0, sigma());
    }
  }

  SparseVector SSM::observation_matrix(int)const{
    SparseVector ans(state_dimension());
    ans[0] = 1;
    return ans;
  }

  void SSM::set_sigsq(double sigsq){
    ZeroMeanGaussianModel::set_sigsq(sigsq);
    RQR0_->set_value(sigsq);
    state_error_variance_at_new_season_->set_value(sigsq);
  }

  void SSM::set_time_of_first_observation(int t){
    time_of_first_observation_ = t;
  }

  Vector SSM::initial_state_mean()const{
    return initial_state_mean_;}

  SpdMatrix SSM::initial_state_variance()const{
    if(initial_state_variance_.nrow() != state_dimension()){
      ostringstream err;
      err << "The initial state variance has the wrong size in "
          << "SeasonalStateModel.  " << endl
          << "It must be set manually, and it must be of dimension "
          << "number_of_seasons - 1.";
      report_error(err.str());
    }
    return initial_state_variance_;}

  void SSM::set_initial_state_mean(const Vector &mu){
    if(mu.size() != state_dimension()){
      ostringstream err;
      err << "wrong size arugment passed to "
          << "SeasonalStateModel::set_initial_state_mean"
          << endl
          << "state dimension is " << state_dimension()
          << endl
          << "argument dimension is " << mu.size()
          << endl;
      report_error(err.str());
    }
    initial_state_mean_ = mu;}

  void SSM::set_initial_state_variance(const SpdMatrix &v){
    if(ncol(v) != state_dimension()){
      ostringstream err;
      err << "wrong size arugment passed to "
          << "SeasonalStateModel::set_initial_state_variance"
          << endl
          << "state dimension is " << state_dimension()
          << endl
          << "argument dimension is " << ncol(v)
          << endl;
      report_error(err.str());
    }
    initial_state_variance_ = v;}

  void SSM::set_initial_state_variance(double sigsq){
    SpdMatrix v(state_dimension(), sigsq);
    initial_state_variance_ = v;}

  void SSM::update_complete_data_sufficient_statistics(
      int t,
      const ConstVectorView &state_error_mean,
      const ConstSubMatrix &state_error_variance) {
    if (state_error_mean.size() != 1
        || state_error_variance.nrow() != 1
        || state_error_variance.ncol() != 1) {
      report_error("Wrong size argument passed to SeasonalStateModel::"
                   "update_complete_data_sufficient_statistics");
    }
    if (new_season(t)) {
      double mean = state_error_mean[0];
      double var = state_error_variance(0, 0);
      suf()->update_expected_value(1.0, mean, var + square(mean));
    }
  }


}
