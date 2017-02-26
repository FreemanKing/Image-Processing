#ifndef _JDGBASEROWFILTER_H
#define _JDGBASEROWFILTER_H

#include "opencv2\opencv.hpp"

using namespace cv;
using namespace std;

class CV_EXPORTS JDGBaseRowFilter
{
public:
	//! the default constructor
	JDGBaseRowFilter();
	//! the destructor
	virtual ~JDGBaseRowFilter();
	//! the filtering operator. Must be overridden in the derived classes. The horizontal border interpolation is done outside of the class.
	virtual void operator()(const uchar* src, uchar* dst,
		int width, int cn) = 0;
	int ksize, anchor;
};

#endif