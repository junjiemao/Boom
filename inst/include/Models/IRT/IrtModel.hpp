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

#ifndef IRT_MODEL_HPP
#define IRT_MODEL_HPP


#include <Models/IRT/IRT.hpp>
#include <Models/ModelTypes.hpp>
#include <Models/Policies/CompositeParamPolicy.hpp>
#include <Models/Policies/IID_DataPolicy.hpp>
#include <Models/Policies/PriorPolicy.hpp>

namespace BOOM{
  class PosteriorSampler;
  class MvnModel;
  class MvRegModel;
  namespace IRT{

    class IrtModel
      : public CompositeParamPolicy,
        public IID_DataPolicy<Subject>,
        public PriorPolicy
    {
    public:
      typedef std::vector<string> StringVector;
      typedef Ptr<SubjectPrior> PriPtr;
      enum ModelTypeName{MultiSubscaleLogitCut};

      IrtModel();
      IrtModel(uint Nsub);
      IrtModel(const StringVector &Subscale_Names);
      IrtModel(const IrtModel &rhs);
      IrtModel * clone()const override;

      virtual double pdf(Ptr<Data> dp, bool logsc)const;
      virtual double pdf(Ptr<Subject> dp, bool logsc)const;

      void set_subscale_names(const StringVector &);
      const StringVector & subscale_names();
      ostream & print_subscales(ostream &, bool nl=true, bool decorate=true);

      uint nscales()const; // number of subscales
      uint nsubjects()const;
      uint nitems()const;

      void add_item(Ptr<Item>);
      Ptr<Item> find_item(const string &id, bool nag=true)const;
      ItemIt item_begin();
      ItemIt item_end();
      ItemItC item_begin()const;
      ItemItC item_end()const;

      void add_subject(Ptr<Subject>);
      SI subject_begin();
      SI subject_end();
      CSI subject_begin()const;
      CSI subject_end()const;
      Ptr<Subject> find_subject(const string &id, bool nag=true)const;

      void set_subject_prior(Ptr<MvnModel>);
      void set_subject_prior(Ptr<MvRegModel>);
      void set_subject_prior(Ptr<SubjectPrior>);
      PriPtr subject_prior();

      //----------- io functions -------
      void item_report(ostream &, uint max_name_width=40)const;
      void item_report(const string &fname)const;
    private:
      // see IRT.hpp for types
      StringVector subscale_names_;
      SubjectSet subjects_;
      ItemSet items;

      uint theta_freq, item_freq, R_freq, niter;
      bool theta_suppressed;
      std::vector<Ptr<Subject> > subject_subset;

      PriPtr subject_prior_;

      mutable Ptr<Subject> subject_search_helper;
      mutable Ptr<Item> item_search_helper;

      void allocate_subjects();
      // helper function for set_subject_prior
    };

    //======================================================================
    // subject_info_file can be either:
    // ID [delim]
    // -or-
    // ID [delim] bg1 [delim] bg2 [delim] ...
    void read_subject_info_file(const string &fname, Ptr<IrtModel> m,
                                char delim=' ');

    void read_item_response_file(const string &fname, Ptr<IrtModel> m);

  } // ends namespace IRT
} // ends namespace BOOM

#endif // IRT_MODEL_HPP
