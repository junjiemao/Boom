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

#ifndef BOOM_SEASONALSTATE_MODEL_HPP
#define BOOM_SEASONALSTATE_MODEL_HPP
#include <Models/StateSpace/StateModels/StateModel.hpp>
#include <Models/ZeroMeanGaussianModel.hpp>
#include <Models/StateSpace/Filters/SparseVector.hpp>
#include <Models/StateSpace/Filters/SparseMatrix.hpp>

namespace BOOM{

  // StateModel for describing evolving seasonal effects.
  class SeasonalStateModel
      : public ZeroMeanGaussianModel,
        public StateModel
  {
   public:
    // Primary constructor.
    // Args:
    //   sigsq: variance of the error term at the start of a new season
    //   nseasons: number of seasons in the model, e.g. 52 for a
    //     week-of-year effect, or 7 for a day-of-week effect.
    //   season_duration: length of each season.  For example, with
    //     daily data the week-of-year effect would have
    //     season_duration = 7.  A different class will be needed to
    //     have a month-effect because months have different
    //     durations.
    SeasonalStateModel(int nseasons,
                       int season_duration = 1);
    SeasonalStateModel(const SeasonalStateModel &rhs);
    SeasonalStateModel * clone() const override;

    void observe_state(const ConstVectorView then,
                       const ConstVectorView now,
                       int t) override;
    uint state_dimension() const override;
    uint state_error_dimension() const override {return 1;}
    void simulate_state_error(VectorView eta, int t) const override;

    Ptr<SparseMatrixBlock> state_transition_matrix(int t) const override;
    Ptr<SparseMatrixBlock> state_variance_matrix(int t) const override;
    Ptr<SparseMatrixBlock> state_error_expander(int t) const override;
    Ptr<SparseMatrixBlock> state_error_variance(int t) const override;
    SparseVector observation_matrix(int t) const override;

    void set_sigsq(double sigsq) override; // also resets model matrices

    // If the time series does not start at t0 then you establish the
    // time of the first observation with this function.
    void set_time_of_first_observation(int t0);

    Vector initial_state_mean() const override;
    void set_initial_state_mean(const Vector &mu);
    SpdMatrix initial_state_variance() const override;
    void set_initial_state_variance(const SpdMatrix &Sigma);

    // Sets all diagonal elements of Sigma to sigsq and all
    // off-diagaonal elements to zero.
    void set_initial_state_variance(double sigsq);

    // returns true if t is the start of a new season.
    bool new_season(int t)const;

    void update_complete_data_sufficient_statistics(
        int t,
        const ConstVectorView &state_error_mean,
        const ConstSubMatrix &state_error_variance) override;

   private:
    uint nseasons_;
    uint duration_;
    int time_of_first_observation_;

    // Model matrices at the start of a new season
    Ptr<SeasonalStateSpaceMatrix> T0_;

    Ptr<UpperLeftCornerMatrix> RQR0_;  // sigsq() is in the upper left
                                      // corner.  other elements are
                                      // zero.
    Ptr<UpperLeftCornerMatrix> state_error_variance_at_new_season_;


    // Model matrices in the interior of a season, when nothing changes
    Ptr<IdentityMatrix> T1_;    //
    Ptr<ZeroMatrix> RQR1_;      // dimension = state dimension
    Ptr<ZeroMatrix> state_error_variance_in_season_interior_;

    Ptr<FirstElementSingleColumnMatrix> state_error_expander_;

    Vector initial_state_mean_;
    SpdMatrix initial_state_variance_;
    // state is (s[t], s[t-1], ... s[t-nseasons_])  ...
    // contribution to y[t] is s[t] (i.e. Z = (1,0,0,0,...)  )
  };

}

#endif // BOOM_SEASONALSTATE_MODEL_HPP
