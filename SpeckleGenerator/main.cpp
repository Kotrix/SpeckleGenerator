/**
Subpixel shifter
@author Krzysztof Kotowski
*/
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>
#include <fstream>

using namespace cv;
using namespace std;

double inline sign(double x)
{
	if (x < 0) return -1.0;
	if (x > 0) return 1.0;
	return 0.0;
}

Point2f pabs(Point2f point)
{
	return Point2f(fabs(point.x), fabs(point.y));
}

String getName(int number)
{
	String name = "images\\img";
	for (int dec = 10; dec <= pow(10, 3); dec *= 10)
	{
		if (number < dec) name += "0";
	}
	name += to_string(number);
	name += ".png";
	return name;
}

int main()
{
	cout << "Speckle movement generator\n";

	//base image for generator
	Mat image = imread("C:\\Users\\Krzysztof\\Pictures\\data\\img01.png", IMREAD_GRAYSCALE);
	//output for ground truth file
	ofstream movesFile("images\\moves.txt");

	Size dstSize = Size(512, 512); //size of images to generate
	int moves = 100; //number of images (moves) to generate
	Point2f direction = Point2f(0, -13); //starting direction
	Point2f maxDev = Point2f(0,0); //maximum deviation from direction
	double precision = 0.1;
	bool drawPath = false;

	//check if enough space left for movement
	Size emptySpace = image.size() - dstSize;
	Point2f maxShift = pabs(direction) + pabs(maxDev);
	if (emptySpace.width < maxShift.x || emptySpace.height < maxShift.y)
	{
		cout << "Too small source image for this translation" << endl;
		return EXIT_FAILURE;
	}

	//set precision
	if (precision > 1.0 || precision < 0.0)
	{
		cout << "Precision has to be positive and less than 1" << endl;
		return EXIT_FAILURE;
	}
	int scale = ceil(pow(precision, -1)); //calculate minimum up-scale needed to achieve precision
	precision = pow(scale, -1);
	movesFile << scale << endl;

	//up-scale to achieve given precision
	Mat bigger;
	resize(image, bigger, Size(), scale, scale, INTER_AREA);
	Rect mBiggerROI = Rect((bigger.size() - dstSize * scale) / 2, dstSize * scale); //ROI in the centre
	Point intDirection = Point(direction * scale);
	Point intMaxDev = Point(pabs(maxDev) * scale);

	RNG rng(0xFFFFFF);
	Point position = Point(0, 0);

	Mat dst;
	if(drawPath) namedWindow("path", WINDOW_NORMAL);

	for (int i = 0; i <= moves; i++)
	{
		if (i != 0)
		{
			//get next translation vector
			Point move(intDirection.x + rng.uniform(-intMaxDev.x, intMaxDev.x), intDirection.y + rng.uniform(-intMaxDev.y, intMaxDev.y));
			move.y = -move.y; //invert Y axis for Euclidean move
			Point nextROIpoint = mBiggerROI.tl() + move;

			//change direction if boundary approached
			if (nextROIpoint.x < 0 || nextROIpoint.x > bigger.cols - mBiggerROI.width)
			{
				intDirection.x = -intDirection.x;
				move.x = -move.x;
			}
			if (nextROIpoint.y < 0 || nextROIpoint.y > bigger.rows - mBiggerROI.height)
			{
				intDirection.y = -intDirection.y;
				move.y = -move.y;
			}

			mBiggerROI += move;
			move.y = -move.y; //re-invert Y axis for Euclidean move
			position += move;

			if (drawPath)
			{
				//draw and show path
				Mat pathImg;
				cvtColor(bigger, pathImg, CV_GRAY2BGR);
				//line(pathImg, mBiggerROI.tl() - move, mBiggerROI.tl(), Scalar(0, 255, 0), 2 * scale);
				rectangle(pathImg, mBiggerROI, Scalar(0, 0, 255), 2 * scale);
				imshow("path", pathImg);
				waitKey(1);
			}

			movesFile << position.x * precision << "," << position.y * precision << endl;
			cout << i << ". " << position.x * precision << "," << position.y * precision << endl;
		}

		resize(bigger(mBiggerROI), dst, dstSize, 0, 0, INTER_AREA);
		imwrite(getName(i), dst);
	}

	movesFile.close();
	return EXIT_SUCCESS;
}

