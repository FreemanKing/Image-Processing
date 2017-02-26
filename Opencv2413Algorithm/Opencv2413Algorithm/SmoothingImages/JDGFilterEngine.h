#ifndef _JDGFILGERENGINE_H
#define _JDGFILGERENGINE_H

#include "opencv2\opencv.hpp"

#include "JDGBaseFilter.h"
#include "JDGBaseRowFilter.h"
#include "JDGBaseColumnFilter.h"

#define  CV_MALLOC_ALIGN    16

static const int VEC_ALIGN = CV_MALLOC_ALIGN;

class CV_EXPORTS JDGFilterEngine
{
public:
	//! the default constructor
	JDGFilterEngine();
	//! the full constructor. Either _filter2D or both _rowFilter and _columnFilter must be non-empty.
	JDGFilterEngine(const Ptr<JDGBaseFilter>& _filter2D,
		const Ptr<JDGBaseRowFilter>& _rowFilter,
		const Ptr<JDGBaseColumnFilter>& _columnFilter,
		int srcType, int dstType, int bufType,
		int _rowBorderType = BORDER_REPLICATE,
		int _columnBorderType = -1,
		const Scalar& _borderValue = Scalar());
	//! the destructor
	virtual ~JDGFilterEngine();
	//! reinitializes the engine. The previously assigned filters are released.
	void JDGinit(const Ptr<JDGBaseFilter>& _filter2D,
		const Ptr<JDGBaseRowFilter>& _rowFilter,
		const Ptr<JDGBaseColumnFilter>& _columnFilter,
		int srcType, int dstType, int bufType,
		int _rowBorderType = BORDER_REPLICATE, int _columnBorderType = -1,
		const Scalar& _borderValue = Scalar());
	//! starts filtering of the specified ROI of an image of size wholeSize.
	virtual int jdgstart(Size wholeSize, Rect roi, int maxBufRows = -1);
	//! starts filtering of the specified ROI of the specified image.
	virtual int JDGstart(const Mat& src, const Rect& srcRoi = Rect(0, 0, -1, -1),
		bool isolated = false, int maxBufRows = -1);
	//! processes the next srcCount rows of the image.
	virtual int JDGproceed(const uchar* src, int srcStep, int srcCount,
		uchar* dst, int dstStep);
	//! applies filter to the specified ROI of the image. if srcRoi=(0,0,-1,-1),
	//! the whole image is filtered.
	virtual void JDGapply(const Mat& src, Mat& dst,
		const Rect& srcRoi = Rect(0, 0, -1, -1),
		Point dstOfs = Point(0, 0),
		bool isolated = false);
	//! returns true if the filter is separable
	bool isSeparable() const { return (const JDGBaseFilter*)filter2D == 0; }
	//! returns the number
	int JDGremainingInputRows() const;
	int JDGremainingOutputRows() const;

	int srcType, dstType, bufType;
	Size ksize;
	Point anchor;
	int maxWidth;
	Size wholeSize;
	Rect roi;
	int dx1, dx2;
	int rowBorderType, columnBorderType;
	vector<int> borderTab;
	int borderElemSize;
	vector<uchar> ringBuf;
	vector<uchar> srcRow;
	vector<uchar> constBorderValue;
	vector<uchar> constBorderRow;
	int bufStep, startY, startY0, endY, rowCount, dstY;
	vector<uchar*> rows;

	Ptr<JDGBaseFilter> filter2D;
	Ptr<JDGBaseRowFilter> rowFilter;
	Ptr<JDGBaseColumnFilter> columnFilter;
};

#endif