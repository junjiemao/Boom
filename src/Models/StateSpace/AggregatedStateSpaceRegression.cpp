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

#include <Models/StateSpace/AggregatedStateSpaceRegression.hpp>
#include <Models/StateSpace/StateModels/RegressionStateModel.hpp>
#include <LinAlg/VectorView.hpp>
#include <cpputil/math_utils.hpp>
#include <distributions.hpp>

#include <boost/function.hpp>
#include <boost/bind.hpp>

namespace BOOM{
  //======================================================================
  // One 'week' of data, which may or may not contain an observed
  // monthly total.
  FineNowcastingData::FineNowcastingData(
      const Vector &x,
      double coarse_observation,
      bool coarse_observation_observed,
      bool contains_end,
      double fraction_of_value_in_initial_period)
      : x_(new RegressionData(negative_infinity(), x)),
        coarse_observation_(coarse_observation),
        coarse_observation_observed_(coarse_observation_observed),
        contains_end_(contains_end),
        fraction_in_initial_period_(fraction_of_value_in_initial_period)
  {}

  FineNowcastingData::FineNowcastingData(const FineNowcastingData &rhs)
      : Data(rhs),
        x_(rhs.x_->clone()),
        coarse_observation_(rhs.coarse_observation_),
        coarse_observation_observed_(rhs.coarse_observation_observed_),
        contains_end_(rhs.contains_end_),
        fraction_in_initial_period_(rhs.fraction_in_initial_period_)
  {}

  FineNowcastingData * FineNowcastingData::clone()const{
    return new FineNowcastingData(*this);}

  ostream & FineNowcastingData::display(ostream &out)const{
    out << "x = " << x_->x() << endl
        << "   y = " << coarse_observation_ << " ["
        << (coarse_observation_observed_ ?
            string("observed") : string("missing"))
        << "]" << endl
        << "   contains_end = "
        << (contains_end_ ?
            string("contains_end") :
            string("regular"))
        << endl
        << "   fraction in previous period = ("
        << fraction_in_initial_period_ << ")" << endl;
    return out;
  }

  Ptr<RegressionData> FineNowcastingData::regression_data()const{
    return x_; }

  double FineNowcastingData::fraction_in_initial_period()const{
    return fraction_in_initial_period_;}

  bool FineNowcastingData::contains_end()const{
    return contains_end_;}

  bool FineNowcastingData::coarse_observation_observed()const{
    return coarse_observation_observed_;}

  double FineNowcastingData::coarse_observation()const{
    return coarse_observation_;}

  //======================================================================
  AccumulatorTransitionMatrix::AccumulatorTransitionMatrix(
      const SparseKalmanMatrix *T,
      const SparseVector &Z,
      double fraction_in_initial_period,
      bool contains_end,
      bool owns_matrix)
      : transition_matrix_(T),
        observation_vector_(Z),
        fraction_in_initial_period_(fraction_in_initial_period),
        contains_end_(contains_end),
        owns_matrix_(owns_matrix)
  {
    if(fraction_in_initial_period > 1.0
       || fraction_in_initial_period <= 0.0){
      ostringstream err;
      err << "Error in constructor for AccumulatorTransitionMatrix:"
          << endl
          << "fraction_in_initial_period must be in (0, 1]"
          << endl;
      report_error(err.str());
    }
  }

  AccumulatorTransitionMatrix::~AccumulatorTransitionMatrix() {
    if(transition_matrix_ && owns_matrix_){
      delete transition_matrix_;
    }
  }

  void AccumulatorTransitionMatrix::reset(const SparseKalmanMatrix *T,
                                          const SparseVector &Z,
                                          double fraction_in_initial_period,
                                          bool contains_end){
    if(transition_matrix_ && owns_matrix_){
      delete transition_matrix_;
    }
    transition_matrix_ = T;
    observation_vector_ = Z;
    fraction_in_initial_period_ = fraction_in_initial_period;
    contains_end_ = contains_end;
  }

  int AccumulatorTransitionMatrix::nrow()const{
    return transition_matrix_->nrow() + 2;}
  int AccumulatorTransitionMatrix::ncol()const{
    return transition_matrix_->ncol() + 2;}

  //----------------------------------------------------------------------
  template <class VEC>
  void report_multiplication_error(const SparseKalmanMatrix *T,
                                   const SparseVector &Z,
                                   bool new_time,
                                   double fraction_in_initial_period,
                                   const VEC &v){
      ostringstream err;
      int state_dim = T->nrow();
      err << "incompatible sizes in AccumulatorTransitionMatrix multiplication"
          << endl
          << "T.nrow() = " << state_dim << endl
          << "Z.size() = " << Z.size() << endl
          << "v.size() = " << v.size() << endl
          << "The first two should match.  The last should be two more "
          << "than the others" << endl;
      report_error(err.str());
  }
  //----------------------------------------------------------------------
  namespace{
    // Keep in mind that you might not be multiplying a state vector.
    // You probably are, but you might be multiplying a random column in
    // a variance matrix, etc.
    template <class VEC>
        Vector Multiply(const SparseKalmanMatrix *T,
                     const SparseVector &Z,
                     bool contains_end,
                     double fraction_in_initial_period,
                     const VEC &v) {

      int state_dim = T->nrow();
      if(v.size() != state_dim + 2 || Z.size() != state_dim){
        report_multiplication_error(
            T, Z, contains_end, fraction_in_initial_period, v);
      }
      ConstVectorView old_state(v.data(), state_dim, v.stride());
      double old_weekly_observation(v[state_dim]);
      double old_cumulator(v[state_dim+1]);

      Vector ans(v.size());
      VectorView new_state(ans, 0, state_dim);
      double &new_weekly_observation(ans[state_dim]);
      double &new_cumulator(ans[state_dim+1]);

      new_state = (*T) * old_state;
      new_weekly_observation = Z.dot(new_state);
      if(contains_end){
        new_cumulator = (1-fraction_in_initial_period)*old_weekly_observation;
      } else {
        new_cumulator = old_cumulator + old_weekly_observation;
      }
      return ans;
    }
  }
  //----------------------------------------------------------------------
  Vector AccumulatorTransitionMatrix::operator *(const Vector &v)const{
    return Multiply(transition_matrix_,
                    observation_vector_,
                    contains_end_,
                    fraction_in_initial_period_,
                    v);
  }
  //----------------------------------------------------------------------
  Vector AccumulatorTransitionMatrix::operator *(const VectorView &v)const{
    return Multiply(transition_matrix_,
                    observation_vector_,
                    contains_end_,
                    fraction_in_initial_period_,
                    v);
  }
  //----------------------------------------------------------------------
  Vector AccumulatorTransitionMatrix::operator *(const ConstVectorView &v)const{
    return Multiply(transition_matrix_,
                    observation_vector_,
                    contains_end_,
                    fraction_in_initial_period_,
                    v);
  }
  //----------------------------------------------------------------------
  Vector AccumulatorTransitionMatrix::Tmult(const Vector &v)const{
    int state_dim = transition_matrix_->ncol();
    if(v.size() != state_dim + 2){
      report_multiplication_error(
          transition_matrix_, observation_vector_, contains_end_,
          fraction_in_initial_period_, v);
    }

    double w = v[state_dim];
    double W = v[state_dim + 1];
    Vector ans(v.size());

    VectorView state_component(ans, 0, state_dim);
    Vector arg = (observation_vector_.dense() * w) +
        ConstVectorView(v, 0, state_dim);
    state_component = transition_matrix_->Tmult(arg);
    ans[state_dim] = (1 - fraction_in_initial_period_ * contains_end_) * W;
    ans[state_dim+1] = (1 - contains_end_) * W;
    return ans;
  }
  //----------------------------------------------------------------------

  // P is decomposed where dim(a) = 'state_dim' and dim(y) = dim(Y) = 1.
  // | Pa  Pay PaY |
  // | Pya Py  PyY |
  // | PYa PYy PY  |
  void AccumulatorTransitionMatrix::sandwich_inplace(SpdMatrix &P)const{
    int state_dim = transition_matrix_->ncol();
    if(P.ncol() != state_dim+2) report_multiplication_error(
           transition_matrix_, observation_vector_,
           contains_end_, fraction_in_initial_period_,
           P.col(0));

    SubMatrix TPT(P, 0, state_dim-1, 0, state_dim-1);
    transition_matrix_->sandwich_inplace_submatrix(TPT);

    double a = 1 - fraction_in_initial_period_ * contains_end_;
    int b = !contains_end_;

    Vector zTPT = TPT * observation_vector_;
    double zTPTz = observation_vector_.dot(zTPT);

    Vector TPay = (*transition_matrix_) *
        VectorView(P.col(state_dim), 0, state_dim);
    Vector TPaY = (*transition_matrix_) *
        VectorView(P.col(state_dim+1), 0, state_dim);
    double zTPay = observation_vector_.dot(TPay);
    double zTPaY = observation_vector_.dot(TPaY);
    double Py = P(state_dim, state_dim);
    double PY = P(state_dim+1, state_dim+1);
    double PyY = P(state_dim, state_dim+1);

    VectorView(P.col(state_dim), 0, state_dim) = zTPT;
    VectorView(P.row(state_dim), 0, state_dim) = zTPT;
    P(state_dim, state_dim) = zTPTz;

    VectorView tmp(P.col(state_dim+1), 0, state_dim);
    tmp = a*TPay + b*TPaY;
    VectorView(P.row(state_dim+1), 0, state_dim) = tmp;

    P(state_dim+1, state_dim) = a*zTPay + b*zTPaY;
    P(state_dim, state_dim+1) = P(state_dim+1, state_dim);
    P(state_dim+1, state_dim+1) = a*a*Py + b*b*PY + 2*a*b*PyY;
  }
  //----------------------------------------------------------------------
  Matrix & AccumulatorTransitionMatrix::add_to(Matrix &P)const{
    int state_dim = transition_matrix_->nrow();
    if(P.nrow() != state_dim+2 || P.ncol() != state_dim+2){
      report_error("wrong sizes in AccumulatorTransitionMatrix::add_to");
    }
    SubMatrix Pa(P, 0, state_dim-1, 0, state_dim-1);
    transition_matrix_->add_to_submatrix(Pa);
    Vector tmp = transition_matrix_->Tmult(observation_vector_.dense());
    VectorView(P.row(state_dim), 0, state_dim) += tmp;
    double a = 1 - fraction_in_initial_period_ * contains_end_;
    int b = !contains_end_;
    P(state_dim+1, state_dim) += a;
    P(state_dim+1, state_dim+1) += b;
    return P;
  }
  //======================================================================
  AccumulatorStateVarianceMatrix::AccumulatorStateVarianceMatrix(
      const SparseKalmanMatrix *RQR,
      const SparseVector &Z,
      double observation_variance,
      bool owns_matrix)
      : state_variance_matrix_(RQR),
        observation_vector_(Z),
        observation_variance_(observation_variance),
        owns_matrix_(owns_matrix)
  {}

  AccumulatorStateVarianceMatrix::~AccumulatorStateVarianceMatrix(){
    if(state_variance_matrix_ && owns_matrix_){
      delete state_variance_matrix_;
    }
  }

  void AccumulatorStateVarianceMatrix::reset(const SparseKalmanMatrix *RQR,
                                             const SparseVector &Z,
                                             double observation_variance){
    if(state_variance_matrix_ && owns_matrix_){
      delete state_variance_matrix_;
    }
    state_variance_matrix_ = RQR;
    observation_vector_ = Z;
    observation_variance_ = observation_variance;
  }

  int AccumulatorStateVarianceMatrix::nrow()const{
    return state_variance_matrix_->nrow() + 2;}
  int AccumulatorStateVarianceMatrix::ncol()const{
    return state_variance_matrix_->ncol() + 2;}

  template <class VECTOR>
  Vector RQR_Multiply(const VECTOR &v,
                   const SparseKalmanMatrix &RQR,
                   const SparseVector &Z,
                   double H){
    int state_dim = Z.size();
    if(v.size() != state_dim + 2){
      report_error("wrong sizes in RQR_Multiply");
    }
    // Partition v = [eta, epsilon, 0]
    ConstVectorView eta(v, 0, state_dim);
    double epsilon = v[state_dim];

    // Partition this
    Vector RQRZ = RQR * Z.dense();
    double ZRQRZ_plus_H = Z.dot(RQRZ) + H;

    Vector ans(v.size());
    VectorView(ans, 0, state_dim) = (RQR * eta).axpy(RQRZ, epsilon);
    ans[state_dim] = RQRZ.dot(eta) + ZRQRZ_plus_H * epsilon;
    return ans;
  }

  Vector AccumulatorStateVarianceMatrix::operator*(const Vector &v)const{
    return RQR_Multiply(v,
                        *state_variance_matrix_,
                        observation_vector_,
                        observation_variance_);
  }
  Vector AccumulatorStateVarianceMatrix::operator*(const VectorView &v)const{
    return RQR_Multiply(v,
                        *state_variance_matrix_,
                        observation_vector_,
                        observation_variance_);
  }
  Vector AccumulatorStateVarianceMatrix::operator*(
      const ConstVectorView &v)const{
    return RQR_Multiply(v,
                        *state_variance_matrix_,
                        observation_vector_,
                        observation_variance_);
  }

  Vector AccumulatorStateVarianceMatrix::Tmult(const Vector &v)const{
    return RQR_Multiply(v,
                        *state_variance_matrix_,
                        observation_vector_,
                        observation_variance_);
  }

  Matrix & AccumulatorStateVarianceMatrix::add_to(Matrix &m)const{
    int state_dim(state_variance_matrix_->nrow());
    if(m.nrow()!= state_dim+2){
      report_error("wrong sizes in AccumulatorStateVarianceMatrix::add_to");
    }

    SubMatrix RQR(m, 0, state_dim, 0, state_dim);
    state_variance_matrix_->add_to_submatrix(RQR);

    Vector ZRQR = (*state_variance_matrix_) * observation_vector_.dense();
    VectorView(m.col(state_dim), 0, state_dim) += ZRQR;
    VectorView(m.row(state_dim), 0, state_dim) += ZRQR;
    m(state_dim, state_dim) +=
        observation_vector_.dot(ZRQR) + observation_variance_;
    return m;
  }
  //======================================================================
  typedef AggregatedRegressionStateModel ARSM;
  ARSM::AggregatedRegressionStateModel(Ptr<RegressionModel> m)
      : RegressionStateModel(m),
        final_x_(m->xdim())
  {}

  void ARSM::set_final_x(const Vector &x){
    final_x_ = x;
  }

  SparseVector ARSM::observation_matrix(int t)const {
    int n = regression()->dat().size();
    if(t < n) return RegressionStateModel::observation_matrix(t);
    if(t > n) {
      report_error("argument too large in "
                   "AggregatedRegressionStateModel::observation_matrix");
    }
    // Handle the t == n case, which will occur on the final step of
    // the Kalman filter.
    double eta = regression()->predict(final_x_);
    SparseVector ans(1);
    ans[0] = eta;
    return ans;
  }

  //======================================================================
  typedef AggregatedStateSpaceRegression ASSR;

  ASSR::AggregatedStateSpaceRegression(int predictor_dimension)
      : regression_(new RegressionModel(predictor_dimension)),
        observation_model_(new GaussianModel(0, 0)),
        regression_state_(new AggregatedRegressionStateModel(regression_))
   {
     add_state(regression_state_);
   }

  ASSR::AggregatedStateSpaceRegression(const ASSR &rhs)
      : Model(rhs),
        StateSpaceModelBase(),
        DataPolicy(rhs),
        PriorPolicy(rhs),
        regression_(rhs.regression_->clone()),
        observation_model_(rhs.observation_model_->clone()),
        regression_state_(new AggregatedRegressionStateModel(regression_))
  {
    add_state(regression_state_);
    for(int s = 1; s < rhs.nstate(); ++s){
      add_state(rhs.state_model(s)->clone());
    }
    clear_data();
    regression_->clear_data();
    const std::vector<Ptr<FineNowcastingData> > & data(rhs.dat());
    for(int i = 0; i < data.size(); ++i) add_data(data[i]);
  }

  AggregatedStateSpaceRegression * ASSR::clone()const{
    return new AggregatedStateSpaceRegression(*this);}

  void ASSR::add_data(Ptr<Data> dp){add_data(DAT(dp));}

  void ASSR::add_data(Ptr<FineNowcastingData> dp){
    DataPolicy::add_data(dp);
    Ptr<RegressionData> rdp(dp->regression_data());
    regression_->add_data(rdp);  // full of missing values
  }

  Ptr<FineNowcastingData> ASSR::fine_data(int t){ return dat()[t]; }
  const Ptr<FineNowcastingData> ASSR::fine_data(int t)const{ return dat()[t]; }

  int ASSR::time_dimension()const{return dat().size();}

  int ASSR::state_dimension()const{
    return 2 + StateSpaceModelBase::state_dimension();}

  double ASSR::observation_variance(int t)const{return 0;}

  double ASSR::adjusted_observation(int t)const{
    return fine_data(t)->coarse_observation(); }
  bool ASSR::is_missing_observation(int t)const{
    return !(fine_data(t)->coarse_observation_observed());}

  // Update the sufficient statistics for the regression component of
  // the model.
  void ASSR::observe_data_given_state(int t) {
    const ConstVectorView alpha(state(t));
    int state_dim = state_dimension();
    const ConstVectorView client_state_error(alpha, 0, state_dim - 2);
    double y = alpha[state_dim - 2];
    // y is the imputed fine-scale observation from the Kalman filter.

    Ptr<RegressionData> dp(regression_->dat()[t]);
    // The state_mean is computed using the observation_matrix from
    // the client model, available from StateSpaceModelBase.
    double state_mean =
        StateSpaceModelBase::observation_matrix(t).dot(client_state_error);

    // We want y with time series effects subtracted off.  We get this
    // by computing state_mean (which contains the full prediction of
    // y given all state, including the regressors), computing the
    // residual from state_mean, and adding in the regression effect
    // back in.
    double residual = y - state_mean;
    double predicted = regression_->predict(dp->x());
    regression_->suf()->add_mixture_data(residual + predicted, dp->x(), 1.0);
  }

  // TODO(stevescott): This and other code involving model matrices is
  // an optimization opportunity.  Test it out to see if
  // precomputation makes sense.
  const AccumulatorTransitionMatrix * ASSR::state_transition_matrix(int t)const{
    Ptr<FineNowcastingData> fine_data(this->fine_data(t));
    if(!transition_matrix_) {
      transition_matrix_.reset(new AccumulatorTransitionMatrix(
          StateSpaceModelBase::state_transition_matrix(t),
          StateSpaceModelBase::observation_matrix(t+1),
          fine_data->fraction_in_initial_period(),
          fine_data->contains_end()));
    } else {
      transition_matrix_->reset(
          StateSpaceModelBase::state_transition_matrix(t),
          StateSpaceModelBase::observation_matrix(t+1),
          fine_data->fraction_in_initial_period(),
          fine_data->contains_end());
    }
    return transition_matrix_.get();
  }

  // TODO(stevescott):  check this
  SparseVector ASSR::observation_matrix(int t)const{
    Ptr<FineNowcastingData> fine_data(this->fine_data(t));
    int p = state_dimension();
    SparseVector ans(p);
    ans[p-1] = 1;
    ans[p-2] = fine_data->fraction_in_initial_period();
    return ans;
  }

  const AccumulatorStateVarianceMatrix *
  ASSR::state_variance_matrix(int t)const{
    if(!variance_matrix_){
      variance_matrix_.reset(new AccumulatorStateVarianceMatrix(
          StateSpaceModelBase::state_variance_matrix(t),
          StateSpaceModelBase::observation_matrix(t+1),
          regression_->sigsq()));
    } else {
      variance_matrix_->reset(
          StateSpaceModelBase::state_variance_matrix(t),
          StateSpaceModelBase::observation_matrix(t+1),
          regression_->sigsq());
    }
    return variance_matrix_.get();
  }

  // TODO(stevescott):  test
  void ASSR::simulate_initial_state(VectorView state0)const{
    // First, simulate the initial state of the client state vector.
    VectorView client_state(state0, 0, state0.size()-2);
    StateSpaceModelBase::simulate_initial_state(client_state);

    // Next simulate the initial value of the first latent weekly
    // observation.
    double mu = StateSpaceModelBase::observation_matrix(0).dot(client_state);
    state0[state_dimension() - 2] = rnorm(mu, regression_->sigma());

    // Finally, the initial state of the cumulator variable is zero.
    state0[state_dimension() - 1] = 0;
  }

  Vector ASSR::simulate_initial_state()const{
    Vector ans(state_dimension());
    simulate_initial_state(VectorView(ans));
    return ans;
  }

  Vector ASSR::simulate_state_error(int t)const{
    int state_dim = state_dimension();
    Vector ans(state_dim, 0);
    VectorView client_state_error(ans, 0, state_dim - 2);
    client_state_error = StateSpaceModelBase::simulate_state_error(t);

    // TODO(stevescott):  check this
    ans[state_dim - 2] =
        StateSpaceModelBase::observation_matrix(t).dot(client_state_error)
        + rnorm(0, regression_->sigma());
    ans.back() = 0;

    return ans;
  }

  Vector ASSR::initial_state_mean()const{
    Vector ans = StateSpaceModelBase::initial_state_mean();
    double y0 = StateSpaceModelBase::observation_matrix(0).dot(ans);
    ans.push_back(y0);
    ans.push_back(0.0);
    return ans;
  }

  // | V0   Z^T*V0   0 |
  // | V0*Z Z^T*V0*Z 0 |
  // | 0    0        0 |
  SpdMatrix ASSR::initial_state_variance()const{
    SpdMatrix V0 = StateSpaceModelBase::initial_state_variance();
    SparseVector Z0(StateSpaceModelBase::observation_matrix(0));
    Vector covariance = V0 * Z0;
    double y_variance = Z0.dot(covariance) + regression_->sigsq();

    int state_dim = state_dimension();
    SpdMatrix ans(state_dim, 0.0);
    SubMatrix upper_left(ans, 0, state_dim - 3, 0, state_dim - 3);
    upper_left = V0;
    ans.col(state_dim - 2);
    VectorView covariance_column(ans.col(state_dim - 2), 0, state_dim - 2 );
    VectorView covariance_row(ans.row(state_dim - 2), 0, state_dim - 2 );
    covariance_column = covariance;
    covariance_row = covariance;
    ans(state_dim - 2, state_dim -2) = y_variance;
    return ans;
  }

}
