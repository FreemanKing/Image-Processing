#include "JDGSmooth.h"

template<typename ST, typename T> struct ColumnSum : public JDGBaseColumnFilter
{
	ColumnSum(int _ksize, int _anchor, double _scale)
	{
		ksize = _ksize;
		anchor = _anchor;
		scale = _scale;
		sumCount = 0;
	}

	void reset() { sumCount = 0; }

	void operator()(const uchar** src, uchar* dst, int dststep, int count, int width)
	{
		int i;
		ST* SUM;
		bool haveScale = scale != 1;
		double _scale = scale;

		if (width != (int)sum.size())
		{
			sum.resize(width);
			sumCount = 0;
		}

		SUM = &sum[0];
		if (sumCount == 0)
		{
			for (i = 0; i < width; i++)
				SUM[i] = 0;
			for (; sumCount < ksize - 1; sumCount++, src++)
			{
				const ST* Sp = (const ST*)src[0];
				for (i = 0; i <= width - 2; i += 2)
				{
					ST s0 = SUM[i] + Sp[i], s1 = SUM[i + 1] + Sp[i + 1];
					SUM[i] = s0; SUM[i + 1] = s1;
				}

				for (; i < width; i++)
					SUM[i] += Sp[i];
			}
		}
		else
		{
			CV_Assert(sumCount == ksize - 1);
			src += ksize - 1;
		}

		for (; count--; src++)
		{
			const ST* Sp = (const ST*)src[0];
			const ST* Sm = (const ST*)src[1 - ksize];
			T* D = (T*)dst;
			if (haveScale)
			{
				for (i = 0; i <= width - 2; i += 2)
				{
					ST s0 = SUM[i] + Sp[i], s1 = SUM[i + 1] + Sp[i + 1];
					D[i] = saturate_cast<T>(s0*_scale);
					D[i + 1] = saturate_cast<T>(s1*_scale);
					s0 -= Sm[i]; s1 -= Sm[i + 1];
					SUM[i] = s0; SUM[i + 1] = s1;
				}

				for (; i < width; i++)
				{
					ST s0 = SUM[i] + Sp[i];
					D[i] = saturate_cast<T>(s0*_scale);
					SUM[i] = s0 - Sm[i];
				}
			}
			else
			{
				for (i = 0; i <= width - 2; i += 2)
				{
					ST s0 = SUM[i] + Sp[i], s1 = SUM[i + 1] + Sp[i + 1];
					D[i] = saturate_cast<T>(s0);
					D[i + 1] = saturate_cast<T>(s1);
					s0 -= Sm[i]; s1 -= Sm[i + 1];
					SUM[i] = s0; SUM[i + 1] = s1;
				}

				for (; i < width; i++)
				{
					ST s0 = SUM[i] + Sp[i];
					D[i] = saturate_cast<T>(s0);
					SUM[i] = s0 - Sm[i];
				}
			}
			dst += dststep;
		}
	}

	double scale;
	int sumCount;
	vector<ST> sum;
};

template<> struct ColumnSum<int, uchar> : public JDGBaseColumnFilter
{
	ColumnSum(int _ksize, int _anchor, double _scale)
	{
		ksize = _ksize;
		anchor = _anchor;
		scale = _scale;
		sumCount = 0;
	}

	void reset() { sumCount = 0; }

	void operator()(const uchar** src, uchar* dst, int dststep, int count, int width)
	{
		//std:cout << "operator 2 " << std::endl; //run here
		int i;
		int* SUM;
		bool haveScale = scale != 1;
		double _scale = scale;

		//#if CV_SSE2
		//		bool haveSSE2 = checkHardwareSupport(CV_CPU_SSE2);
		//#endif

		if (width != (int)sum.size())
		{
			sum.resize(width);
			sumCount = 0;
		}

		SUM = &sum[0];

		// ksize是不停的在变化：1，3，5，7。。。29
		//std:cout << "jdgksize 2 " << ksize << std::endl; //run here

		if (sumCount == 0)
		{
			memset((void*)SUM, 0, width*sizeof(int));
			for (; sumCount < ksize - 1; sumCount++, src++)
			{
				const int* Sp = (const int*)src[0];
				i = 0;

				for (; i < width; i++)
					SUM[i] += Sp[i];
			}
		}
		else
		{
			CV_Assert(sumCount == ksize - 1);
			src += ksize - 1;
		}

		// _scale == 0.12 if ksize==9
		//std:cout << "jdgdcount 2 " << count << std::endl; //run here

		for (; count--; src++)
		{
			const int* Sp = (const int*)src[0];
			const int* Sm = (const int*)src[1 - ksize];
			uchar* D = (uchar*)dst;
			if (haveScale)
			{
				i = 0;

				for (; i < width; i++)
				{
					int s0 = SUM[i] + Sp[i];
					D[i] = saturate_cast<uchar>(s0*_scale);
					SUM[i] = s0 - Sm[i];
				}
			}
			else
			{
				i = 0;

				for (; i < width; i++)
				{
					int s0 = SUM[i] + Sp[i];
					D[i] = saturate_cast<uchar>(s0);
					SUM[i] = s0 - Sm[i];
				}
			}
			dst += dststep;
		}
	}

	double scale;
	int sumCount;
	vector<int> sum;
};

template<> struct ColumnSum<int, short> : public JDGBaseColumnFilter
{
	ColumnSum(int _ksize, int _anchor, double _scale)
	{
		ksize = _ksize;
		anchor = _anchor;
		scale = _scale;
		sumCount = 0;
	}

	void reset() { sumCount = 0; }

	void operator()(const uchar** src, uchar* dst, int dststep, int count, int width)
	{
		int i;
		int* SUM;
		bool haveScale = scale != 1;
		double _scale = scale;

		if (width != (int)sum.size())
		{
			sum.resize(width);
			sumCount = 0;
		}
		SUM = &sum[0];
		if (sumCount == 0)
		{
			memset((void*)SUM, 0, width*sizeof(int));
			for (; sumCount < ksize - 1; sumCount++, src++)
			{
				const int* Sp = (const int*)src[0];
				i = 0;

				for (; i < width; i++)
					SUM[i] += Sp[i];
			}
		}
		else
		{
			CV_Assert(sumCount == ksize - 1);
			src += ksize - 1;
		}

		for (; count--; src++)
		{
			const int* Sp = (const int*)src[0];
			const int* Sm = (const int*)src[1 - ksize];
			short* D = (short*)dst;
			if (haveScale)
			{
				i = 0;

				for (; i < width; i++)
				{
					int s0 = SUM[i] + Sp[i];
					D[i] = saturate_cast<short>(s0*_scale);
					SUM[i] = s0 - Sm[i];
				}
			}
			else
			{
				i = 0;

				for (; i < width; i++)
				{
					int s0 = SUM[i] + Sp[i];
					D[i] = saturate_cast<short>(s0);
					SUM[i] = s0 - Sm[i];
				}
			}
			dst += dststep;
		}
	}

	double scale;
	int sumCount;
	vector<int> sum;
};


template<> struct ColumnSum<int, ushort> : public JDGBaseColumnFilter
{
	ColumnSum(int _ksize, int _anchor, double _scale)
	{
		ksize = _ksize;
		anchor = _anchor;
		scale = _scale;
		sumCount = 0;
	}

	void reset() { sumCount = 0; }

	void operator()(const uchar** src, uchar* dst, int dststep, int count, int width)
	{
		int i;
		int* SUM;
		bool haveScale = scale != 1;
		double _scale = scale;

		if (width != (int)sum.size())
		{
			sum.resize(width);
			sumCount = 0;
		}
		SUM = &sum[0];
		if (sumCount == 0)
		{
			memset((void*)SUM, 0, width*sizeof(int));
			for (; sumCount < ksize - 1; sumCount++, src++)
			{
				const int* Sp = (const int*)src[0];
				i = 0;

				for (; i < width; i++)
					SUM[i] += Sp[i];
			}
		}
		else
		{
			CV_Assert(sumCount == ksize - 1);
			src += ksize - 1;
		}

		for (; count--; src++)
		{
			const int* Sp = (const int*)src[0];
			const int* Sm = (const int*)src[1 - ksize];
			ushort* D = (ushort*)dst;
			if (haveScale)
			{
				i = 0;

				for (; i < width; i++)
				{
					int s0 = SUM[i] + Sp[i];
					D[i] = saturate_cast<ushort>(s0*_scale);
					SUM[i] = s0 - Sm[i];
				}
			}
			else
			{
				i = 0;

				for (; i < width; i++)
				{
					int s0 = SUM[i] + Sp[i];
					D[i] = saturate_cast<ushort>(s0);
					SUM[i] = s0 - Sm[i];
				}
			}
			dst += dststep;
		}
	}

	double scale;
	int sumCount;
	vector<int> sum;
};

Ptr<JDGBaseColumnFilter> JDGgetColumnSumFilter(int sumType, int dstType, int ksize,
	int anchor, double scale)
{
std:cout << "jdgscale " << scale << std::endl; //run here
	int sdepth = CV_MAT_DEPTH(sumType), ddepth = CV_MAT_DEPTH(dstType);
	CV_Assert(CV_MAT_CN(sumType) == CV_MAT_CN(dstType));

	if (anchor < 0)
		anchor = ksize / 2;

	if (ddepth == CV_8U && sdepth == CV_32S)
		return Ptr<JDGBaseColumnFilter>(new ColumnSum<int, uchar>(ksize, anchor, scale));
	if (ddepth == CV_8U && sdepth == CV_64F)
		return Ptr<JDGBaseColumnFilter>(new ColumnSum<double, uchar>(ksize, anchor, scale));
	if (ddepth == CV_16U && sdepth == CV_32S)
		return Ptr<JDGBaseColumnFilter>(new ColumnSum<int, ushort>(ksize, anchor, scale));
	if (ddepth == CV_16U && sdepth == CV_64F)
		return Ptr<JDGBaseColumnFilter>(new ColumnSum<double, ushort>(ksize, anchor, scale));
	if (ddepth == CV_16S && sdepth == CV_32S)
		return Ptr<JDGBaseColumnFilter>(new ColumnSum<int, short>(ksize, anchor, scale));
	if (ddepth == CV_16S && sdepth == CV_64F)
		return Ptr<JDGBaseColumnFilter>(new ColumnSum<double, short>(ksize, anchor, scale));
	if (ddepth == CV_32S && sdepth == CV_32S)
		return Ptr<JDGBaseColumnFilter>(new ColumnSum<int, int>(ksize, anchor, scale));
	if (ddepth == CV_32F && sdepth == CV_32S)
		return Ptr<JDGBaseColumnFilter>(new ColumnSum<int, float>(ksize, anchor, scale));
	if (ddepth == CV_32F && sdepth == CV_64F)
		return Ptr<JDGBaseColumnFilter>(new ColumnSum<double, float>(ksize, anchor, scale));
	if (ddepth == CV_64F && sdepth == CV_32S)
		return Ptr<JDGBaseColumnFilter>(new ColumnSum<int, double>(ksize, anchor, scale));
	if (ddepth == CV_64F && sdepth == CV_64F)
		return Ptr<JDGBaseColumnFilter>(new ColumnSum<double, double>(ksize, anchor, scale));

	CV_Error_(CV_StsNotImplemented,
		("Unsupported combination of sum format (=%d), and destination format (=%d)",
		sumType, dstType));

	return Ptr<JDGBaseColumnFilter>(0);
}

template<typename T, typename ST> struct RowSum : public JDGBaseRowFilter
{
	RowSum(int _ksize, int _anchor)
	{
		ksize = _ksize;
		anchor = _anchor;
	}

	void operator()(const uchar* src, uchar* dst, int width, int cn)
	{
		const T* S = (const T*)src;
		ST* D = (ST*)dst;
		int i = 0, k, ksz_cn = ksize*cn;

		width = (width - 1)*cn;
		for (k = 0; k < cn; k++, S++, D++)
		{
			ST s = 0;
			for (i = 0; i < ksz_cn; i += cn)
				s += S[i];
			D[0] = s;
			for (i = 0; i < width; i += cn)
			{
				s += S[i + ksz_cn] - S[i];
				D[i + cn] = s;
			}
		}
	}
};

Ptr<JDGBaseRowFilter> JDGgetRowSumFilter(int srcType, int sumType, int ksize, int anchor)
{
	int sdepth = CV_MAT_DEPTH(srcType), ddepth = CV_MAT_DEPTH(sumType);
	CV_Assert(CV_MAT_CN(sumType) == CV_MAT_CN(srcType));

	if (anchor < 0)
		anchor = ksize / 2;

	if (sdepth == CV_8U && ddepth == CV_32S)
		return Ptr<JDGBaseRowFilter>(new RowSum<uchar, int>(ksize, anchor));
	if (sdepth == CV_8U && ddepth == CV_64F)
		return Ptr<JDGBaseRowFilter>(new RowSum<uchar, double>(ksize, anchor));
	if (sdepth == CV_16U && ddepth == CV_32S)
		return Ptr<JDGBaseRowFilter>(new RowSum<ushort, int>(ksize, anchor));
	if (sdepth == CV_16U && ddepth == CV_64F)
		return Ptr<JDGBaseRowFilter>(new RowSum<ushort, double>(ksize, anchor));
	if (sdepth == CV_16S && ddepth == CV_32S)
		return Ptr<JDGBaseRowFilter>(new RowSum<short, int>(ksize, anchor));
	if (sdepth == CV_32S && ddepth == CV_32S)
		return Ptr<JDGBaseRowFilter>(new RowSum<int, int>(ksize, anchor));
	if (sdepth == CV_16S && ddepth == CV_64F)
		return Ptr<JDGBaseRowFilter>(new RowSum<short, double>(ksize, anchor));
	if (sdepth == CV_32F && ddepth == CV_64F)
		return Ptr<JDGBaseRowFilter>(new RowSum<float, double>(ksize, anchor));
	if (sdepth == CV_64F && ddepth == CV_64F)
		return Ptr<JDGBaseRowFilter>(new RowSum<double, double>(ksize, anchor));

	CV_Error_(CV_StsNotImplemented,
		("Unsupported combination of source format (=%d), and buffer format (=%d)",
		srcType, sumType));

	return Ptr<JDGBaseRowFilter>(0);
}

Ptr<JDGFilterEngine> JDGcreateBoxFilter(int srcType, int dstType, Size ksize,
	Point anchor, bool normalize, int borderType)
{
	int sdepth = CV_MAT_DEPTH(srcType);
	cout << "JDGsdepth" << sdepth << endl;

	int cn = CV_MAT_CN(srcType), sumType = CV_64F;
	if (sdepth <= CV_32S && (!normalize ||
		ksize.width*ksize.height <= (sdepth == CV_8U ? (1 << 23) :
		sdepth == CV_16U ? (1 << 15) : (1 << 16))))
		sumType = CV_32S;
	sumType = CV_MAKETYPE(sumType, cn);

	Ptr<JDGBaseRowFilter> rowFilter = JDGgetRowSumFilter(srcType, sumType, ksize.width, anchor.x);
	Ptr<JDGBaseColumnFilter> columnFilter = JDGgetColumnSumFilter(sumType,
		dstType, ksize.height, anchor.y, normalize ? 1. / (ksize.width*ksize.height) : 1);

	return Ptr<JDGFilterEngine>(new JDGFilterEngine(Ptr<JDGBaseFilter>(0), rowFilter, columnFilter,
		srcType, dstType, sumType, borderType));
}

void JDGboxFilter(InputArray _src, OutputArray _dst, int ddepth,
	Size ksize, Point anchor, bool normalize, int borderType)
{
	Mat src = _src.getMat();
	int sdepth = src.depth(), cn = src.channels();
	if (ddepth < 0)
		ddepth = sdepth;
	_dst.create(src.size(), CV_MAKETYPE(ddepth, cn));
	Mat dst = _dst.getMat();
	if (borderType != BORDER_CONSTANT && normalize && (borderType & BORDER_ISOLATED) != 0)
	{
		if (src.rows == 1)
			ksize.height = 1;
		if (src.cols == 1)
			ksize.width = 1;
	}

	//#ifdef HAVE_TEGRA_OPTIMIZATION
	//	if (tegra::box(src, dst, ksize, anchor, normalize, borderType))
	//		return;
	//#endif

	// src.type()== 16
	// ksize的值已经在函数JDGcreateBoxFilter中进行处理，使其成为kernel
	Ptr<JDGFilterEngine> f = JDGcreateBoxFilter(src.type(), dst.type(),
		ksize, anchor, normalize, borderType);
	//cout << "jdgfscale " << endl;
	f->JDGapply(src, dst);
}


void JDGblur(InputArray src, OutputArray dst,
	Size ksize, Point anchor, int borderType)
{
	JDGboxFilter(src, dst, -1, ksize, anchor, true, borderType);
}