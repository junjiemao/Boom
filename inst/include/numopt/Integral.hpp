/*
  Copyright (C) 2005-2010 Steven L. Scott

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

#ifndef BOOM_NUMERICAL_INTEGRAL_HPP_
#define BOOM_NUMERICAL_INTEGRAL_HPP_

#include <boost/function.hpp>
#include <cpputil/math_utils.hpp>
#include <vector>

namespace BOOM{

  class Integral{
   public:
    typedef boost::function<double(double)> Fun;
    Integral(const Fun &integrand,
             double lower_limit = BOOM::negative_infinity(),
             double upper_limit = BOOM::infinity(),
             int iwork_limit=100
             );

    void set_work_vector_size(int lenw);
    void set_absolute_epsilon(double eps);
    void set_relative_epsilon(double eps);
    void throw_on_error(bool);
    // this is the main driver
    double integrate();

    // the following are available after calling integrate:
    double absolute_error()const;
    int number_of_function_evaluations()const;
    int number_of_partitions()const;
    int error_code()const;
    std::string error_message()const;

    std::string debug_string() const;
   private:
    Fun f_;               // integrand
    double lo_, hi_;      // limits of integration

    // everything below this point is a variable to be passed to the
    // fortran code
    int limit_;
    std::vector<double> work_;
    std::vector<int> iwork_;
    double rel_tol_;
    double abs_tol_;

    double result_;
    double abs_err_;
    int neval_;
    int npartitions_;

    bool throw_on_error_;
    int error_code_;
  };

}
#endif // BOOM_NUMERICAL_INTEGRAL_HPP_
