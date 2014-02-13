//   ีอออออออออออออออออออออออออออออออออออออออออออธ
//   ณ CGI_IMGutil  Image Utility                ณ
//   ณ          Lettura e scrittura delle        ณ
//   ณ          immagini                         ณ
//   ณ                                           ณ
//   ณ             8 Maggio                      ณ
//   ณ             by Ferr… Art & Tecnology 1999 ณ
//   ิอออออออออออออออออออออออออออออออออออออออออออพ

#include "\ehtool\include\ehsw_i.h"
#include "\ehtool\imgutil.h"
#include "\ehtool\cpucontrol.h"
#include <math.h>

/*
FilteredResizeH(IMGHEADER *ImgSorg, 
				double subrange_left, 
				double subrange_width, 
                int target_width, 
				ResamplingFunction* func, 
				IScriptEnvironment* env )
{
	BITMAPINFOHEADER BHSorg=(BITMAPINFOHEADER *) &ImgSorg->bmiHeader;
	pattern_chroma = 0;
    original_width = BHSorg->biWidth;//_child->GetVideoInfo().width;
    if (target_width<4)
		PRG_end("Resize: Width must be bigger than or equal to 4.");
    pattern_luma = GetResamplingPatternRGB(original_width, subrange_left, subrange_width, target_width, func);
    vi.width = target_width;
}
*/

//#include "internal.h"

// Original value: 65536
// 2 bits sacrificed because of 16 bit signed MMX multiplication
const int FPScale = 16384; // fixed point scaler

// 09-14-2002 - Vlad59 - Lanczos3Resize - Constant added
#define M_PI 3.14159265358979323846

/*******************************************
   ***************************************
   **  Helper classes for resample.cpp  **
   ***************************************
 *******************************************/

class ResamplingFunction 
/**
  * Pure virtual base class for resampling functions
  */
{
public:
  virtual double f(double x) = 0;
  virtual double support() = 0;
};

class PointFilter : public ResamplingFunction 
/**
  * Nearest neighbour (point sampler), used in PointResize
 **/
{
public:
  double f(double x);  
  double support() { return 0.5; }  // 0.0 crashes it.
};


class TriangleFilter : public ResamplingFunction 
/**
  * Simple triangle filter, used in BilinearResize
 **/
{
public:
  double f(double x);  
  double support() { return 1.0; }
};


class MitchellNetravaliFilter : public ResamplingFunction 
/**
  * Mitchell-Netraveli filter, used in BicubicResize
 **/
{
public:
  MitchellNetravaliFilter(double b, double c);
  double f(double x);
  double support() { return 2.0; }

private:
  double p0,p2,p3,q0,q1,q2,q3;
};

// 09-14-2002 - Vlad59 - Lanczos3Resize
class Lanczos3Filter : public ResamplingFunction
/**
  * Lanczos3 filter, used in Lanczos3Resize
 **/
{
public:
	double f(double x);
	double support() { return 3.0; };

private:
	double sinc(double value);
};



/*******************************************
   ***************************************
   **  Helper classes for resample.cpp  **
   ***************************************
 *******************************************/

/***************************
 ***** Point filter *****
 **************************/

double PointFilter::f(double x)
{ 
  x = fabs(x);
  return (x<1.0) ? x : 0.0;  // Can somebody confirm this function?
}


/***************************
 ***** Triangle filter *****
 **************************/

double TriangleFilter::f(double x)
{  
  x = fabs(x);
  return (x<1.0) ? 1.0-x : 0.0;
}

/*********************************
 *** Mitchell-Netravali filter ***
 *********************************/
MitchellNetravaliFilter::MitchellNetravaliFilter (double b=1./3., double c=1./3.) 
{
  p0 = (   6. -  2.*b            ) / 6.;
  p2 = ( -18. + 12.*b +  6.*c    ) / 6.;
  p3 = (  12. -  9.*b -  6.*c    ) / 6.;
  q0 = (            8.*b + 24.*c ) / 6.;
  q1 = (         - 12.*b - 48.*c ) / 6.;
  q2 = (            6.*b + 30.*c ) / 6.;
  q3 = (      -     b -  6.*c    ) / 6.;
}

double MitchellNetravaliFilter::f (double x)
{
    x = fabs(x);
    return (x<1) ? (p0+x*x*(p2+x*p3)) : (x<2) ? (q0+x*(q1+x*(q2+x*q3))) : 0.0;
}


/***********************
 *** Lanczos3 filter ***
 ***********************/

double Lanczos3Filter::sinc(double value)
{
	if (value != 0.0)
	{
		value *= M_PI;
		return sin(value) / value;
	}
	else
	{
		return 1.0;
	}
}

double Lanczos3Filter::f(double value)
{
	if (value < 0.0)
	{
		value = -value;
	}
	if (value < 3.0)
	{
		return (sinc(value) * sinc(value / 3.0));
	}
	else
	{
		return 0.0;
	}
}


/******************************
 **** Resampling Patterns  ****
 *****************************/

static int* GetResamplingPatternRGB(int original_width, 
									double subrange_start, 
									double subrange_width,
									int target_width, 
									ResamplingFunction* func )
/** 
  * This function returns a resampling "program" which is interpreted by the 
  * FilteredResize filters.  It handles edge conditions so FilteredResize    
  * doesn't have to.  
 **/
{
  double scale = double(target_width) / subrange_width;
  double filter_step = min(scale, 1.0);
  double filter_support = func->support() / filter_step;
  int fir_filter_size = int(ceil(filter_support*2));
//  int * result = (int*) aligned_malloc((1 + target_width*(1+fir_filter_size)) * 4, 4);
  int * result = (int*) malloc((1 + target_width*(1+fir_filter_size)) * 4);
  int i;
  int * cur = result;
  *cur++ = fir_filter_size;

  double pos_step = subrange_width / target_width;
  // the following translates such that the image center remains fixed
  double pos = subrange_start + ((subrange_width - target_width) / (target_width*2));

  for (i=0; i<target_width; ++i) 
  {
	int start_pos;
    double total,total2;
    int end_pos = int(pos + filter_support);

    if (end_pos > original_width-1)  end_pos = original_width-1;
    
	start_pos = end_pos - fir_filter_size + 1;
    if (start_pos < 0) start_pos = 0;
    *cur++ = start_pos;
    
	// the following code ensures that the coefficients add to exactly FPScale
    total = 0.0;
    for (int j=0; j<fir_filter_size; ++j)
      total += func->f((start_pos+j - pos) * filter_step);
    total2 = 0.0;
    for (int k=0; k<fir_filter_size; ++k) 
		{
			double total3 = total2 + func->f((start_pos+k - pos) * filter_step) / total;
			*cur++ = int(total3*FPScale+0.5) - int(total2*FPScale+0.5);
			total2 = total3;
		}
    pos += pos_step;
  }
  return result;
}
