/*
 * SpFunctions.cpp
 *
 *  Created on: 3 Oct 2016
 *      Author: ljuvela
 */

#include <gsl/gsl_spline.h>			/* GSL, Interpolation */
#include <gsl/gsl_fft_real.h>
#include <gsl/gsl_fft_halfcomplex.h>
#include <gsl/gsl_errno.h>       /* GSL, Error handling */
#include <gsl/gsl_poly.h>        /* GSL, Polynomials */
#include <gsl/gsl_sort_double.h> /* GSL, Sort double */
#include <gsl/gsl_sort_vector.h> /* GSL, Sort vector */
#include <gsl/gsl_complex.h>     /* GSL, Complex numbers */
#include <gsl/gsl_complex_math.h>   /* GSL, Arithmetic operations for complex numbers */
#include <gslwrap/vector_double.h>
#include <vector>
#include <queue>
#include "ComplexVector.h"
#include "definitions.h"
#include "SpFunctions.h"

void Filter(const gsl::vector &b, const gsl::vector &a, const gsl::vector &x, gsl::vector *y) {
	int i,j;
	double sum;

	int order = 0;

	if(!y->is_set()) {
		*y = gsl::vector(x.size(),true);
	} else {
		if(y->size() > x.size()) {
			y->resize(x.size());
      } else {
         order = x.size()-y->size();
      }
      y->set_all(0.0);
	}


	gsl::vector result(x.size(),true);
	/* Filter */
	for (i=0;i<(int)x.size();i++){
		sum = 0.0;
		for(j=0;j<(int)b.size();j++) { /* Loop for FIR filter */
			if ((i-j) >= 0) {
				sum += x(i-j)*b(j);
			}
		}
		for(j=1;j<(int)a.size();j++) { /* Loop for IIR filter */
			if ((i-j) >= 0) {
				sum -= result(i-j)*a(j);
			}
		}
		result(i) = sum;
	}

	for(i=0;i<(int)y->size();i++)
      (*y)(i) = result(i+order);
}

void Filter(const std::vector<double> &b, const std::vector<double> &a, const gsl::vector &x, gsl::vector *y) {
	size_t i;
	gsl::vector a_vec = gsl::vector(a.size());
	for(i=0;i<a_vec.size();i++)
		a_vec(i) = a[i];

	gsl::vector b_vec = gsl::vector(b.size());
	for(i=0;i<b_vec.size();i++)
		b_vec(i) = b[i];

	Filter(b_vec, a_vec, x, y);
}


void Filter(const gsl::vector b, const std::vector<double> &a, const gsl::vector &x, gsl::vector *y) {
	size_t i;
	gsl::vector a_vec = gsl::vector(a.size());
	for(i=0;i<a_vec.size();i++)
		a_vec(i) = a[i];

	Filter(b, a_vec, x, y);
}

void Filter(const std::vector<double> &b, const gsl::vector a, const gsl::vector &x, gsl::vector *y) {
	size_t i;
	gsl::vector b_vec = gsl::vector(b.size());
	for(i=0;i<b_vec.size();i++)
		b_vec(i) = b[i];

	Filter(b_vec, a, x, y);
}


void InterpolateLinear(const gsl::vector &vector, const size_t interpolated_size, gsl::vector *i_vector) {
	size_t len = vector.size();
   if (i_vector->is_set()) {
      i_vector->resize(interpolated_size);
   } else {
      *i_vector = gsl::vector(interpolated_size);
   }

	/* Read values to array */
	double *x = new double[len];
	double *y = new double[len];
	size_t i;
	for(i=0; i<len; i++) {
		x[i] = i;
		y[i] = vector(i);
	}
	gsl_interp_accel *acc = gsl_interp_accel_alloc();
	gsl_spline *spline = gsl_spline_alloc(gsl_interp_linear, len);
	gsl_spline_init(spline, x, y, len);
	double xi;
    i = 0;

    xi = x[0];
    while(i<interpolated_size) {
    	(*i_vector)(i) = gsl_spline_eval(spline, xi, acc);
    	xi += (len-1)/(double)(interpolated_size-1);
    	if(xi > len-1)
    		xi = len-1;
    	i++;
    }

   /* Free memory */
   gsl_spline_free(spline);
	gsl_interp_accel_free(acc);
   delete[] x;
	delete[] y;
}

/** Interp1
 *
 */
void InterpolateLinear(const gsl::vector &x_orig, const gsl::vector &y_orig, const gsl::vector &x_interp, gsl::vector *y_interp) {
   size_t len = x_orig.size();
	size_t interpolated_size = x_interp.size();
	*y_interp = gsl::vector(interpolated_size);



	/* Read values to array */
	//double x[len];
	//double y[len];
	double *x = new double[len];
	double *y = new double[len];
	size_t i;
	for(i=0; i<len; i++) {
		x[i] = x_orig(i);
		y[i] = y_orig(i);
	}
	gsl_interp_accel *acc = gsl_interp_accel_alloc();
	gsl_spline *spline = gsl_spline_alloc(gsl_interp_linear, len);
	gsl_spline_init(spline, x, y, len);
	double xi;
    i = 0;


   for(i=0; i<interpolated_size; i++) {
      xi = x_interp(i);
      if (xi > x_orig(x_orig.size()-1))
         (*y_interp)(i) = y_orig(y_orig.size()-1);
      else if  (xi < x_orig(0))
         (*y_interp)(i) = y_orig(0);
      else
         (*y_interp)(i) = gsl_spline_eval(spline, xi, acc);
   }

    /* Free memory */
   gsl_spline_free(spline);
	gsl_interp_accel_free(acc);
	delete[] x;
	delete[] y;
}



void InterpolateSpline(const gsl::vector &vector, const size_t interpolated_size, gsl::vector *i_vector) {

   size_t len = vector.size();
   if (i_vector->is_set()) {
      i_vector->resize(interpolated_size);
   } else {
      *i_vector = gsl::vector(interpolated_size);
   }

   /* Read values to array */
	double *x = new double[len];
	double *y = new double[len];
   size_t i;
   for(i=0; i<len; i++) {
      x[i] = i;
      y[i] = vector(i);
   }
   gsl_interp_accel *acc = gsl_interp_accel_alloc();
   gsl_spline *spline = gsl_spline_alloc(gsl_interp_cspline,len);
   gsl_spline_init(spline, x, y, len);
   double xi;
   i = 0;

   xi = x[0];
   while(i<len) {
      (*i_vector)(i) = gsl_spline_eval(spline, xi, acc);
      xi += (len-1)/(double)(len-1);
      if(xi > len-1)
         xi = len-1;
      i++;
   }

   /* Free memory */
   gsl_spline_free(spline);
   gsl_interp_accel_free(acc);
   delete[] x;
   delete[] y;
}

/**
 * Function Interpolate
 *
 * Interpolates given vector to new vector of given length
 *
 * @param vector original vector
 * @param i_vector interpolated vector
 */
void Interpolate(const gsl::vector &vector, gsl::vector *i_vector) {

   int i,len = vector.size(),length = i_vector->size();

   /* Read values to array */
   double x[len];
   double y[len];
   for(i=0; i<len; i++) {
      x[i] = i;
      y[i] = vector(i);
   }
   gsl_interp_accel *acc = gsl_interp_accel_alloc();
    gsl_spline *spline = gsl_spline_alloc(gsl_interp_cspline,len);
    gsl_spline_init(spline, x, y, len);
    double xi;
    i = 0;

    /* New implementation (27.3.2009, bug fix 8.2.2010) */
    /* Bug fix to GSL v.1.15, 26.1.2012 */
    xi = x[0];
    while(i<length) {
      (*i_vector)(i) = gsl_spline_eval(spline, xi, acc);
      xi += (len-1)/(double)(length-1);
      if(xi > len-1)
         xi = len-1;
      i++;
    }

    /* Free memory */
    gsl_spline_free(spline);
   gsl_interp_accel_free(acc);
}

void InterpolateNearest(const gsl::vector &vector, const size_t interpolated_size, gsl::vector *i_vector) {
	if(i_vector->is_set()) {
		if(i_vector->size() != interpolated_size) {
			i_vector->resize(interpolated_size);
		} else {
			i_vector->set_all(0.0);
		}
	} else {
		*i_vector = gsl::vector(interpolated_size);
	}
	int i,j;
	for (i=0;i<(int)i_vector->size();i++){
		j = rint((double)i/(double)(vector.size()-1)*(double)(i_vector->size()-1));
		j = GSL_MAX(0,j);
		j = GSL_MIN(j,(int)vector.size()-1);
		(*i_vector)(i) = vector(j);
	}
}

void ApplyWindowingFunction(const WindowingFunctionType &window_function, gsl::vector *frame) {
	size_t i;
	double n = (double)frame->size();
	switch(window_function) {
	case HANN :
		for(i=0;i<frame->size();i++)
			(*frame)(i) *= 0.5*(1.0-cos(2.0*M_PI*((double)i)/((n)-1.0)));
		break;
	case HAMMING :
		for(i=0;i<frame->size();i++)
			(*frame)(i) *= 0.53836 - 0.46164*(cos(2.0*M_PI*((double)i)/((n)-1.0)));
		break;
	case BLACKMAN :
		for(i=0;i<frame->size();i++)
			(*frame)(i) *= 0.42-0.5*cos(2.0*M_PI*((double)i)/((n)-1))+0.08*cos(4.0*M_PI*((double)i)/((n)-1.0));
		break;
	case COSINE :
		for(i=0;i<frame->size();i++)
			(*frame)(i) *= sqrt(0.5*(1.0-cos(2.0*M_PI*((double)i)/((n)-1.0))));
		break;
	}
}

void Autocorrelation(const gsl::vector &frame, const int &order, gsl::vector *r) {
	if(!r->is_set()) {
		*r = gsl::vector(order+1);
	} else {
		if((int)r->size() != order+1) {
			r->resize(order+1);
		} else {
			r->set_all(0.0);
		}
	}
	/* Autocorrelation sequence */
	int i,n;
	for(i=0; i<order+1;i++) {
		for(n=i; n<(int)frame.size(); n++) {
			(*r)(i) += frame(n)*frame(n-i);
		}
	}
}


void Levinson(const gsl::vector &r, gsl::vector *A) {
    size_t p = r.size()-1;
	if(!A->is_set()) {
		*A = gsl::vector(p+1);
	} else {
		if(A->size() != p+1) {
			A->resize(p+1);
		} else {
			A->set_all(0.0);
		}
	}

    gsl::vector tmp = gsl::vector(p+1,true);
    double e, ki;
    size_t i, j;

    /* Levinson-Durbin recursion for finding AR polynomial coefficients */
    e = r(0);
    (*A)(0) = 1.0;
    for(i = 1; i <= p; i++) {
        ki= 0.0;
        for(j = 1; j < i; j++) ki+= (*A)(j) * r(i-j);
        ki= (r(i) - ki) / e;
        (*A)(i) = ki;
        for(j = 1; j < i; j++) tmp(j) = (*A)(j) - ki * (*A)(i-j);
        for(j = 1; j < i; j++) (*A)(j) = tmp(j);
        e = (1 - ki* ki) * e;
    }
    (*A) *= -1.0; /* Invert coefficient signs */
    (*A)(0) = 1.0;
}


void AllPassDelay(const double &lambda, gsl::vector *signal) {
	double A[2] = {-lambda, 1.0};
	double B[2] = {0.0, lambda};

	int i,j;
	double sum;

	gsl::vector signal_orig(signal->size());
	signal_orig.copy(*signal);

	for(i=0;i<(int)signal->size();i++) {
		sum = 0.0;
		for(j=0;j<2;j++) {
			if((i-j) >= 0)
				sum += signal_orig(i-j)*A[j] + (*signal)(i-j)*B[j];
		}
		(*signal)(i) = sum;
	}
}

void FFTRadix2(const gsl::vector &x, size_t nfft, ComplexVector *X ) {
   size_t i;
   size_t N = x.size();
   //assert(IsPow2(nfft));
   if(!IsPow2(nfft))
      nfft = (size_t)NextPow2(nfft);


   if(X == NULL) {
      *X = ComplexVector(nfft/2+1);
   } else {
      X->resize(nfft/2+1);
   }

   /* Allocate space for FFT */
   double *data = (double *)calloc(nfft,sizeof(double));

   /* Calculate spectrum */
   for (i=0; i<N; i++)
      data[i] = x(i);
   gsl_fft_real_radix2_transform(data, 1, nfft);
   for(i=1; i<nfft/2; i++){
      X->setReal(i, data[i]);
      X->setImag(i, data[nfft-i]);
   }
   X->setReal(0, data[0]);
   X->setReal(nfft/2, data[nfft/2]);

   /* Free memory*/
   free(data);
}

void FFTRadix2(const gsl::vector &x, ComplexVector *X ) {
   size_t i;
   size_t N = x.size();
   size_t nfft = NextPow2(2*N);


   if(X == NULL) {
      *X = ComplexVector(nfft/2+1);
   } else {
      X->resize(nfft/2+1);
   }

   /* Allocate space for FFT */
   double *data = (double *)calloc(nfft,sizeof(double));

   /* Calculate spectrum */
   for (i=0; i<N; i++)
      data[i] = x(i);
   gsl_fft_real_radix2_transform(data, 1, nfft);
   for(i=1; i<nfft/2; i++){
      X->setReal(i, data[i]);
      X->setImag(i, data[nfft-i]);
   }
   X->setReal(0, data[0]);
   X->setReal(nfft/2, data[nfft/2]);

   /* Free memory*/
   free(data);
}

void IFFTRadix2(const ComplexVector &X, gsl::vector *x) {

   size_t nfft = 2*(X.getSize()-1);
   assert(IsPow2(nfft));

   if(!(x->is_set()))
      *x = gsl::vector(X.getSize());

   size_t N = x->size();

   double *data = (double*)calloc(nfft,sizeof(double)); /* complex, takes 2*nfft/2  values*/

   /* Inverse transform  */
   size_t i;
   for(i=1; i<nfft/2; i++){
      data[i] = X.getReal(i);
      data[nfft-i] = X.getImag(i);
   }
   data[0] = X.getReal(0);
   data[nfft/2] = X.getReal(nfft/2);
   gsl_fft_halfcomplex_radix2_inverse(data, 1, nfft);

   for (i=0;i<N;i++)
   {
      (*x)(i) = data[i];
   }

   /* Free memory*/
   free(data);
}

void FastAutocorr(const gsl::vector &x, gsl::vector *ac)
{
   size_t i;
   size_t N = x.size();
   size_t nfft = NextPow2(2*N-1);

   double *data = (double*)calloc(nfft,sizeof(double)); /* complex, takes 2*nfft/2  values*/
   gsl::vector X(nfft/2+1);

   /* Calculate power spectral density */
   for (i=0; i<x.size(); i++)
      data[i] = x(i);
   gsl_fft_real_radix2_transform(data, 1, nfft);
   for(i=1; i<nfft/2; i++) {
      X(i) = data[i]*data[i]+ data[nfft-i]*data[nfft-i];
   }
   X(0) = data[0]*data[0];
   X(nfft/2) = data[nfft/2]*data[nfft/2];

    /* Inverse transform PSD for autocorrelation */
    for (i=0;i<nfft/2+1;i++)
        data[i] = X(i);
    for (i=nfft/2+1;i<nfft;i++)
        data[i] = 0;
    gsl_fft_halfcomplex_radix2_inverse(data, 1, nfft);

    if (ac == NULL || !ac->is_set())
       *ac = gsl::vector(2*N-1);
    else
       ac->resize(2*N-1);

    /* Set values symmetrically to AC vector */
    for (i=0;i<ac->size();i++) {
      if (i<N)
         (*ac)(i) = data[N-i-1];
      else
         (*ac)(i) = data[i-N+1];
    }

    /* Free memory*/
    free(data);
}


/**
 * Find the next integer that is a power of 2
 * @param n
 * @return
 */
int NextPow2(int n) {
   int m = 1;
   while (m < n)
      m*=2;
   return m;
}

bool IsPow2(int n) {
   while (n > 2) {
      if (n % 2 == 0)
         n = n/2;
      else
         return false;
   }
   return true;
}


void ConcatenateFrames(const gsl::vector &frame1, const gsl::vector &frame2, gsl::vector *frame_result) {
   size_t n1 = frame1.size();
   size_t n2 = frame2.size();
   if(!frame_result->is_set()) {
		*frame_result = gsl::vector(n1+n2);
	} else {
		if(frame_result->size() != n1+n2) {
			frame_result->resize(n1+n2);
		}
	}
	size_t i;
	for(i=0;i<n1;i++)
      (*frame_result)(i) = frame1(i);

   for(i=0;i<n2;i++)
      (*frame_result)(i+n1) = frame2(i);
}

/**
 * Function WFilter
 *
 * Warped FIR/IIR filter.
 *
 * This function is more or less adapted from WarpTB (MATLAB toolbox for frequency-warped signal processing)
 * authored by Aki H\E4rm\E4 and Matti Karjalainen.
 *
 * @param signal signal vector with preframe
 * @param A filter numerator
 * @param B filter denominator
 * @param lambda warping parameter
 *
 */
void WFilter(const gsl::vector &A, const gsl::vector &B,const gsl::vector &signal,const double &lambda, gsl::vector *result) {

	int i,q,mlen;
    size_t o;
    double xr,x,ffr,tmpr,Bb;

    size_t len = signal.size();
    int adim = (int)A.size();
    int bdim = (int)B.size();


   if(!result->is_set())
		*result = gsl::vector(len);



    double *Ar = (double *)calloc(adim,sizeof(double));
    double *Br = (double *)calloc(bdim,sizeof(double));
    double *ynr = (double *)calloc(signal.size(),sizeof(double));
    double *rsignal = (double *)calloc(signal.size(),sizeof(double));
    double *rmem = (double *)calloc(GSL_MAX(adim,bdim)+2,sizeof(double));
   double *sigma = (double *)calloc(sizeof(double),bdim+2);;
    /* Set signal to array */
    for(i=0;i<(int)len;i++) {
		rsignal[i] = signal(i);
	}

    /* Set A and B to arrays */
	for(i=0;i<adim;i++) {
		Ar[i] = A(i);
	}
	for(i=0;i<bdim;i++) {
		Br[i] = B(i);
	}

    /* Initialize */
    WarpingAlphas2Sigmas(Br,sigma,lambda,bdim-1);
    if(adim >= bdim)
    	mlen = adim;
    else
    	mlen = bdim + 1;
    Bb = 1/Br[0];

    /* Warped filtering */
    for(o=0;o<len;o++) {

    	xr = rsignal[o]*Bb;

    	/* Update feedbackward sum */
    	for(q=0;q<bdim;q++) {
    		xr -= sigma[q]*rmem[q];
    	}
    	xr = xr/sigma[bdim];
    	x = xr*Ar[0];

    	/* Update inner states */
    	for(q=0;q<mlen;q++) {
    		tmpr = rmem[q] + lambda*(rmem[q+1] - xr);
    		rmem[q] = xr;
    		xr = tmpr;
    	}

    	/* Update feedforward sum */
    	for(q=0,ffr=0.0;q<adim-1;q++) {
    		ffr += Ar[q+1]*rmem[q+1];
    	}

       /* Update output */
       ynr[o] = x + ffr;
	}

    /* Set output to result */
    int order = (int)signal.size()-(int)result->size();
    for(i=order;i<(int)signal.size();i++) {
    	(*result)(i-order) = ynr[i];
    }

    /* Free memory */
	free(ynr);
	free(rsignal);
	free(Ar);
	free(Br);
	free(rmem);
	free(sigma);
}

void WarpingAlphas2Sigmas(double *alp, double *sigm, double lambda, int dim) {

	int q;
	double S=0,Sp;

	sigm[dim] = lambda*alp[dim]/alp[0];
	Sp = alp[dim]/alp[0];
	for(q=dim;q>1;q--) {
		S = alp[q-1]/alp[0] - lambda*Sp;
		sigm[q-1] = lambda*S + Sp;
		Sp = S;
	}
	sigm[0] = S;
	sigm[dim+1] = 1 - lambda*S;
}

void Roots(const gsl::vector &x, const size_t ncoef, ComplexVector *r) {

   if (r == NULL) {
      *r = ComplexVector(x.size());
   } else {
      r->resize(x.size());
   }

   /* Initialize root arrays */
    //size_t ncoef = x.size()-1; // no minus one?
    size_t nroots = ncoef-1;
    //double coeffs[ncoef];
    double *coeffs = new double[ncoef];

    /* Complex values require 2 x space*/
    double *roots = new double[2*nroots];
    //double roots[2*nroots];

    /* Copy coefficients to arrays */
    /* Copy coefficients to arrays */
    size_t i;
    for(i=0; i<ncoef; i++) {
       coeffs[i] = x(i);
    }

    /* Solve roots */
    gsl_poly_complex_workspace *w = gsl_poly_complex_workspace_alloc(ncoef);
    gsl_poly_complex_solve(coeffs, ncoef, w, roots);

    /* Roots are in complex conjugate pairs: [Re=x,Im=y,Re=x,Im=-y] */
    size_t stride = 2;
    size_t ind;
    for(i=0, ind=0; i<2*nroots; i+=stride, ind++) {
       r->setReal(ind, roots[i]);
       r->setImag(ind, roots[i+1]);
    }

    gsl_poly_complex_workspace_free(w);
    delete[] coeffs;
    delete[] roots;

}

void Roots(const gsl::vector &x, ComplexVector *r) {
   Roots(x, x.size(), r);
}

void Poly2Lsf(const gsl::vector &a, gsl::vector *lsf) {


   /* by ljuvela, based on traitio and matlab implementation of poly2lsf */
   size_t i,n;

   /* Count the number of nonzero elements in "a" */
   n = 0;
   for(i=0; i<a.size(); i++) {
      if(a(i) != 0.0) {
         n++;
      }
   }

   /* In case of only one non-zero element */
   if(n == 1) {
      for(i=0; i<lsf->size(); i++)
         (*lsf)(i) = (i+1)*M_PI/(double)(lsf->size()+1);
      return;
   }

   gsl::vector aa = gsl::vector(n+1,true);
   gsl::vector flip_aa = gsl::vector(n+1,true);
   gsl::vector p = gsl::vector(n+1,true);
   gsl::vector q = gsl::vector(n+1,true);

   /* Construct vectors aa=[a 0] and flip_aa=[0 flip(a)] */
   for(i=0; i<n; i++) {
      aa(i) = a(i);
      flip_aa(flip_aa.size()-1-i) = a(i);
   }

   /* Construct vectors p and q */
   for(i=0; i<n+1; i++) {
      p(i) = aa(i) + flip_aa(i);
      q(i) = aa(i) - flip_aa(i);
   }

   /* ljuvela: NOTE deconvolution of a with b is the same as filtering with: Filter(a,b) */
   /* Remove trivial roots */
   if((n+1)%2 == 0) {
      double y;
      /* Deconvolve p with [1 1] */
      y = 0;
      for(i=0; i<p.size(); i++) {
         p(i) = p(i)-y;
         y = p(i);
      }
      p(p.size()-1) = 0;
      /* Deconvolve q with [1 -1] */
      y = 0;
      for(i=0; i<q.size(); i++) {
         q(i) = q(i)+y;
         y = q(i);
      }
      q(q.size()-1) = 0;
   } else {
      double y;
      /* Deconvolve q with [1 1] */
      y = 0;
      for(i=0; i<q.size(); i++) {
         q(i) = q(i)-y;
         y = q(i);
      }
      /* Deconvolve q with [1 -1] */
      y = 0;
      for(i=0; i<q.size(); i++) {
         q(i) = q(i)+y;
         y = q(i);
      }
      q(q.size()-1) = 0.0;
      q(q.size()-2) = 0.0;
   }

   /* NOTE deconvolution leaves a trailing zero to vector */
   size_t nroots_p = p.size()-2;
   size_t nroots_q = q.size()-2;
   size_t lsf_size = (nroots_p+nroots_p)/2;
   size_t ind=0;
   //double lsf_double[lsf_size];
   double *lsf_double = new double[lsf_size];

   /* Solve roots of P and convert to angle */
   ComplexVector r_p;
   Roots(p, p.size()-1, &r_p); // ignore last coefficient (zero)
   /* skip odd roots (complex conjugates) */
   for(i=0; i<nroots_p; i+=2, ind++)
      lsf_double[ind] = r_p.getAng(i);

   /* Solve roots of Q and convert to angle */
   ComplexVector r_q;
   Roots(q, q.size()-1, &r_q); // ignore last coefficient (zero)
   for(i=0; i<nroots_q; i+=2, ind++)
      lsf_double[ind] = r_q.getAng(i);

   /* Sort and copy LSFs to gsl::vector LSF */
   gsl_sort(lsf_double, 1, (nroots_p+nroots_q)/2);
   for(i=0; i<lsf_size; i++)
      (*lsf)(i) = lsf_double[i];

   delete[] lsf_double;
}

void Poly2Lsf(const gsl::matrix &a_mat, gsl::matrix *lsf_mat) {

   if (lsf_mat == NULL) {
      *lsf_mat = gsl::matrix(a_mat.size1()-1, a_mat.size2());
   } else {
      // TODO: resize (not urgent, lsf_mat is correctly allocated)
   }

   size_t i;
   gsl::vector lsf(a_mat.size1()-1);
   for(i=0;i<a_mat.size2();i++) {
      Poly2Lsf(a_mat.get_col_vec(i), &lsf);
      lsf_mat->set_col_vec(i, lsf);
   }
}

/** Overlap-add a frame to the target vector position given by
 *  size_t center_index (center of the frame goes to center_index)
 *  author: @mairaksi
 */
void OverlapAdd(const gsl::vector &frame, const size_t center_index, gsl::vector *target) {
   //center index = frame_index*params.frame_shift , start_ind = frame_index*params.frame_shift - ((int)frame->size())/2 + i;
   // Frame must be HANN windowed beforehand!
   int start_ind = (int)center_index - ((int)frame.size())/2;
   int stop_ind = start_ind+frame.size()-1;

   if(start_ind < 0)
      start_ind = 0;

   if(stop_ind > (int)target->size())
      stop_ind = (int)target->size();

   int i;
   for(i=start_ind;i<stop_ind;i++) {
      (*target)(i) += frame(i-start_ind);
   }
}

/** Compute the mean value of a vector
 *  author: @mairaksi
 */
double getMean(const gsl::vector &vec) {
   size_t i;
   double sum = 0.0;
   for(i=0;i<vec.size();i++)
      sum += vec(i);

   return sum/(double)vec.size();
}

/** Compute the total energy of a vector (subtract mean)
 *  author: @mairaksi
 */
double getEnergy(const gsl::vector &vec) {
   double mean = getMean(vec);
   double sum = 0.0;
   size_t i;
   for(i=0;i<vec.size();i++)
      sum += (vec(i)-mean)*(vec(i)-mean);

   return sqrt(sum);
}

double LogEnergy2FrameEnergy(const double &log_energy, const size_t frame_size) {
   return (double)pow(10,log_energy/20.0)*(double)frame_size;
}

double FrameEnergy2LogEnergy(const double &frame_energy, const size_t frame_size) {
   return (double)20.0*log10(frame_energy/(double)frame_size);
}

/** Compute the skewness statistic of a vector
 *  author: @mairaksi
 */
double Skewness(const gsl::vector &data) {
	int i;
	int N = (int)data.size();
	double mu=getMean(data);

	double m3=0.0;
	for (i=0;i<N;i++)
		m3 += pow(data(i)-mu,3);
	m3 = m3/(double)N;

	double s3 = 0.0;
	for (i=0;i<N;i++)
		s3 += pow(data(i)-mu,2);
	s3 = s3/(double)(N-1);
	s3 = pow(s3,1.5);

	return m3/s3;
}

/** Compute vector mean of non-zero elements
 *  author: @mairaksi
 */
double getMeanF0(const gsl::vector &fundf) {
   size_t i;
   int n = 0;
   double sum = 0.0;

   for(i=0;i<fundf.size();i++) {
      if(fundf(i) != 0.0) {
         sum += fundf(i);
         n++;
      }
   }
   return sum/(double)GSL_MAX(n,1);
}

/** Find the peaks (indeces and values) of gsl::vector vec
 *  With amplitudes greater than threshold*max(abs(vec))
 *  author: @mairaksi
 */
int FindPeaks(const gsl::vector &vec, const double &threshold, gsl::vector_int *index, gsl::vector *value) {

   int i,ii;
   gsl::vector_int idxtemp(vec.size());
   gsl::vector valtemp(vec.size());
    /* find maximum of abs x */
   //int maxidx = 0;
   double xmax = 0.0;
    for(i=0;i<(int)vec.size();i++){
      if (vec(i) > xmax){
         xmax = vec(i);
     //    maxidx = i;
      }
      if (vec(i) < -1*xmax){
         xmax = -1*vec(i);
       //  maxidx = i;
      }
    }

    /* copy and differentiate */
   gsl::vector vec_diff(vec.size(),true);
   Filter(std::vector<double>{1.0, -1.0}, std::vector<double>{1.0},vec,&vec_diff);

   /* find peaks at diff zero crossings */
   for (i=1,ii=0;i<(int)vec_diff.size();i++){
      if (vec_diff(i)*vec_diff(i-1)<0 &&
            (vec(i) > threshold*xmax || vec(i) < -1.0*threshold*xmax)){
         idxtemp(ii) = i;
         valtemp(ii) = vec(i);
         ii++;
      }
   }

   if (ii){
      *(index) = gsl::vector_int(ii);
      *(value) = gsl::vector(ii);

      for(i=0;i<ii;i++) {
         (*index)(i) = idxtemp(i);
         (*value)(i) = valtemp(i);
      }
   } else {
      (*index) = gsl::vector_int();
      (*value) = gsl::vector();
   }
   return ii;
}

/** Find the indices of the harmonic peaks of a magnitude spectrum
 *  Authors: @mairaksi & @ljuvela
 */
gsl::vector_int FindHarmonicPeaks(const gsl::vector &fft_mag, const double &f0, const int &fs) {
   double HARMONIC_SEARCH_COEFF = 0.5;
   int MAX_HARMONICS = 300;
   int fft_length = ((int)fft_mag.size()-1)*2;
   int MIN_SEARCH_RANGE = rint(10/fs*fft_length); // 10 Hz minimum
   gsl::vector_int peak_inds_temp(MAX_HARMONICS);
   gsl::vector search_range_vector;


	int current_harmonic = 0, guess_index = 0, harmonic_search_range;
	int i,j;
	double guess_index_double, val;

	while(1) {
      /* Number of frequency bins/f0 * search coeff (0.5) * attenuation */
      val = (double)fft_length*HARMONIC_SEARCH_COEFF*f0/(double)fs * (double)(fft_mag.size()-1-guess_index)/(double)(fft_mag.size()-1);
		/* Define harmonics search range, decreasing to the higher frequencies */
		harmonic_search_range = (int)GSL_MAX(val,MIN_SEARCH_RANGE);

		/* Estimate the index of the i_th harmonic
		 * Use an iterative estimation based on earlier values */
		if(current_harmonic > 0) {
			guess_index_double = 0.0;
			for(j=0;j<current_harmonic;j++)
				guess_index_double += peak_inds_temp(j)/(j+1.0)*(current_harmonic+1.0); // f0 estimate based on previous harmonic
			guess_index = (int)GSL_MAX(guess_index_double/j - (harmonic_search_range-1)/2.0,0); // Shift index by half of search range size
		} else
			guess_index = (int)GSL_MAX(f0/(fs/(double)fft_length) - (harmonic_search_range-1)/2.0,0);

		/* Stop search if the end (minus safe limit) of the fft vector or the maximum number of harmonics is reached */
		if(guess_index + harmonic_search_range > (int)fft_mag.size()-1 || current_harmonic > MAX_HARMONICS-1) {
			break;
		}

		/* Find the maximum of the i_th harmonic */
		search_range_vector = gsl::vector(harmonic_search_range);
		for(j=0; j<harmonic_search_range; j++) {
				search_range_vector(j) = fft_mag(guess_index+j);
		}

		peak_inds_temp(current_harmonic) = guess_index + search_range_vector.max_index();
		current_harmonic++;
	}

   gsl::vector_int peak_inds(current_harmonic);
   for(i=0;i<current_harmonic;i++)
      peak_inds(i) = peak_inds_temp(i);

   return peak_inds;
}






/** Stabilize a polynomial by computing the FFT autocorrelation of
 *  its' inverse power spectrum and perfoming Levinson
 *  author: @mairaksi
 */
void StabilizePoly(const int &fft_length, gsl::vector *A) {

	ComplexVector a_fft;
	FFTRadix2(*A,(size_t)fft_length,&a_fft);

	gsl::vector a_mag = a_fft.getAbs();
	size_t i;
	double thresh = 0.0001;
	for(i=0;i<a_mag.size();i++) {
      a_mag(i) = 1.0/GSL_MAX(pow(a_mag(i),2),thresh);
	}

	a_fft.setAllImag(0.0);
	a_fft.setReal(a_mag);

	gsl::vector ac(A->size());
	IFFTRadix2(a_fft, &ac);

	Levinson(ac, A);

}


gsl::vector_int LinspaceInt(const int &start_val, const int &hop_val,const int &end_val) {
   int N = floor((end_val-start_val)/hop_val)+1;
   if(N < 1)
      return gsl::vector_int();

   gsl::vector_int vec(N);
   int i;
   for(i=0;i<N;i++)
      vec(i) = start_val+hop_val*i;

   return vec;
}

void Linear2Erb(const gsl::vector &linvec, const int &fs, gsl::vector *erbvec) {
	double SMALL_VALUE = 0.0001;
	int i,j,hnr_channels = erbvec->size();
   gsl::vector erb(linvec.size());
	gsl::vector erb_sum(hnr_channels);

	/* Evaluate ERB scale indices for vector */
	for(i=0;i<(int)linvec.size();i++)
		erb(i) = log10(0.00437*((double)i/((double)linvec.size()-1.0)*((double)fs/2.0))+1.0) / log10(0.00437*((double)fs/2.0)+1.0) * ((double)hnr_channels-SMALL_VALUE);
                                                                                          // Subtract SMALL_VALUE to keep erb indeces within range of hnr_channels

	/* Evaluate values according to ERB rate */
	for(i=0;i<(int)linvec.size();i++) {
		j = floor(erb(i));
		(*erbvec)(j) += linvec(i);
		erb_sum(j) += 1.0;
	}

	/* Average values */
	for(i=0;i<hnr_channels;i++)
		(*erbvec)(i) *= 1.0/GSL_MAX(erb_sum(i),1.0);
}

void MedianFilter(const gsl::vector &x, const size_t filterlen, gsl::vector *y) {


}

int GetFrame(const gsl::vector &signal, const int &frame_index, const int &frame_shift,gsl::vector *frame, gsl::vector *pre_frame) {
	int i, ind;
	/* Get samples to frame */
	if (frame != NULL) {
		for(i=0; i<(int)frame->size(); i++) {
			ind = frame_index*frame_shift - ((int)frame->size())/2 + i; // SPTK compatible, ljuvela
			if (ind >= 0 && ind < (int)signal.size()){
				(*frame)(i) = signal(ind);
			}
		}
	} else {
		return EXIT_FAILURE;
	}

	/* Get pre-frame samples for smooth filtering */
	if (pre_frame){
		for(i=0; i<(int)pre_frame->size(); i++) {
			ind = frame_index*frame_shift - (int)frame->size()/2+ i - pre_frame->size(); // SPTK compatible, ljuvela
			if(ind >= 0 && ind < (int)signal.size())
				(*pre_frame)(i) = signal(ind);

  		}
	}

	return EXIT_SUCCESS;
}

