#ifndef _JDGSmooth_H
#define _JDGSmooth_H
#include <vector>
#include <iostream>

#include "opencv2\opencv.hpp"
#include "opencv2\imgproc\types_c.h"
#include "opencv2\imgproc\imgproc.hpp"

#include "JDGFilterEngine.h"

void JDGblur(InputArray src, OutputArray dst,
	Size ksize, Point anchor, int borderType = BORDER_DEFAULT);

#endif