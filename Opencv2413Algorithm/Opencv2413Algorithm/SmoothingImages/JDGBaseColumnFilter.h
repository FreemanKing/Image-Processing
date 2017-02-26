#ifndef _JDGBASECOLUMNFILTER_H
#define _JDGBASECOLUMNFILTER_H

#include "opencv2\opencv.hpp"

using namespace cv;
using namespace std;

class CV_EXPORTS JDGBaseColumnFilter
{
public:
	//! the default constructor
	JDGBaseColumnFilter();
	//! the destructor
	virtual ~JDGBaseColumnFilter();
	//! the filtering operator. Must be overridden in the derived classes.
	//! The vertical border interpolation is done outside of the class.

	// To be overriden by the user.
	//
	// runs a filtering operation on the set of rows,
	// "dstcount + ksize - 1" rows on input,
	// "dstcount" rows on output,
	// each input and output row has "width" elements
	// the filtered rows are written into "dst" buffer.
	virtual void operator()(const uchar** src, uchar* dst, int dststep,
		int dstcount, int width) = 0;
	//! resets the internal buffers, if any
	virtual void reset();
	int ksize, anchor;
};

#endif