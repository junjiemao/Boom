#ifndef BOOM_GLM_POISSON_REGRESSION_SPIKE_SLAB_POSTERIOR_SAMPLER_HPP_
#define BOOM_GLM_POISSON_REGRESSION_SPIKE_SLAB_POSTERIOR_SAMPLER_HPP_

#include <Models/Glm/PoissonRegressionModel.hpp>
#include <Models/Glm/PosteriorSamplers/PoissonRegressionAuxMixSampler.hpp>
#include <Models/Glm/PosteriorSamplers/SpikeSlabSampler.hpp>


namespace BOOM {

  // A spike-and-slab sampler for Poisson regression models.
  class PoissonRegressionSpikeSlabSampler
      : public PoissonRegressionAuxMixSampler
  {
   public:
    // Args:
    //   model:  The model to be posterior sampled.
    //   slab_prior: The prior distribution for the Poisson regression
    //     coefficients, conditional on inclusion.
    //   spike_prior: The prior on which coefficients should be
    //     included.
    //   number_of_threads: The number of threads to use for data
    //     augmentation.
    PoissonRegressionSpikeSlabSampler(
        PoissonRegressionModel *model,
        Ptr<MvnBase> slab_prior,
        Ptr<VariableSelectionPrior> spike_prior,
        int number_of_threads = 1,
        RNG &seeding_rng = GlobalRng::rng);

    void draw() override;
    double logpri() const override;

    // If tf == true then draw_model_indicators is a no-op.  Otherwise
    // model indicators will be sampled each iteration.
    void allow_model_selection(bool tf);

    // In very large problems you may not want to sample every element
    // of the inclusion vector each time.  If max_flips is set to a
    // positive number then at most that many randomly chosen
    // inclusion indicators will be sampled.
    void limit_model_selection(int max_flips);

    // Sets the coefficients in model_ to their posterior mode, and
    // saves the value of the un-normalized log-posterior at the
    // mode.  The optimization is with respect to coefficients that
    // are "in" the model.  Dropped coefficients will remain zero.
    void find_posterior_mode(double epsilon = 1e-5) override;

    bool can_find_posterior_mode() const override {
      return true;
    }

    double log_posterior_at_mode() const {
      return log_posterior_at_mode_;
    }

   private:
    PoissonRegressionModel *model_;
    SpikeSlabSampler sam_;
    Ptr<MvnBase> slab_prior_;
    Ptr<VariableSelectionPrior> spike_prior_;
    double log_posterior_at_mode_;
  };

}  // namespace BOOM

#endif //  BOOM_GLM_POISSON_REGRESSION_SPIKE_SLAB_POSTERIOR_SAMPLER_HPP_
