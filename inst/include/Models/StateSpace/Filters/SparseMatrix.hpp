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

#ifndef BOOM_SPARSE_MATRIX_HPP_
#define BOOM_SPARSE_MATRIX_HPP_

#include <map>
#include <boost/shared_ptr.hpp>

#include <LinAlg/Vector.hpp>
#include <LinAlg/Matrix.hpp>
#include <LinAlg/SpdMatrix.hpp>
#include <LinAlg/SubMatrix.hpp>

#include <Models/ParamTypes.hpp>

#include <cpputil/RefCounted.hpp>
#include <cpputil/Ptr.hpp>
#include <cpputil/report_error.hpp>

#include <Models/StateSpace/Filters/SparseVector.hpp>
#include <Models/Glm/GlmCoefs.hpp>

namespace BOOM{
  //======================================================================
  // The SparseMatrixBlock classes are designed to be used as elements
  // in a BlockDiagonalMatrix.  They only need to implement the
  // functionality required by the DiagonalMatrix implementation.
  class SparseMatrixBlock : private RefCounted {
   public:
    ~SparseMatrixBlock() override{}
    virtual SparseMatrixBlock *clone() const = 0;

    virtual int nrow() const = 0;
    virtual int ncol() const = 0;

    // lhs = this * rhs
    virtual void multiply(VectorView lhs,
                          const ConstVectorView &rhs) const = 0;

    // lhs = this.transpose() * rhs
    virtual void Tmult(VectorView lhs, const ConstVectorView &rhs) const = 0;

    // Replace x with this * x.  Assumes *this is square.
    virtual void multiply_inplace(VectorView x) const = 0;

    // m = this * m
    virtual void matrix_multiply_inplace(SubMatrix m) const;

    // m = m * this->t();
    virtual void matrix_transpose_premultiply_inplace(SubMatrix m) const;

    // Add *this to block
    // TODO(stevescott):  needs unit tests for all derived classes
    virtual void add_to(SubMatrix block) const = 0;
    void conforms_to_rows(int i) const;
    void conforms_to_cols(int i) const;
    void check_can_add(const SubMatrix &m) const;
    virtual Matrix dense() const;
    int ref_count() const {return RefCounted::ref_count();}
   private:
    friend void intrusive_ptr_add_ref(SparseMatrixBlock *m){m->up_count();}
    friend void intrusive_ptr_release(SparseMatrixBlock *m){
      m->down_count();
      if(m->ref_count() == 0) delete m;}
  };

  //======================================================================
  // The LocalLinearTrendMatrix is
  //  1 1
  //  0 1
  //  It corresponds to state elements [mu, delta], where mu[t] =
  //  mu[t-1] + delta[t-1] + error[0] and de[ta[t] = delta[t-1] +
  //  error[1].
  class LocalLinearTrendMatrix : public SparseMatrixBlock {
   public:
    LocalLinearTrendMatrix * clone() const override;
    int nrow() const override {return 2;}
    int ncol() const override {return 2;}
    void multiply(VectorView lhs, const ConstVectorView &rhs) const override;
    void Tmult(VectorView lhs, const ConstVectorView &rhs) const override;
    void multiply_inplace(VectorView x) const override;
    void add_to(SubMatrix block) const override;
    Matrix dense() const override;
  };

  //======================================================================
  // A SparseMatrixBlock filled with a DenseMatrix.  I.e. a dense
  // sub-block of a sparse matrix.
  class DenseMatrix : public SparseMatrixBlock {
   public:
    DenseMatrix(const Matrix &m) : m_(m){}
    DenseMatrix(const DenseMatrix &rhs)
        : SparseMatrixBlock(rhs),
          m_(rhs.m_)
    {}
    DenseMatrix * clone() const override {return new DenseMatrix(*this);}
    int nrow() const override {return m_.nrow();}
    int ncol() const override {return m_.ncol();}
    void multiply(VectorView lhs, const ConstVectorView &rhs) const override {
      lhs = m_ * rhs; }
    void Tmult(VectorView lhs, const ConstVectorView &rhs) const override {
      lhs = m_.Tmult(rhs); }
    void multiply_inplace(VectorView x) const override { x = m_ * x;}
    void add_to(SubMatrix block) const override { block += m_; }
    Matrix dense() const override { return m_; }
   private:
    Matrix m_;
  };

  //======================================================================
  // A SparseMatrixBlock filled with a dense SpdMatrix.
  class DenseSpd : public SparseMatrixBlock {
   public:
    DenseSpd(const SpdMatrix &m) : m_(m){}
    DenseSpd(const DenseSpd &rhs) : SparseMatrixBlock(rhs), m_(rhs.m_){}
    DenseSpd * clone() const override {return new DenseSpd(*this);}
    void set_matrix(const SpdMatrix &m){m_ = m;}
    int nrow() const override {return m_.nrow();}
    int ncol() const override {return m_.ncol();}
    void multiply(VectorView lhs, const ConstVectorView &rhs) const override {
      lhs = m_ * rhs; }
    void Tmult(VectorView lhs, const ConstVectorView &rhs) const override {
      lhs = m_ * rhs; }
    void multiply_inplace(VectorView x) const override { x = m_ * x;}
    void add_to(SubMatrix block) const override { block += m_; }
   private:
    SpdMatrix m_;
  };

  //======================================================================
  // A component that is is a diagonal matrix (a square matrix with
  // zero off-diagonal components).  The diagonal elements can be
  // changed to arbitrary values after construction.  This class is
  // conceptutally similar to UpperLeftDiagonalMatrix, but it allows
  // different behavior with respect to setting its elements to
  // arbitrary values.
  class DiagonalMatrixBlock : public SparseMatrixBlock {
   public:
    DiagonalMatrixBlock(int size)
        : diagonal_elements_(size)
    {}
    DiagonalMatrixBlock(const Vector &diagonal_elements)
        : diagonal_elements_(diagonal_elements)
    {}
    DiagonalMatrixBlock * clone() const override {return new DiagonalMatrixBlock(*this);}
    void set_elements(const Vector &v){diagonal_elements_ = v;}
    void set_elements(const VectorView &v){diagonal_elements_ = v;}
    void set_elements(const ConstVectorView &v){diagonal_elements_ = v;}
    double operator[](int i) const {return diagonal_elements_[i];}
    double & operator[](int i){return diagonal_elements_[i];}
    int nrow() const override {return diagonal_elements_.size();}
    int ncol() const override {return diagonal_elements_.size();}
    void multiply(VectorView lhs, const ConstVectorView &rhs) const override {
      lhs = diagonal_elements_;
      lhs *= rhs;
    }
    void Tmult(VectorView lhs, const ConstVectorView &rhs) const override {
      multiply(lhs, rhs);
    }
    void multiply_inplace(VectorView x) const override {x *= diagonal_elements_;}
    void matrix_multiply_inplace(SubMatrix m) const override {
      for(int i = 0; i < m.ncol(); ++i){
        m.col(i) *= diagonal_elements_;
      }
    }

    void matrix_transpose_premultiply_inplace(SubMatrix m) const override {
      for(int i = 0; i < m.nrow(); ++i){
        m.row(i) *= diagonal_elements_;
      }
    }

    void add_to(SubMatrix block) const override {
      block.diag() += diagonal_elements_;
    }

   private:
    Vector diagonal_elements_;
  };

  //======================================================================
  // A seasonal state space matrix describes the state evolution in an
  // dynamic linear model.  Conceptually it looks like this:
  // -1 -1 -1 -1 ... -1
  //  1  0  0  0 .... 0
  //  0  1  0  0 .... 0
  //  0  0  1  0 .... 0
  //  0  0  0  1 .... 0
  // A row of -1's at the top, then an identity matrix with a column
  // of 0's appended on the right hand side.
  class SeasonalStateSpaceMatrix : public SparseMatrixBlock {
   public:
    SeasonalStateSpaceMatrix(int number_of_seasons);
    SeasonalStateSpaceMatrix * clone() const override;
    int nrow() const override;
    int ncol() const override;

    // lhs = (*this) * rhs;
    void multiply(VectorView lhs, const ConstVectorView &rhs) const override;
    // lhs = this->transpose() * rhs
    void Tmult(VectorView lhs, const ConstVectorView &rhs) const override;
    // x = (*this) * x;
    void multiply_inplace(VectorView x) const override;
    void add_to(SubMatrix block) const override;
    Matrix dense() const override;
   private:
    int number_of_seasons_;
  };

  //======================================================================
  // An AutoRegressionTransitionMatrix is a [p X p] matrix with top
  // row containing a vector of autoregression parameters.  The lower
  // left block is a [p-1 X p-1] identity matrix (i.e. a shift-down
  // operator), and the lower right block is a [p-1 X 1] vector of
  // 0's.
  class AutoRegressionTransitionMatrix : public SparseMatrixBlock{
   public:
    AutoRegressionTransitionMatrix(Ptr<GlmCoefs> rho);
    AutoRegressionTransitionMatrix(const AutoRegressionTransitionMatrix &rhs);
    AutoRegressionTransitionMatrix * clone() const override;

    int nrow() const override;
    int ncol() const override;
    // lhs = this * rhs
    void multiply(VectorView lhs, const ConstVectorView &rhs) const override;
    void Tmult(VectorView lhs, const ConstVectorView &rhs) const override;
    void multiply_inplace(VectorView x) const override;
    void add_to(SubMatrix block) const override;
    // virtual void matrix_multiply_inplace(SubMatrix m) const;
    // virtual void matrix_transpose_premultiply_inplace(SubMatrix m) const;
    Matrix dense() const override;
   private:
    Ptr<GlmCoefs> autoregression_params_;
  };

  //======================================================================
  // The [dim x dim] identity matrix
  class IdentityMatrix : public SparseMatrixBlock{
   public:
    IdentityMatrix(int dim) : dim_(dim){}
    IdentityMatrix * clone() const override {return new IdentityMatrix(*this);}
    int nrow() const override {return dim_;}
    int ncol() const override {return dim_;}
    void multiply(VectorView lhs, const ConstVectorView &rhs) const override {
      conforms_to_cols(rhs.size());
      conforms_to_rows(lhs.size());
      lhs = rhs;}
    void Tmult(VectorView lhs, const ConstVectorView &rhs) const override {
      conforms_to_rows(rhs.size());
      conforms_to_cols(lhs.size());
      lhs = rhs;}
    void multiply_inplace(VectorView x) const override {}
    void matrix_multiply_inplace(SubMatrix m) const override {}
    void matrix_transpose_premultiply_inplace(SubMatrix m) const override {}
    void add_to(SubMatrix block) const override { block.diag() += 1.0; }
   private:
    int dim_;
  };

  //======================================================================
  // An empty matrix with no rows or columns.  This is useful for
  // models which are all deterministic, with no random component.
  class EmptyMatrix : public SparseMatrixBlock {
   public:
    EmptyMatrix * clone() const override {return new EmptyMatrix(*this);}
    int nrow() const override {return 0;}
    int ncol() const override {return 0;}
    void multiply(VectorView lhs, const ConstVectorView &rhs) const override {}
    void Tmult(VectorView lhs, const ConstVectorView &rhs) const override {}
    void multiply_inplace(VectorView x) const override{}
    void matrix_multiply_inplace(SubMatrix m) const override {}
    void matrix_transpose_premultiply_inplace(SubMatrix m) const override {}
    void add_to(SubMatrix block) const override {}
  };

  //======================================================================
  // A scalar constant times the identity matrix
  class ConstantMatrix : public SparseMatrixBlock{
   public:
    ConstantMatrix(int dim, double value)
        : dim_(dim),
          value_(value)
    {}
    ConstantMatrix * clone() const override {return new ConstantMatrix(*this);}
    int nrow() const override {return dim_;}
    int ncol() const override {return dim_;}
    void multiply(VectorView lhs, const ConstVectorView &rhs) const override {
      conforms_to_cols(rhs.size());
      conforms_to_rows(lhs.size());
      // Doing this operation in two steps, insted of lhs = rhs *
      // value_, eliminates a temporary that profiliing found to be
      // expensive.
      lhs = rhs;
      lhs *= value_;
    }
    void Tmult(VectorView lhs, const ConstVectorView &rhs) const override {
      conforms_to_rows(rhs.size());
      conforms_to_cols(lhs.size());
      lhs = rhs * value_;
    }
    void multiply_inplace(VectorView x) const override {
      x *= value_;}
    void matrix_multiply_inplace(SubMatrix x) const override {
      x *= value_;}
    void matrix_transpose_premultiply_inplace(SubMatrix x) const override {
      x *= value_;}
    void add_to(SubMatrix block) const override {block.diag() += value_;}
    void set_value(double value){value_ = value;}
   private:
    int dim_;
    double value_;
  };

  //======================================================================
  // A square matrix of all zeros.
  class ZeroMatrix : public ConstantMatrix{
   public:
    ZeroMatrix(int dim) : ConstantMatrix(dim, 0.0){}
    ZeroMatrix * clone() const override {return new ZeroMatrix(*this);}
    void add_to(SubMatrix block) const override {}
  };

  //======================================================================
  //  A matrix that is all zeros except for a single nonzero value in
  //  the (0,0) corner.
  class UpperLeftCornerMatrix : public SparseMatrixBlock {
   public:
    UpperLeftCornerMatrix(int dim, double value)
        : dim_(dim),
          value_(value)
    {}
    UpperLeftCornerMatrix * clone() const override {
      return new UpperLeftCornerMatrix(*this);}
    int nrow() const override {return dim_;}
    int ncol() const override {return dim_;}
    void multiply(VectorView lhs, const ConstVectorView &rhs) const override {
      conforms_to_cols(rhs.size());
      conforms_to_rows(lhs.size());
      lhs = rhs * 0;
      lhs[0] = rhs[0] * value_;
    }
    void Tmult(VectorView lhs, const ConstVectorView &rhs) const override {
      // An upper left corner matrix is symmetric, so Tmult is the
      // same as multiply.
      multiply(lhs, rhs); }
    void multiply_inplace(VectorView x) const override {
      double tmp = x[0];
      x = 0;
      x[0] = tmp * value_;
    }
    void set_value(double value){value_ = value;}
    void add_to(SubMatrix block) const override { block(0,0) += value_; }
   private:
    int dim_;
    double value_; // the value in the upper left corner of the matrix
  };

  //======================================================================
  // An nx1 rectangular matrix with a 1 in the upper left corner and
  // 0's elsewhere.  This is intended to be the "expander matrix" for
  // state errors in models with a single dimension of randomness but
  // multiple dimensions of state.
  //
  // (*this) = [1, 0, 0, ..., 0]^T
  class FirstElementSingleColumnMatrix
      : public SparseMatrixBlock {
   public:
    FirstElementSingleColumnMatrix(int nrow) : nrow_(nrow) {}
    FirstElementSingleColumnMatrix *clone() const override {
      return new FirstElementSingleColumnMatrix(*this);
    }

    int nrow() const override {return nrow_;};
    int ncol() const override {return 1;}

    void multiply(VectorView lhs,
                  const ConstVectorView &rhs) const override {
      lhs[0] = rhs[0];
    }

    void Tmult(VectorView lhs, const ConstVectorView &rhs) const override {
      lhs[0] = rhs[0];
    }

    void multiply_inplace(VectorView x) const override {
      report_error("multiply_inplace only works for square matrices.");
    }

    void matrix_multiply_inplace(SubMatrix m) const override {
      report_error("matrix_multiply_inplace only works for square matrices.");
    }

    void matrix_transpose_premultiply_inplace(SubMatrix m) const override {
      report_error("matrix_transpose_premultiply_inplace only works for "
                   "square matrices.");
    }

    void add_to(SubMatrix block) const override {
      block(0, 0) += 1.0;
    }

    Matrix dense() const override {
      Matrix ans(nrow(), ncol(), 0.0);
      ans(0, 0) = 1.0;
      return ans;
    }

   private:
    int nrow_;
  };
  //======================================================================
  // A rectangular matrix consisting of an Identity matrix with rows
  // of zeros appended on bottom.
  //
  class ZeroPaddedIdentityMatrix : public SparseMatrixBlock {
   public:
    ZeroPaddedIdentityMatrix(int nrow, int ncol)
        : nrow_(nrow),
          ncol_(ncol) {
      if (nrow < ncol) {
        report_error("A ZeroPaddedIdentityMatrix must have at least as many "
                     "rows as columns.");
      }
    }
    ZeroPaddedIdentityMatrix * clone() const override {
      return new ZeroPaddedIdentityMatrix(*this);
    }
    int nrow() const override {return nrow_;}
    int ncol() const override {return ncol_;}
    void multiply(VectorView lhs, const ConstVectorView &rhs) const override {
      conforms_to_rows(lhs.size());
      conforms_to_cols(rhs.size());
      for (int i = 0; i < ncol_; ++i) {
        lhs[i] = rhs[i];
      }
      for (int i = ncol_; i < lhs.size(); ++i) {
        lhs[i] = 0.0;
      }
    }

    void Tmult(VectorView lhs, const ConstVectorView &rhs) const override {
      conforms_to_cols(lhs.size());
      conforms_to_rows(rhs.size());
      for (int i = 0; i < ncol_; ++i) {
        lhs[i] = rhs[i];
      }
    }

    void multiply_inplace(VectorView x) const override {
      report_error("multiply_inplace only applies to square matrices.");
    }

    void matrix_multiply_inplace(SubMatrix x) const override {
      report_error("matrix_multiply_inplace only applies to square matrices.");
    }

    void matrix_transpose_premultiply_inplace(SubMatrix x) const override {
      report_error("matrix_transpose_premultiply_inplace only applies "
                   "to square matrices.");
    }

    void add_to(SubMatrix m) const override {
      conforms_to_rows(m.nrow());
      conforms_to_cols(m.ncol());
      m.diag() += 1.0;
    }

    Matrix dense() const override {
      Matrix ans(nrow_, ncol_, 0.0);
      ans.diag() = 1.0;
      return ans;
    }

   private:
    int nrow_;
    int ncol_;
  };

  //======================================================================
  // A diagonal matrix that is zero in all but (at most) one element.
  class SingleSparseDiagonalElementMatrix : public SparseMatrixBlock{
   public:
    SingleSparseDiagonalElementMatrix(int dim, double value, int which_element)
        : dim_(dim),
          value_(value),
          which_element_(which_element)
    {}
    SingleSparseDiagonalElementMatrix * clone() const override {
      return new SingleSparseDiagonalElementMatrix(*this);}

    void set_value(double value){value_ = value;}
    void set_element(int which_element){which_element_ = which_element;}

    int nrow() const override {return dim_;}
    int ncol() const override {return dim_;}
    void multiply(VectorView lhs, const ConstVectorView &rhs) const override {
      conforms_to_rows(lhs.size());
      conforms_to_cols(rhs.size());
      lhs = 0;
      lhs[which_element_] = value_ * rhs[which_element_];
    }
    void Tmult(VectorView lhs, const ConstVectorView &rhs) const override {
      // Symmetric
      multiply(lhs, rhs);
    }
    void multiply_inplace(VectorView x) const override {
      conforms_to_cols(x.size());
      x[which_element_] *= value_;
    }
    void add_to(SubMatrix block) const override {
      check_can_add(block);
      block(which_element_, which_element_) += value_;
    }
   private:
    int dim_;
    double value_;
    int which_element_;
  };

  //======================================================================
  // A diagonal matrix whose diagonal entries are zero beyond a
  // certain point.  Diagonal entry i is the product of a
  // BOOM::UnivParams and a constant scalar factor.  Interesting
  // special cases that can be handled include
  //  *) The entire diagonal is nonzero.
  //  *) All scale factors are 1.
  class UpperLeftDiagonalMatrix : public SparseMatrixBlock {
   public:
    UpperLeftDiagonalMatrix(const std::vector<Ptr<UnivParams> > &diagonal,
                            int dim)
        : diagonal_(diagonal),
          dim_(dim),
          constant_scale_factor_(diagonal.size(), 1.0)
    {
      check_diagonal_dimension(dim_, diagonal_);
      check_scale_factor_dimension(diagonal, constant_scale_factor_);
    }

    UpperLeftDiagonalMatrix(const std::vector<Ptr<UnivParams> > &diagonal,
                            int dim,
                            const Vector &scale_factor)
        : diagonal_(diagonal),
          dim_(dim),
          constant_scale_factor_(scale_factor)
    {
      check_diagonal_dimension(dim_, diagonal_);
      check_scale_factor_dimension(diagonal_, constant_scale_factor_);
    }

    UpperLeftDiagonalMatrix * clone() const override {
      return new UpperLeftDiagonalMatrix(*this);}
    int nrow() const override {return dim_;};
    int ncol() const override {return dim_;}
    void multiply(VectorView lhs, const ConstVectorView &rhs) const override {
      conforms_to_cols(rhs.size());
      conforms_to_rows(lhs.size());
      for(int i = 0; i < diagonal_.size(); ++i){
        lhs[i] = rhs[i] * diagonal_[i]->value() * constant_scale_factor_[i];
      }
      for(int i = diagonal_.size(); i < dim_; ++i) lhs[i] = 0;
    }
    void Tmult(VectorView lhs, const ConstVectorView &rhs) const override {
      multiply(lhs, rhs);
    }
    void multiply_inplace(VectorView x) const override {
      conforms_to_cols(x.size());
      for(int i = 0; i < diagonal_.size(); ++i){
        x[i] *= diagonal_[i]->value() * constant_scale_factor_[i];
      }
      for(int i = diagonal_.size(); i < dim_; ++i) x[i] = 0;
    }

    void add_to(SubMatrix block) const override {
      conforms_to_rows(block.nrow());
      conforms_to_cols(block.ncol());
      for(int i = 0; i < diagonal_.size(); ++i){
        block(i,i) += diagonal_[i]->value() * constant_scale_factor_[i];
      }
    }
   private:
    std::vector<Ptr<UnivParams> > diagonal_;
    int dim_;
    Vector constant_scale_factor_;

    void check_diagonal_dimension(
        int dim, const std::vector<Ptr<UnivParams> > &diagonal){
      if(dim < diagonal.size()){
        report_error("dim must be at least as large as diagonal in "
                     "constructor for UpperLeftDiagonalMatrix");
      }
    }

    void check_scale_factor_dimension(
        const std::vector<Ptr<UnivParams> > &diagonal,
        const Vector &scale_factor){
      if(diagonal.size() != scale_factor.size()){
        report_error("diagonal and scale_factor must be the same size in "
                     "constructor for UpperLeftDiagonalMatrix");
      }
    }
  };

  //======================================================================
  // A SparseKalmanMatrix is a sparse matrix that can be used in the
  // Kalman recursions.  This may get expanded to a more full fledged
  // sparse matrix class later on, if need be.
  class SparseKalmanMatrix {
   public:
    virtual ~SparseKalmanMatrix(){}

    virtual int nrow() const = 0;
    virtual int ncol() const = 0;

    virtual Vector operator*(const Vector &v) const = 0;
    virtual Vector operator*(const VectorView &v) const = 0;
    virtual Vector operator*(const ConstVectorView &v) const = 0;

    virtual Vector Tmult(const Vector &v) const = 0;

    // Replace the argument P with
    //   this * P * this.transpose()
    // This only works with square matrices.  Non-square matrices will throw.
    virtual void sandwich_inplace(SpdMatrix &P) const;
    virtual void sandwich_inplace_submatrix(SubMatrix P) const;

    // Replace the argument P with
    //    this->transpose() * P * this
    // This only works with square matrices.  Non-square matrices will throw.
    virtual void sandwich_inplace_transpose(SpdMatrix &P) const;

    // Returns *this * P * this->transpose().
    // This is a valid call, even if *this is non-square.
    virtual SpdMatrix sandwich(const SpdMatrix &P) const;

    // Returns this->Tmult(P) * (*this), which is equivalent to
    // calling this->transpose()->sandwich(P) (or it would be if
    // this->transpose() was defined).  This is a valid call, even if
    // *this is non-square.
    virtual SpdMatrix sandwich_transpose(const SpdMatrix &P) const;

    // P += *this
    virtual Matrix & add_to(Matrix &P) const = 0;
    virtual SubMatrix add_to_submatrix(SubMatrix P) const;

    // Returns a dense matrix representation of *this.  Mainly for
    // debugging and testing.
    //
    // The default implementation only works for square matrices.
    // Child classes that can be non-square should override.
    virtual Matrix dense() const;
  };

  //======================================================================
  // The state transition equation for a dynamic linear model will
  // typically involve a block diagonal matrix.  The blocks will
  // typically be:  SeasonalStateSpaceMatrix, IdentityMatrix, etc.
  class BlockDiagonalMatrix : public SparseKalmanMatrix{
   public:
    // Start off with an empty matrix.  Use add_block() to add blocks
    // Adds a block to the block diagonal matrix
    BlockDiagonalMatrix();

    void add_block(Ptr<SparseMatrixBlock> m);
    void replace_block(int which_block, Ptr<SparseMatrixBlock> b);
    void clear();

    int nrow() const override;
    int ncol() const override;

    Vector operator*(const Vector &v) const override;
    Vector operator*(const VectorView &v) const override;
    Vector operator*(const ConstVectorView &v) const override;

    Vector Tmult(const Vector &r) const override;
    // P -> this * P * this.transpose()
    void sandwich_inplace(SpdMatrix &P) const override;
    void sandwich_inplace_submatrix(SubMatrix P) const override;

    // sandwich(P) = this * P * this.transpose()
    SpdMatrix sandwich(const SpdMatrix &P) const override;

    Matrix & add_to(Matrix &P) const override;
    SubMatrix add_to_submatrix(SubMatrix P) const override;
   private:
    // Replace middle with left * middle * right.transpose()
    void sandwich_inplace_block(const SparseMatrixBlock &left,
                                const SparseMatrixBlock &right,
                                SubMatrix middle) const;

    // Returns the (i,j) block of the matrix m, with block sizes
    // determined by the rows and columns of the entries in blocks_.
    SubMatrix get_block(Matrix &m, int i, int j) const;
    SubMatrix get_row_block(Matrix &m, int block) const;
    SubMatrix get_col_block(Matrix &m, int block) const;
    SubMatrix get_submatrix_block(SubMatrix m, int i, int j) const;
    std::vector<Ptr<SparseMatrixBlock> > blocks_;

    int nrow_;
    int ncol_;

    // row_boundaries_[i] contains the one-past-the-end position of
    std::vector<int> row_boundaries_;
    std::vector<int> col_boundaries_;
  };
  //======================================================================

  Vector operator*(const SparseMatrixBlock &,
                const Vector &);

  // P += TPK * K.transpose * w
  void add_outer_product(
      SpdMatrix &P,
      const Vector &TPK,
      const Vector &K,
      double w);

  // P += RQR
  void add_block_diagonal(SpdMatrix &P, const BlockDiagonalMatrix &RQR);

}
#endif // BOOM_SPARSE_MATRIX_HPP_
