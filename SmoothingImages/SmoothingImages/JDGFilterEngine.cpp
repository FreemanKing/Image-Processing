#include "JDGFilterEngine.h"


JDGFilterEngine::JDGFilterEngine()
{
	srcType = dstType = bufType = -1;
	rowBorderType = columnBorderType = BORDER_REPLICATE;
	bufStep = startY = startY0 = endY = rowCount = dstY = 0;
	maxWidth = 0;

	wholeSize = Size(-1, -1);
}

JDGFilterEngine::JDGFilterEngine(const Ptr<JDGBaseFilter>& _filter2D,
	const Ptr<JDGBaseRowFilter>& _rowFilter,
	const Ptr<JDGBaseColumnFilter>& _columnFilter,
	int _srcType, int _dstType, int _bufType,
	int _rowBorderType, int _columnBorderType,
	const Scalar& _borderValue)
{
	JDGinit(_filter2D, _rowFilter, _columnFilter, _srcType, _dstType, _bufType,
		_rowBorderType, _columnBorderType, _borderValue);
}

JDGFilterEngine::~JDGFilterEngine()
{

}


void JDGFilterEngine::JDGinit(const Ptr<JDGBaseFilter>& _filter2D,
	const Ptr<JDGBaseRowFilter>& _rowFilter,
	const Ptr<JDGBaseColumnFilter>& _columnFilter,
	int _srcType, int _dstType, int _bufType,
	int _rowBorderType, int _columnBorderType,
	const Scalar& _borderValue)
{
	_srcType = CV_MAT_TYPE(_srcType);
	_bufType = CV_MAT_TYPE(_bufType);
	_dstType = CV_MAT_TYPE(_dstType);
std:cout << "JDGinit " << std::endl; //run here

	srcType = _srcType;
	int srcElemSize = (int)getElemSize(srcType);
	dstType = _dstType;
	bufType = _bufType;

	filter2D = _filter2D;
	rowFilter = _rowFilter;
	columnFilter = _columnFilter;

	if (_columnBorderType < 0)
		_columnBorderType = _rowBorderType;

	rowBorderType = _rowBorderType;
	columnBorderType = _columnBorderType;

	CV_Assert(columnBorderType != BORDER_WRAP);

	if (isSeparable())
	{
		CV_Assert(!rowFilter.empty() && !columnFilter.empty());
		ksize = Size(rowFilter->ksize, columnFilter->ksize);
		anchor = Point(rowFilter->anchor, columnFilter->anchor);
	}
	else
	{
		CV_Assert(bufType == srcType);
		ksize = filter2D->ksize;
		anchor = filter2D->anchor;
	}

	CV_Assert(0 <= anchor.x && anchor.x < ksize.width &&
		0 <= anchor.y && anchor.y < ksize.height);

	borderElemSize = srcElemSize / (CV_MAT_DEPTH(srcType) >= CV_32S ? sizeof(int) : 1);
	int borderLength = std::max(ksize.width - 1, 1);
	borderTab.resize(borderLength*borderElemSize);

	maxWidth = bufStep = 0;
	constBorderRow.clear();

	if (rowBorderType == BORDER_CONSTANT || columnBorderType == BORDER_CONSTANT)
	{
		constBorderValue.resize(srcElemSize*borderLength);
		int srcType1 = CV_MAKETYPE(CV_MAT_DEPTH(srcType), MIN(CV_MAT_CN(srcType), 4));
		scalarToRawData(_borderValue, &constBorderValue[0], srcType1,
			borderLength*CV_MAT_CN(srcType));
	}

	wholeSize = Size(-1, -1);
}

int JDGFilterEngine::jdgstart(Size _wholeSize, Rect _roi, int _maxBufRows)
{
	std:cout << "jdgstart " << std::endl; //run here
	int i, j;

	wholeSize = _wholeSize;
	roi = _roi;
	CV_Assert(roi.x >= 0 && roi.y >= 0 && roi.width >= 0 && roi.height >= 0 &&
		roi.x + roi.width <= wholeSize.width &&
		roi.y + roi.height <= wholeSize.height);

	int esz = (int)getElemSize(srcType);
	int bufElemSize = (int)getElemSize(bufType);
	const uchar* constVal = !constBorderValue.empty() ? &constBorderValue[0] : 0;

	if (_maxBufRows < 0)
		_maxBufRows = ksize.height + 3;
	_maxBufRows = std::max(_maxBufRows, std::max(anchor.y, ksize.height - anchor.y - 1) * 2 + 1);

	if (maxWidth < roi.width || _maxBufRows != (int)rows.size())
	{
		rows.resize(_maxBufRows);
		maxWidth = std::max(maxWidth, roi.width);
		int cn = CV_MAT_CN(srcType);
		srcRow.resize(esz*(maxWidth + ksize.width - 1));
		if (columnBorderType == BORDER_CONSTANT)
		{
			constBorderRow.resize(getElemSize(bufType)*(maxWidth + ksize.width - 1 + VEC_ALIGN));
			uchar *dst = alignPtr(&constBorderRow[0], VEC_ALIGN), *tdst;
			int n = (int)constBorderValue.size(), N;
			N = (maxWidth + ksize.width - 1)*esz;
			tdst = isSeparable() ? &srcRow[0] : dst;

			for (i = 0; i < N; i += n)
			{
				n = std::min(n, N - i);
				for (j = 0; j < n; j++)
					tdst[i + j] = constVal[j];
			}

			if (isSeparable())
				(*rowFilter)(&srcRow[0], dst, maxWidth, cn);
		}

		int maxBufStep = bufElemSize*(int)alignSize(maxWidth +
			(!isSeparable() ? ksize.width - 1 : 0), VEC_ALIGN);
		ringBuf.resize(maxBufStep*rows.size() + VEC_ALIGN);
	}

	// adjust bufstep so that the used part of the ring buffer stays compact in memory
	bufStep = bufElemSize*(int)alignSize(roi.width + (!isSeparable() ? ksize.width - 1 : 0), 16);

	dx1 = std::max(anchor.x - roi.x, 0);
	dx2 = std::max(ksize.width - anchor.x - 1 + roi.x + roi.width - wholeSize.width, 0);

	// recompute border tables
	if (dx1 > 0 || dx2 > 0)
	{
		if (rowBorderType == BORDER_CONSTANT)
		{
			int nr = isSeparable() ? 1 : (int)rows.size();
			for (i = 0; i < nr; i++)
			{
				uchar* dst = isSeparable() ? &srcRow[0] : alignPtr(&ringBuf[0], VEC_ALIGN) + bufStep*i;
				memcpy(dst, constVal, dx1*esz);
				memcpy(dst + (roi.width + ksize.width - 1 - dx2)*esz, constVal, dx2*esz);
			}
		}
		else
		{
			int xofs1 = std::min(roi.x, anchor.x) - roi.x;

			int btab_esz = borderElemSize, wholeWidth = wholeSize.width;
			int* btab = (int*)&borderTab[0];

			for (i = 0; i < dx1; i++)
			{
				int p0 = (borderInterpolate(i - dx1, wholeWidth, rowBorderType) + xofs1)*btab_esz;
				for (j = 0; j < btab_esz; j++)
					btab[i*btab_esz + j] = p0 + j;
			}

			for (i = 0; i < dx2; i++)
			{
				int p0 = (borderInterpolate(wholeWidth + i, wholeWidth, rowBorderType) + xofs1)*btab_esz;
				for (j = 0; j < btab_esz; j++)
					btab[(i + dx1)*btab_esz + j] = p0 + j;
			}
		}
	}

	rowCount = dstY = 0;
	startY = startY0 = std::max(roi.y - anchor.y, 0);
	endY = std::min(roi.y + roi.height + ksize.height - anchor.y - 1, wholeSize.height);
	if (!columnFilter.empty())
		columnFilter->reset();
	if (!filter2D.empty())
		filter2D->reset();

	return startY;
}



int JDGFilterEngine::JDGstart(const Mat& src, const Rect& _srcRoi,
	bool isolated, int maxBufRows)
{
	Rect srcRoi = _srcRoi;

	if (srcRoi == Rect(0, 0, -1, -1))
		srcRoi = Rect(0, 0, src.cols, src.rows);

	CV_Assert(srcRoi.x >= 0 && srcRoi.y >= 0 &&
		srcRoi.width >= 0 && srcRoi.height >= 0 &&
		srcRoi.x + srcRoi.width <= src.cols &&
		srcRoi.y + srcRoi.height <= src.rows);

	Point ofs;
	Size wsz(src.cols, src.rows);
	if (!isolated)
		src.locateROI(wsz, ofs);
	jdgstart(wsz, srcRoi + ofs, maxBufRows);

	return startY - ofs.y;
}


int JDGFilterEngine::JDGproceed(const uchar* src, int srcstep, int count,
	uchar* dst, int dststep)
{
	CV_Assert(wholeSize.width > 0 && wholeSize.height > 0);

	const int *btab = &borderTab[0];
	int esz = (int)getElemSize(srcType), btab_esz = borderElemSize;
	uchar** brows = &rows[0];
	int bufRows = (int)rows.size();
	int cn = CV_MAT_CN(bufType);
	int width = roi.width, kwidth = ksize.width;
	int kheight = ksize.height, ay = anchor.y;
	int _dx1 = dx1, _dx2 = dx2;
	int width1 = roi.width + kwidth - 1;
	int xofs1 = std::min(roi.x, anchor.x);
	bool isSep = isSeparable();
	bool makeBorder = (_dx1 > 0 || _dx2 > 0) && rowBorderType != BORDER_CONSTANT;
	int dy = 0, i = 0;

	src -= xofs1*esz;
	count = std::min(count, JDGremainingInputRows());

	CV_Assert(src && dst && count > 0);

	for (;; dst += dststep*i, dy += i)
	{
		int dcount = bufRows - ay - startY - rowCount + roi.y;
		dcount = dcount > 0 ? dcount : bufRows - kheight + 1;
		dcount = std::min(dcount, count);
		count -= dcount;
		for (; dcount-- > 0; src += srcstep)
		{
			int bi = (startY - startY0 + rowCount) % bufRows;
			uchar* brow = alignPtr(&ringBuf[0], VEC_ALIGN) + bi*bufStep;
			uchar* row = isSep ? &srcRow[0] : brow;

			if (++rowCount > bufRows)
			{
				--rowCount;
				++startY;
			}

			memcpy(row + _dx1*esz, src, (width1 - _dx2 - _dx1)*esz);

			if (makeBorder)
			{
				if (btab_esz*(int)sizeof(int) == esz)
				{
					const int* isrc = (const int*)src;
					int* irow = (int*)row;

					for (i = 0; i < _dx1*btab_esz; i++)
						irow[i] = isrc[btab[i]];
					for (i = 0; i < _dx2*btab_esz; i++)
						irow[i + (width1 - _dx2)*btab_esz] = isrc[btab[i + _dx1*btab_esz]];
				}
				else
				{
					for (i = 0; i < _dx1*esz; i++)
						row[i] = src[btab[i]];
					for (i = 0; i < _dx2*esz; i++)
						row[i + (width1 - _dx2)*esz] = src[btab[i + _dx1*esz]];
				}
			}

			if (isSep)
				(*rowFilter)(row, brow, width, CV_MAT_CN(srcType));
		}

		int max_i = std::min(bufRows, roi.height - (dstY + dy) + (kheight - 1));
		for (i = 0; i < max_i; i++)
		{
			int srcY = borderInterpolate(dstY + dy + i + roi.y - ay,
				wholeSize.height, columnBorderType);
			if (srcY < 0) // can happen only with constant border type
				brows[i] = alignPtr(&constBorderRow[0], VEC_ALIGN);
			else
			{
				CV_Assert(srcY >= startY);
				if (srcY >= startY + rowCount)
					break;
				int bi = (srcY - startY0) % bufRows;
				brows[i] = alignPtr(&ringBuf[0], VEC_ALIGN) + bi*bufStep;
			}
		}
		if (i < kheight)
			break;
		i -= kheight - 1;
		// i == 4
		// cn == 3
		// roi.width*cn == 1536
		if (isSeparable())
			(*columnFilter)((const uchar**)brows, dst, dststep, i, roi.width*cn);
		else
			(*filter2D)((const uchar**)brows, dst, dststep, i, roi.width, cn);
	}

	dstY += dy;
	CV_Assert(dstY <= roi.height);
	return dy;
}


void JDGFilterEngine::JDGapply(const Mat& src, Mat& dst,
	const Rect& _srcRoi, Point dstOfs, bool isolated)
{
	CV_Assert(src.type() == srcType && dst.type() == dstType);

	Rect srcRoi = _srcRoi;
	if (srcRoi == Rect(0, 0, -1, -1))
		srcRoi = Rect(0, 0, src.cols, src.rows);

	if (srcRoi.area() == 0)
		return;

	CV_Assert(dstOfs.x >= 0 && dstOfs.y >= 0 &&
		dstOfs.x + srcRoi.width <= dst.cols &&
		dstOfs.y + srcRoi.height <= dst.rows);

	int y = JDGstart(src, srcRoi, isolated);
	JDGproceed(src.data + y*src.step
		+ srcRoi.x*src.elemSize(),
		(int)src.step, endY - startY,
		dst.data + dstOfs.y*dst.step +
		dstOfs.x*dst.elemSize(), (int)dst.step);
}


int JDGFilterEngine::JDGremainingInputRows() const
{
	return endY - startY - rowCount;
}

int JDGFilterEngine::JDGremainingOutputRows() const
{
	return roi.height - dstY;
}
