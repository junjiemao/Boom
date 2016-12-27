/*
  Copyright (C) 2007 Steven L. Scott

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
#ifndef BOOM_SPD_STORAGE_HPP
#define BOOM_SPD_STORAGE_HPP

#include <Models/DataTypes.hpp>
#include <boost/function.hpp>

namespace BOOM{
  namespace SPD{

    // Abstract base class for storing the different incarnations of a
    // symmetric positive definite matrix.  The class keeps track of
    // whether or not the currently stored data is current by placing
    // observers in other Storage objects.  When the user sets the
    // data in a Storage object, it signals the list of observers
    // watching it, so they know that their data needs to be updated.
    class Storage{
     public:
      Storage(bool current=false);
      Storage(const Storage &rhs);
      virtual ~Storage();
      virtual Storage * clone()const=0;

      // Returns the length of one side of the stored matrix.
      virtual uint dim()const=0;

      // The storage capacity requirements of either the full matrix
      // (minimial = false), or the triangle (minimial = true).
      virtual uint size(bool minimal=true)const;

      // Is the stored data current?
      bool current()const;

      // Signal any observers that a change has been made.
      void signal();

      // Marks the stored data as current.
      void set_current();

      // Creates an observer that can be placed in another Storage
      // object using add_observer().
      boost::function<void(void)> create_observer();

      // Adds an from another Storage object.
      void add_observer(boost::function<void(void)>);
     private:
      bool current_;
      void observe_changes();

      std::vector<boost::function<void(void)> >signals_;
    };

    //------------------------------------------------------------
    class SpdStorage;
    class CholStorage : public Storage{
    public:
      CholStorage();
      CholStorage(const SpdMatrix &S);
      CholStorage(const CholStorage &rhs);
      CholStorage * clone()const override;
      uint dim()const override;
      void set(const Matrix &L, bool sig=true);
      void refresh(const SpdStorage &);
      const Matrix & value()const;
    private:
      Matrix L;
    };
    typedef boost::shared_ptr<CholStorage> CholPtr;

    //------------------------------------------------------------
    class SpdStorage : public Storage{
    public:
      SpdStorage();
      SpdStorage(const SpdMatrix &S);
      SpdStorage(const SpdStorage &S);
      SpdStorage * clone()const override;
      uint dim()const override;
      const SpdMatrix & value()const;
      void set(const SpdMatrix &, bool sig=true);
      void refresh_from_chol(const CholStorage&, bool inv=false);
      void refresh_from_inv(const SpdStorage &, CholStorage &);
    private:
      SpdMatrix sig_;
    };
    typedef boost::shared_ptr<SpdStorage> SpdPtr;
  }
  //____________________________________________________________

  // This class stores a SpdMatrix matrix in several different formats.  It
  // keeps both the original matrix (though of as a variance matrix),
  // its inverse (ivar), as well as the cholesky triangles for these
  // two matrices.  This is extravagant, but it prevents excessive
  // computation.
  class SpdData
    : public DataTraits<Spd>
  {
  public:
    SpdData(uint n, double diag=1.0, bool ivar=false);
    SpdData(const SpdMatrix &S, bool ivar=false);
    SpdData(const SpdData &rhs);
    SpdData * clone()const override;

    virtual uint size(bool minimal=true)const;
    virtual uint dim()const;
    ostream & display(ostream &out)const override;

    const SpdMatrix & value()const override;
    void set(const SpdMatrix &v, bool sig=true) override;

    const SpdMatrix & var()const;
    const SpdMatrix & ivar()const;
    const Matrix & var_chol()const;
    const Matrix & ivar_chol()const;
    double ldsi()const;

    void set_var(const SpdMatrix &, bool signal=true);
    void set_ivar(const SpdMatrix &, bool signal=true);
    void set_var_chol(const Matrix &L, bool signal=true);
    void set_ivar_chol(const Matrix &L, bool signal=true);

    // Args:
    //   sd:  A vector of standard deviations to go along the diagonal.
    //   L:  The lower cholesky triangle for a correlation matrix.
    void set_S_Rchol(const Vector &sd, const Matrix &L);

    void ensure_ivar_current()const;
    void ensure_var_current()const;
    void ensure_var_chol_current()const;
    void ensure_ivar_chol_current()const;

  private:
    typedef boost::shared_ptr<SPD::SpdStorage>  SpdPtr;
    typedef boost::shared_ptr<SPD::CholStorage> CholPtr;
    typedef boost::shared_ptr<SPD::Storage> StoragePtr;

    mutable SpdPtr var_;
    mutable SpdPtr ivar_;
    mutable CholPtr var_chol_;
    mutable CholPtr ivar_chol_;

    // Points to the current representation: variance, inverse
    // variance, cholesky of variance, cholesky of inverse.
    StoragePtr current_rep_;

    // Create observers among all the available storage modes.
    void setup_storage();
    void ensure_current(SpdPtr, CholPtr, SpdPtr, CholPtr)const;
    void ensure_chol_current(CholPtr, SpdPtr, CholPtr, SpdPtr)const;
  };
}
#endif// BOOM_SPD_STORAGE_HPP
