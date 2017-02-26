#ifndef _JDGBASEFILER_H
#define _JDGBASEFILER_H

#include "opencv2\opencv.hpp"

using namespace cv;
using namespace std;

class CV_EXPORTS JDGBaseFilter
{
public:
	//! the default constructor
	JDGBaseFilter();
	//! the destructor
	virtual ~JDGBaseFilter();
	//! the filtering operator. The horizontal and the vertical border interpolation is done outside of the class.
	virtual void operator()(const uchar** src, uchar* dst, int dststep,
		int dstcount, int width, int cn) = 0;
	//! resets the internal buffers, if any
	virtual void reset();
	Size ksize;
	Point anchor;
};

template<typename ST, class CastOp, class VecOp> struct Filter2D : public JDGBaseFilter
{
	typedef typename CastOp::type1 KT;
	typedef typename CastOp::rtype DT;

	Filter2D(const Mat& _kernel, Point _anchor,
		double _delta, const CastOp& _castOp = CastOp(),
		const VecOp& _vecOp = VecOp())
	{
		anchor = _anchor;
		ksize = _kernel.size();
		delta = saturate_cast<KT>(_delta);
		castOp0 = _castOp;
		vecOp = _vecOp;
		CV_Assert(_kernel.type() == DataType<KT>::type);
		preprocess2DKernel(_kernel, coords, coeffs);
		ptrs.resize(coords.size());
	}

	void operator()(const uchar** src, uchar* dst, int dststep, int count, int width, int cn)
	{
		KT _delta = delta;
		const Point* pt = &coords[0];
		const KT* kf = (const KT*)&coeffs[0];
		const ST** kp = (const ST**)&ptrs[0];
		int i, k, nz = (int)coords.size();
		CastOp castOp = castOp0;

		width *= cn;
		for (; count > 0; count--, dst += dststep, src++)
		{
			DT* D = (DT*)dst;

			for (k = 0; k < nz; k++)
				kp[k] = (const ST*)src[pt[k].y] + pt[k].x*cn;

			i = vecOp((const uchar**)kp, dst, width);
#if CV_ENABLE_UNROLLED
			for (; i <= width - 4; i += 4)
			{
				KT s0 = _delta, s1 = _delta, s2 = _delta, s3 = _delta;

				for (k = 0; k < nz; k++)
				{
					const ST* sptr = kp[k] + i;
					KT f = kf[k];
					s0 += f*sptr[0];
					s1 += f*sptr[1];
					s2 += f*sptr[2];
					s3 += f*sptr[3];
				}

				D[i] = castOp(s0); D[i + 1] = castOp(s1);
				D[i + 2] = castOp(s2); D[i + 3] = castOp(s3);
			}
#endif
			for (; i < width; i++)
			{
				KT s0 = _delta;
				for (k = 0; k < nz; k++)
					s0 += kf[k] * kp[k][i];
				D[i] = castOp(s0);
			}
		}
	}

	vector<Point> coords;
	vector<uchar> coeffs;
	vector<uchar*> ptrs;
	KT delta;
	CastOp castOp0;
	VecOp vecOp;
};

#endif

