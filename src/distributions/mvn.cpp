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

#include <cmath>

#include <distributions.hpp>


#include <LinAlg/Vector.hpp>
#include <LinAlg/Matrix.hpp>
#include <LinAlg/SpdMatrix.hpp>
#include <LinAlg/Cholesky.hpp>
#include <algorithm>

namespace BOOM{

  Vector rmvn_robust(const Vector &mu, const SpdMatrix &V){
    return rmvn_robust_mt(GlobalRng::rng, mu, V); }
  Vector rmvn_robust_mt(RNG &rng, const Vector &mu, const SpdMatrix &V){
    uint n = V.nrow();
    Matrix Q(n,n);
    Vector lam = eigen(V,Q);
    for(uint i=0; i<n; ++i){
      // We're guaranteed that lam[i] is real and non-negative.  We
      // can take the absolute value of lam[i] to guard against
      // spurious negative numbers close to zero.
      lam[i]  = sqrt(fabs(lam[i])) * rnorm_mt(rng, 0,1);
    }
    Vector ans(Q*lam);
    ans += mu;
    return ans;
  }

  Vector rmvn_L(const Vector &mu, const Matrix &L){
    return rmvn_L_mt(GlobalRng::rng, mu, L);}

  Vector rmvn_L_mt(RNG & rng, const Vector &mu, const Matrix &L){
    // L is the lower cholesky triange of Sigma
    uint n = mu.size();
    Vector wsp(n);
    for(uint i = 0; i<n; ++i) wsp[i] = rnorm_mt(rng, 0,1);
    return Lmult(L, wsp) + mu;
  }
  //======================================================================
  Vector rmvn(const Vector &mu, const SpdMatrix &V){
    return rmvn_mt(GlobalRng::rng, mu, V); }

  Vector rmvn_mt(RNG & rng, const Vector &mu, const SpdMatrix &V){
    bool okay=true;
    Matrix L = V.chol(okay);
    if(okay) return rmvn_L_mt(rng, mu, L);
    return rmvn_robust_mt(rng, mu, V);
  }
  //======================================================================
  Vector rmvn_ivar(const Vector &mu, const SpdMatrix &ivar){
    return rmvn_ivar_mt(GlobalRng::rng, mu, ivar);  }

  Vector rmvn_ivar_mt(RNG & rng, const Vector &mu, const SpdMatrix &ivar){
    // draws a multivariate normal with mean mu and inverse variance
    // Matrix ivar
    Matrix U= ivar.chol().t();  /////// experimental
    return rmvn_ivar_U_mt(rng, mu, U);
  }

  Vector rmvn_ivar_U(const Vector &mu, const Matrix &U){
    return rmvn_ivar_U_mt(GlobalRng::rng, mu, U); }

  Vector rmvn_ivar_U_mt(RNG & rng, const Vector &mu, const Matrix &U){
    // U is the upper cholesky factor of the inverse variance Matrix
    uint n = mu.size();
    Vector z(n);
    for(uint i =0; i<n; ++i) z[i] = rnorm_mt(rng, 0,1);
    //    if ivar = L L^T then Sigma = (L^T)^{-1} L^{-1} = U U^T
    return Usolve_inplace(U,z) + mu;
  }

  Vector rmvn_ivar_L(const Vector &mu, const Matrix &L){
    return rmvn_ivar_L_mt(GlobalRng::rng, mu, L);  }
  Vector rmvn_ivar_L_mt(RNG & rng, const Vector &mu, const Matrix &L){
    // L is the lower cholesky triangle  of the inverse variance Matrix
    return rmvn_ivar_U_mt(rng, mu, L.t());  }


  Vector rmvn_suf(const SpdMatrix & Ivar, const Vector & IvarMu){
    return rmvn_suf_mt(GlobalRng::rng, Ivar, IvarMu);  }

  Vector rmvn_suf_mt(RNG & rng, const SpdMatrix & Ivar, const Vector & IvarMu){
    Chol L(Ivar);
    uint n = IvarMu.size();
    Vector z(n);
    for(uint i=0; i<n; ++i) z[i] = rnorm_mt(rng);
    LTsolve_inplace(L.getL(), z);  // returns LT^-1 z which is ~ N(0, Ivar.inv)
    z+= L.solve(IvarMu);
    return z;
  }

  //======================================================================
  double dmvn(const Vector &y, const Vector &mu,
              const SpdMatrix &Siginv, double ldsi, bool logscale){

    /*---------------------------------------------------------------
      Evaluates the multivariate normal distribution with mean mu and
      inverse variance Matrix Siginv.  ldsi is the log determinant of
      Siginv.
      ---------------------------------------------------------------*/
    const double log2pi = 1.83787706641;
    double n = y.size();
    double ans = 0.5*(ldsi - Siginv.Mdist(y, mu) -n*log2pi);
    return logscale ? ans : std::exp(ans);
  }

  double dmvn_zero_mean(const Vector &y, const SpdMatrix &Siginv,
                        double ldsi, bool logscale){
    const double log2pi = 1.83787706641;
    double n = y.size();
    double ans = 0.5*(ldsi - Siginv.Mdist(y) -n*log2pi);
    return logscale ? ans : std::exp(ans);
  }


  double dmvn(const Vector &y, const Vector &mu, const SpdMatrix &Siginv, bool logscale){
    double ldsi =Siginv.logdet();
    return dmvn(y,mu,Siginv, ldsi, logscale); }
}
