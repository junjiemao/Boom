/*
  Copyright (C) 2005-2011 Steven L. Scott

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

#include <Models/StateSpace/StateModels/LocalLinearTrendMeanRevertingSlope.hpp>
#include <cpputil/report_error.hpp>
#include <distributions.hpp>

namespace BOOM{
  typedef LocalLinearTrendMeanRevertingSlopeMatrix LMAT;
  typedef LocalLinearTrendMeanRevertingSlopeStateModel LMSM;

  LMAT::LocalLinearTrendMeanRevertingSlopeMatrix(Ptr<UnivParams> phi)
      : phi_(phi)
  {}

  LMAT::LocalLinearTrendMeanRevertingSlopeMatrix(const LMAT &rhs)
      : SparseMatrixBlock(rhs),
        phi_(rhs.phi_)
  {}

  LMAT * LMAT::clone() const {return new LMAT(*this);}

  void LMAT::multiply(VectorView lhs, const ConstVectorView &rhs) const {
    if(lhs.size()!=3){
      report_error("lhs is the wrong size in LMAT::multiply");
    }
    if(rhs.size()!=3){
      report_error("rhs is the wrong size in LMAT::multiply");
    }
    double phi = phi_->value();
    lhs[0] = rhs[0] + rhs[1];
    lhs[1] = phi * rhs[1] + (1-phi) * rhs[2];
    lhs[2] = rhs[2];
  }

  void LMAT::Tmult(VectorView lhs, const ConstVectorView &rhs) const {
    if(lhs.size()!=3){
      report_error("lhs is the wrong size in LMAT::Tmult");
    }
    if(rhs.size()!=3){
      report_error("rhs is the wrong size in LMAT::Tmult");
    }
    lhs[0] = rhs[0];
    double phi = phi_->value();
    lhs[1] = rhs[0] + phi * rhs[1];
    lhs[2] = (1-phi) * rhs[1] + rhs[2];
  }

  void LMAT::multiply_inplace(VectorView x) const {
    x[0] += x[1];
    double phi = phi_->value();
    x[1] = phi*x[1] + (1-phi)*x[2];
  }

  void LMAT::add_to(SubMatrix block) const {
    if(block.nrow() != 3 || block.ncol() !=3){
      report_error("block is the wrong size in LMAT::add_to");
    }
    double phi = phi_->value();
    block(0,0) += 1;
    block(0,1) += 1;
    block(1,1) += phi;
    block(1,2) += 1-phi;
    block(2,2) += 1;
  }

  Matrix LMAT::dense()const{
    Matrix ans(3,3, 0.0);
    ans(0, 0) = 1.0;
    ans(0, 1) = 1.0;
    double phi = phi_->value();
    ans(1,1) = phi;
    ans(1,2) = 1-phi;
    ans(2,2) = 1.0;
    return ans;
  }

  LMSM::LocalLinearTrendMeanRevertingSlopeStateModel(
      Ptr<ZeroMeanGaussianModel> level,
      Ptr<NonzeroMeanAr1Model> slope)
      : level_(level),
        slope_(slope),
        observation_matrix_(3),
        state_transition_matrix_(new LMAT(slope_->Phi_prm())),
        state_variance_matrix_(new UpperLeftDiagonalMatrix(
            get_variances(), 3)),
        state_error_expander_(new ZeroPaddedIdentityMatrix(
            3, 2)),
        state_error_variance_(new UpperLeftDiagonalMatrix(
            get_variances(), 2)),
        initial_level_mean_(0.0),
        initial_slope_mean_(0.0),
        initial_state_variance_(3, 1.0)
  {
    observation_matrix_[0] = 1;
    ParamPolicy::add_model(level_);
    ParamPolicy::add_model(slope_);

    // The mean of the slope is a known model parameter.
    initial_state_variance_(2,2) = 0;
  }

  LMSM::LocalLinearTrendMeanRevertingSlopeStateModel(const LMSM &rhs)
      : Model(rhs),
        StateModel(rhs),
        ParamPolicy(rhs),
        DataPolicy(rhs),
        PriorPolicy(rhs),
        level_(rhs.level_->clone()),
        slope_(rhs.slope_->clone()),
        observation_matrix_(rhs.observation_matrix_),
        state_transition_matrix_(new LMAT(slope_->Phi_prm())),
        state_variance_matrix_(new UpperLeftDiagonalMatrix(
            get_variances(), 3)),
        initial_level_mean_(rhs.initial_level_mean_),
        initial_slope_mean_(rhs.initial_slope_mean_),
        initial_state_variance_(rhs.initial_state_variance_)
  {
    ParamPolicy::add_model(level_);
    ParamPolicy::add_model(slope_);
  }

  LMSM * LMSM::clone() const {return new LMSM(*this);}

  void LMSM::clear_data(){
    level_->clear_data();
    slope_->clear_data();
  }


  // State is (level, slope, slope_mean) The level model expects the
  // error term in level.  The slope model expects the current value
  // of the slope.
  void LMSM::observe_state(const ConstVectorView then,
                           const ConstVectorView now,
                           int time_now){
    double change_in_level = now[0] - then[0] - then[1];
    level_->suf()->update_raw(change_in_level);

    double current_slope = now[1];
    slope_->suf()->update_raw(current_slope);
  }

  void LMSM::observe_initial_state(const ConstVectorView &state){
    slope_->suf()->update_raw(state[1]);
  }

  void LMSM::update_complete_data_sufficient_statistics(
      int t,
      const ConstVectorView &,
      const ConstSubMatrix &) {
    report_error("LocalLinearTrendMeanRevertingSlopeStateModel cannot "
                 "be part of an EM algorithm.");
  }

  void LMSM::simulate_state_error(VectorView eta, int t) const {
    eta[0] = rnorm(0, level_->sigma());
    eta[1] = rnorm(0, slope_->sigma());
    eta[2] = 0;
  }

  Ptr<SparseMatrixBlock> LMSM::state_transition_matrix(int) const {
    return state_transition_matrix_;
  }

  Ptr<SparseMatrixBlock> LMSM::state_variance_matrix(int) const {
    return state_variance_matrix_;
  }

  Ptr<SparseMatrixBlock> LMSM::state_error_expander(int) const {
    return state_error_expander_;
  }

  Ptr<SparseMatrixBlock> LMSM::state_error_variance(int) const {
    return state_error_variance_;
  }

  SparseVector LMSM::observation_matrix(int) const {
    return observation_matrix_;}

  Vector LMSM::initial_state_mean()const{
    Vector ans(3);
    ans[0] = initial_level_mean_;
    ans[1] = initial_slope_mean_;
    ans[2] = slope_->mu();
    return ans;
  }

  SpdMatrix LMSM::initial_state_variance()const{
    return initial_state_variance_;
  }
  void LMSM::set_initial_level_mean(double level_mean){
    initial_level_mean_ = level_mean;}
  void LMSM::set_initial_level_sd(double level_sd){
    initial_state_variance_(0,0) = pow(level_sd, 2);}
  void LMSM::set_initial_slope_mean(double slope_mean){
    initial_slope_mean_ = slope_mean;}
  void LMSM::set_initial_slope_sd(double sd){
    initial_state_variance_(1,1) = pow(sd, 2); }

  void LMSM::check_dim(const ConstVectorView &v) const {
    if(v.size() != 3){
      ostringstream err;
      err << "improper dimesion of ConstVectorView v = :"
          << v << endl
          << "in LocalLinearTrendMeanRevertingSlopeStateModel.  "
          << "Should be of dimension 3"
          << endl;
      report_error(err.str());
    }
  }

  std::vector<Ptr<UnivParams> > LMSM::get_variances() {
    std::vector<Ptr<UnivParams> > ans(2);
    ans[0] = level_->Sigsq_prm();
    ans[1] = slope_->Sigsq_prm();
    return ans;
  }

  void LMSM::simulate_initial_state(VectorView state) const {
    check_dim(state);
    state[0] = rnorm(initial_level_mean_, sqrt(initial_state_variance_(0,0)));
    state[1] = rnorm(initial_slope_mean_, sqrt(initial_state_variance_(1,1)));
    state[2] = slope_->mu();
  }

}
