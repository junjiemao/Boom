/*
  Copyright (C) 2006 Steven L. Scott

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
#ifndef BOOM_STRING_SPLIT_HPP
#define BOOM_STRING_SPLIT_HPP

#include <string>
#include <vector>

namespace BOOM{
typedef std::vector<std::string> Svec;
class StringSplitter{
 public:
  StringSplitter(std::string sep=" \t", bool allow_quotes=true);
  Svec operator()(const std::string &s)const;
  //  std::vector<std::string>
 private:
  std::string delim;
  std::string quotes;
  bool delimited;
};

}
#endif // BOOM_STRING_SPLIT_HPP
