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
#ifndef BOOM_IRT_HDR_HPP
#define BOOM_IRT_HDR_HPP

#include <map>
#include <set>
#include <vector>

#include <BOOM.hpp>
#include <Models/ModelTypes.hpp>
#include <Models/CategoricalData.hpp>

#include <LinAlg/Selector.hpp>

namespace BOOM{
  namespace IRT {
    typedef Selector Indicators;

    class Subject;
    class SubjectPrior;
    class IrtModel;
    class Item;

    struct SubjectLess
      : public std::binary_function<bool, Ptr<Subject>, Ptr<Subject> >
    {
      bool operator()(Ptr<Subject>, Ptr<Subject>)const; };

    struct ItemLess{
      bool operator()(Ptr<Item>, Ptr<Item>)const; };

    typedef Ptr<OrdinalData> Response;
    typedef std::vector<Ptr<Subject> >  SubjectSet;
    typedef SubjectSet::iterator SI;
    typedef SubjectSet::const_iterator CSI;  // Miami :)

    void add_subject(SubjectSet &, Ptr<Subject>);

    typedef std::set<Ptr<Item>, ItemLess> ItemSet;
    typedef ItemSet::iterator ItemIt;
    typedef ItemSet::const_iterator ItemItC;
    typedef std::map<Ptr<Item>,Response, ItemLess> ItemResponseMap;
    typedef ItemResponseMap::iterator IrIter;
    typedef ItemResponseMap::const_iterator IrIterC;

  }  // namespace IRT
}  // namespace BOOM
#endif // BOOM_IRT_HDR_HPP
