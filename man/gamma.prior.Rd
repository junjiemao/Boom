% Copyright 2011 Google Inc. All Rights Reserved.
% Author: stevescott@google.com (Steve Scott)

\name{gamma.prior}
\title{Gamma prior distribution}
\alias{GammaPrior}

\description{Specifies gamma prior distribution.}

\usage{
  GammaPrior(a = NULL, b = NULL, prior.mean = NULL, initial.value = NULL)
}

\arguments{
  \item{a}{The shape parameter in the Gamma(a, b) distribution.}
  \item{b}{The scale paramter in the Gamma(a, b) distribution.}
  \item{prior.mean}{The mean the Gamma(a, b) distribution, which is a/b.}
  \item{initial.value}{The initial value in the MCMC algorithm of the
    variable being modeled.}
}

\details{
  The mean of the Gamma(a, b) distribution is a/b and the variance is
  a/b^2.  If \code{prior.mean} is not \code{NULL}, then one of either
  \code{a} or \code{b} must be non-\code{NULL} as well.

  GammaPrior is the conjugate prior for a Poisson mean or an exponential
  rate.  For a Poisson mean \code{a} corresponds to a prior sum of
  observations and \code{b} to a prior number of observations.  For an
  exponential rate the roles are reversed \code{a} represents a number
  of observations and \code{b} the sum of the observed durations.  The
  gamma distribution is a generally useful for parameters that must be
  positive.

  The gamma distribution is the conjugate prior for the reciprocal of a
  Guassian variance, but \code{\link{SdPrior}} should usually be used in
  that case.  }

\references{
  Gelman, Carlin, Stern, Rubin (2003), "Bayesian Data Analysis", Chapman
  and Hall.
}

\author{
  Steven L. Scott \email{stevescott@google.com}
}
