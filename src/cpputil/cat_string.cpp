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

#include <cpputil/string_utils.hpp>
#include <string>
#include <iostream>
#include <sstream>

namespace BOOM{
  std::string operator+(std::string s, int i){
    std::ostringstream ans;
    ans << s << i;
    return ans.str();
  }

  std::string operator+(int i , std::string s){
    std::ostringstream ans;
    ans << i<< s;
    return ans.str();
  }

  std::string operator+(std::string s, double d){
    std::ostringstream ans;
    ans << s << d;
    return ans.str();
  }

  std::string operator+(double d, std::string s){
    std::ostringstream ans;
    ans << d << s;
    return ans.str();
  }

  std::string operator+=(std::string &s, int n){
    std::ostringstream ans;
    ans << n;
    s +=ans.str();
    return s;
  }

  std::string operator+=(std::string &s, double d){
    std::ostringstream ans;
    ans << d;
    s +=ans.str();
    return s;
  }

  std::string operator>>(std::string s, int &n){
    std::istringstream ans(s);
    ans >> n;
    return ans.str();
  }

  std::string operator>>(std::string s, double &d){
    std::istringstream ans(s);
    ans >> d;
    return ans.str();
  }

}
