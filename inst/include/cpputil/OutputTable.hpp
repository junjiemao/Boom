/*
  Copyright (C) 2008 Steven L. Scott

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
#ifndef BOOM_OUTPUT_TABLE_HPP
#define BOOM_OUTPUT_TABLE_HPP
#include <cpputil/string_utils.hpp>
#include <uint.hpp>

namespace BOOM{

class OutputTable{
 public:
  OutputTable(uint pad=2);
  Svec & column(uint i);
  OutputTable & add_row(const Svec &);
  OutputTable & add_column(const Svec &);
  OutputTable & add_to_column(const string &, uint i);

  ostream & print(ostream &)const;
  void equalize_rows();
 private:
  uint pad_;
  std::vector<Svec> cols_;
};
}
#endif// BOOM_OUTPUT_TABLE_HPP
