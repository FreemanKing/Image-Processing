#include <iostream>
#include <vector>

#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/features2d/features2d.hpp"
#include "JDGSmooth.h"

using namespace std;
using namespace cv;

/// Global Variables
int DELAY_CAPTION = 1500;
int DELAY_BLUR = 100;
int MAX_KERNEL_LENGTH = 31;

Mat src; Mat dst;
char window_name[] = "Smoothing Demo";

/// Function headers
int display_caption(const char* caption);
int display_dst(int delay);
void testColum(Mat src);

/**
* function main
*/
int main(void)
{
	namedWindow(window_name, WINDOW_AUTOSIZE);

	/// Load the source image
	src = imread("E:/SampleImages/test.jpg", 1);
	testColum(src);
	if (display_caption("Original Image") != 0) { return 0; }

	dst = src.clone();
	if (display_dst(DELAY_CAPTION) != 0) { return 0; }


	/// Applying Homogeneous blur
	if (display_caption("Homogeneous Blur") != 0) { return 0; }

	{
		JDGblur(src, dst, Size(9, 9), Point(-1, -1));
		imshow("JDGblur", dst);
	}

	/// Wait until user press a key
	display_caption("End: Press a key!");

	waitKey(0);

	return 0;
}

void testColum(Mat src)
{
	std::cout << "jdgsrc.rows " << src.rows << std::endl;
	std::cout << "jdgsrc.cols " << src.cols << std::endl;
}

/**
* @function display_caption
*/
int display_caption(const char* caption)
{
	dst = Mat::zeros(src.size(), src.type());
	putText(dst, caption,
		Point(src.cols / 4, src.rows / 2),
		FONT_HERSHEY_COMPLEX, 1, Scalar(255, 255, 255));

	imshow(window_name, dst);
	int c = waitKey(DELAY_CAPTION);
	if (c >= 0) { return -1; }
	return 0;
}

/**
* @function display_dst
*/
int display_dst(int delay)
{
	imshow(window_name, dst);
	int c = waitKey(delay);
	if (c >= 0) { return -1; }
	return 0;
}
