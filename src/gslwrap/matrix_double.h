// matrix.h

//  This matrix class is a C++ wrapper for the GNU Scientific Library
//  Copyright (C) 2001 Ramin Nakisa

//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.

//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.

//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#if !defined( _matrix_double_h )
#define _matrix_double_h

#ifdef __HP_aCC //for aCC B3910B A.01.27
#include <iostream.h>
#include <fstream.h>
#include <iomanip.h>
#else //for gcc3
#include <iostream>
#include <fstream>
#include <iomanip>
#endif

#include <math.h>
#include <stdlib.h>
#include <assert.h>
///
#include <gsl/gsl_math.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_linalg.h>
#include <gslwrap/permutation.h>
#include <gslwrap/vector_double.h>

#define type_is
#ifdef  type_is
#define type_is_double
#endif

namespace gsl
{

///
class matrix
{
#ifdef type_is_double
	friend class matrix_float;
	friend class matrix_int;
#endif
public:
	typedef double value_type;
	typedef vector vector_type;

	///
	matrix();
	///
	matrix( size_t new_rows, size_t new_cols, bool clear = true );

	template<class oclass>
	void copy(const oclass &other)
	{
		if ( static_cast<const void *>( this ) == static_cast<const void *>( &other ) )
			return;

		set_dimensions( other.get_rows(), other.get_cols() );
		for ( size_t i = 0; i < get_rows(); i++ ) 
		{
			for ( size_t j = 0; j < get_cols(); j++ ) 
			{
				gsl_matrix_set( m, i, j, (double)other(i,j));
			}
		}
	}
/*    	template<>  */
/*    	void copy<matrix>(const matrix &other)  */
/*    	{  */
/*    		set_dimensions(other.size1(),other.size2());  */
/*    		gsl_matrix_memcpy( m, other.m );  */
/*    	}  */
	// copy constructor for type matrix
	matrix( const matrix &other ):m(NULL) {copy(other);}
	///
	template<class oclass>
	matrix( const oclass &other ):m(NULL) {copy(other);}

	///
	~matrix();
	///
//	matrix( const char *Filename );
	///
	size_t get_rows() const {return m->size1;}
	///
	size_t get_cols() const {return m->size2;}
	///
	size_t size1() const {return m->size1;}
	///
	size_t size2() const {return m->size2;}
   

	///
	void dimensions( size_t *num_rows, size_t *num_cols ) const;
	///
double        get_element ( size_t row, size_t col ) const {return  gsl_matrix_get( m, row, col ) ;}
	const double &operator()( size_t row, size_t col ) const {return *gsl_matrix_ptr( m, row, col ) ;}
double       &operator()( size_t row, size_t col )       {return *gsl_matrix_ptr( m, row, col ) ;}
	///
	void set_element( size_t row, size_t col, const double &v ){ gsl_matrix_set( m, row, col, v );}
	///
	void set_elements( const double & new_value );
	void set_all ( const double & new_value ) {gsl_matrix_set_all ( m, new_value );}
	void set_zero() {gsl_matrix_set_zero( m );}
	///
	void set_dimensions( size_t new_rows, size_t new_cols );
	///
	void load( const char *filename );
	///
	void save( const char *filename ) const;
	///
	friend ostream& operator<< ( ostream& os, const matrix& m );
	//This function writes the elements of the matrix m to the stream stream in binary format. The return value is 0 for success and GSL_EFAILED if there was a problem writing to the file. Since the data is written in the native binary format it may not be portable between different architectures.
	int fwrite (FILE * stream) const {return gsl_matrix_fwrite (stream, m);}

//This function reads into the matrix m from the open stream stream in binary format. The matrix m must be preallocated with the correct dimensions since the function uses the size of m to determine how many bytes to read. The return value is 0 for success and GSL_EFAILED if there was a problem reading from the file. The data is assumed to have been written in the native binary format on the same architecture. 
	int fread (FILE * stream) {return gsl_matrix_fread (stream, m);}
	int fscanf (FILE * stream) {return gsl_matrix_fscanf (stream, m);}

    ///
	void load_binary( const char *filename );
	///
	void save_binary( const char *filename ) const;
	///
	bool operator==( const matrix &other ) const;
	bool operator!=( const matrix &other ) const {return !((*this)==other);}
	
	matrix& operator=( const matrix &other ) {copy( other );return *this;}
	/// converts from any other matrix type
	template<class omatrix>
	matrix &operator=( const omatrix& other )
	{
			copy(other);
			return *this;
	}
   ///
	matrix operator+( const matrix &other ) const;
	///
	matrix operator+( const double &f ) const;
	///
	friend matrix operator+( const double &f, const matrix &other );
	///
	matrix &operator+=( const double &f );
	///
	matrix &operator+=( const matrix &other );
	///
	matrix operator-( const matrix &other ) const;
	///
	matrix operator-( const double &f ) const;
	///
	friend matrix operator-( const double &f, const matrix &other );
	///
	matrix &operator-=( const double &f );
	///
	matrix &operator-=( const matrix &other );
	///
	matrix operator*( const matrix &other ) const;
	///
	matrix operator*( const double &f ) const;
	///
	friend matrix operator*( const double &f, const matrix &other );
	///
	matrix &operator*=( const double &f );
	///
	matrix &operator*=( const matrix &other );
	///
	matrix operator/( const double &) const;
	///
	matrix &operator/=( const double &);
	///
	matrix transpose() const;
	///
	matrix LU_decomp(gsl::permutation *perm=NULL,int *psign=NULL) const;
	///
	matrix LU_invert() const;

	// return a submatrix of the this from row_min to row_max (not included!)
	matrix submatrix(size_t row_min, size_t row_max, size_t col_min, size_t col_max) const 
		{
			matrix m(row_max - row_min, col_max - col_min);
			for (size_t i = row_min ; i < row_max ; i++)
			{
				for (size_t j = col_min ; j < col_max ; j++)
				{
					m(i - row_min,j - col_min) = (*this)(i,j);
				}
			}
			return m;
		}
private:
	///
	void LU_decomp( gsl_matrix **a,
					gsl_permutation **permutation,
					int *sign ) const;
public:
	/** returns sum of all the matrix elements. */
    double sum() const;
	/** returns logarithm of the determinant of the matrix. */
	double LU_lndet() const;


	/** returns a vector_view of a single row of the matrix. */
	vector_view       row( size_t rowindex );
	const vector_view row( size_t rowindex ) const ;
	/** returns a vector_view of a single column of the matrix. */
	vector_view       column( size_t colindex );
	const vector_view column( size_t colindex ) const;
	/** returns a vector_view of the diagonal elements of the matrix. */
	vector_view       diagonal();
	const vector_view diagonal() const;


	void set_row_vec(const size_t rowindex, const vector &vec);

	void set_col_vec(const size_t colindex, const vector &vec);

	/** returns a vector containing a single row of the matrix. */
	vector get_row_vec( size_t rowindex ) const;
	/** returns a vector containing a single column of the matrix. */
	vector get_col_vec( size_t colindex ) const;

	/** returns a column matrix containing a single row of the matrix. */
	matrix get_row( size_t rowindex ) const;
	/** returns a column matrix containing a single column of the matrix. */
	matrix get_col( size_t colindex ) const;
	/** calculates sum of rows returned as a column matrix. */
	matrix row_sum() const;
	/** calculates sum of columns returned as a row matrix. */
	matrix column_sum() const;
	/** returns trace (diagonal sum) of a square matrix. */
	double trace() const;
	/** calculates cholesky decomposition of the matrix, returning success if matrix is positive definite. */
	int cholesky_decomp( matrix &a ) const;
//  	/** returns index of nearest row in matrix to vector argument. */
//  	int nearest_row_index( const matrix &v ) const;
	/** calculates covariance of the matrix columns. */
	matrix covariance() const;
	/** returns 1 if matrix is square, 0 otherwise. */
	bool is_square() const;
	/** diag operator (sets the diagonal elements of the matrix to the elements of v */
	void diag(const vector& v);
	/** set diagonal elements of a square matrix to f. */
	void set_diagonal( double f );
	/** sets matrix to a k dimensional unit matrix. */
	void identity( size_t k );
	/** returns sum of nth power of all elements. */
	double norm( double n ) const;

/*  Function: double gsl_matrix_max (const gsl_matrix * m)  */
/*      This function returns the maximum value in the matrix m.  */
	double max() const {return gsl_matrix_max(m);}
/*  Function: double gsl_matrix_min (const gsl_matrix * m)  */
/*      This function returns the minimum value in the matrix m.  */
	double min()const{return gsl_matrix_min(m);}

	/** This function returns 1 if all the elements of the matrix m are zero, and 0 otherwise. */
	bool isnull() const { return gsl_matrix_isnull(m);}
/*  Function: void gsl_matrix_minmax (const gsl_matrix * m, double * min_out, double * max_out)  */
/*      This function returns the minimum and maximum values in the matrix m, storing them in min_out and max_out.  */

/*  Function: void gsl_matrix_max_index (const gsl_matrix * m, size_t * imax, size_t * jmax)  */
/*      This function returns the indices of the maximum value in the matrix m, storing them in imax and jmax. When there are several equal maximum elements then the first element found */
/*      is returned.  */

/*  Function: void gsl_matrix_min_index (const gsl_matrix * m, size_t * imax, size_t * jmax)  */
/*      This function returns the indices of the minimum value in the matrix m, storing them in imax and jmax. When there are several equal minimum elements then the first element found */
/*      is returned.  */

/*  Function: void gsl_matrix_minmax_index (const gsl_matrix * m, size_t * imin, size_t * imax)  */
/*      This function returns the indices of the minimum and maximum values in the matrix m, storing them in (imin,jmin) and (imax,jmax). When there are several equal minimum or */
/*      maximum elements then the first elements found are returned.  */

	/** for interfacing with gsl c */
/*  	gsl_matrix       *gslobj()       {if (!m){cout << "matrix::gslobj ERROR, data not initialized!! " << endl; exit(-1);}return m;} */
/*  	const gsl_matrix *gslobj() const {if (!m){cout << "matrix::gslobj ERROR, data not initialized!! " << endl; exit(-1);}return m;} */
	gsl_matrix       *gslobj()       {assert(m);return m;}
	const gsl_matrix *gslobj() const {assert(m);return m;}
private:
	///
   gsl_matrix *m;

};
}
#undef type_is
#undef type_is_double

#endif // _matrix_double_h
