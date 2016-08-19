/**
Generator of sub-pixel displacements
WARNING: HIGH RAM MEMORY USAGE FOR LARGE PRECISION!
@author Krzysztof Kotowski
*/
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>
#include <fstream>

using namespace cv;
using namespace std;

double inline sign(double x);
Point2f pabs(Point2f point);
Point2f addNoise(Mat& dst, double stdDev);
String getName(int number, const String& folder);

int main(int argc, char** argv)
{
	String folder = "images";
	String source = "img01.png";

	cout << "Speckle movement generator\n";

	Size dstSize = Size(512, 512); //size of images to generate
	int moves = 100; //number of images (moves) to generate
	Point2f direction = Point2f(10, 10); //starting direction
	Point2f maxDev = Point2f(2, 2); //maximum deviation from direction
	double angle = 0;
	double angleVar = 0;
	double noise = 0;
	double precision = 0.1;

	/**
	* USER INTERFACE
	*
	*/
	if (argc > 2) //arguments from command window
	{
		if (argc > 1)  source = argv[1];
		if (argc > 2)  folder = argv[2];
		if (argc > 3)  dstSize = Size(stoi(argv[3]), stoi(argv[3]));
		if (argc > 4)  moves = stoi(argv[4]);
		if (argc > 5)  direction = Point2f(stod(argv[5]), stod(argv[5]));
		if (argc > 6)  maxDev = Point2f(stod(argv[6]), stod(argv[6]));
		if (argc > 7)  angle = stod(argv[7]);
		if (argc > 8)  noise = stod(argv[8]);
		if (argc > 9)  precision = stod(argv[9]);
	}
	else
	{
		cout << "Usage : %s <image> <dst_folder> <dst_size> <moves> <direction> <deviation> <angle> <noise> <precision> <\n", argv[0];
		cout << "Using default parameters only\n";
	}


	//base image for generator
	Mat image = imread(source, IMREAD_GRAYSCALE);
	//output for ground truth file
	ofstream movesFile(folder + "\\moves.txt");

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
	resize(image, bigger, Size(), scale, scale, INTER_NEAREST);
	Rect mBiggerROI = Rect((bigger.size() - dstSize * scale) / 2, dstSize * scale); //ROI in the centre
	Point intDirection = Point(direction * scale);
	Point intMaxDev = Point(pabs(maxDev) * scale);

	RNG rng(0xFFFFFF);
	Point position = Point(0, 0);
	double rotation = 0;

	Mat dst;
	Mat rotatedBigger = bigger.clone();
	Point2f sumMeanDev(0);
	for (int i = 0; i <= moves; i++)
	{
		if (i != 0)
		{
			//get next translation vector
			Point move(intDirection.x + rng.uniform(-intMaxDev.x, intMaxDev.x), intDirection.y + rng.uniform(-intMaxDev.y, intMaxDev.y));
			move.y = -move.y; //invert Y axis for Euclidean move

			/*
			TRANSLATION
			*/
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


			/*
			ROTATION
			*/
			if (abs(angle) > FLT_EPSILON)
			{
				Point2f pc(bigger.cols / 2.0 + position.x, bigger.rows / 2.0 - position.y);
				rotation += angle + rng.uniform(-angleVar, angleVar);
				Mat r = getRotationMatrix2D(pc, rotation, 1.0);
				warpAffine(bigger, rotatedBigger, r, bigger.size());
			}
			else
				bigger.copyTo(rotatedBigger);

			/*
			OUTPUT TO FILE AND SCREEN
			*/
			movesFile << position.x * precision << "," << position.y * precision << "," << rotation << endl;
			cout << i << ". " << position.x * precision << "," << position.y * precision << "," << rotation << endl;
		}

		/*
		SHRINKING
		*/
		resize(rotatedBigger(mBiggerROI), dst, dstSize, 0., 0., INTER_AREA);

		/*
		NOISE
		*/
		if (noise > DBL_EPSILON)
		{
			sumMeanDev += addNoise(dst, noise);
		}

		imwrite(getName(i, folder), dst);
	}
	movesFile.close();
	bigger.release();
	rotatedBigger.release();
	dst.release();

	return EXIT_SUCCESS;
}


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

Point2f addNoise(Mat& dst, double stdDev)
{
	Mat noise(dst.size(), CV_8SC1);

	//generate normal distributed noise using default random generator
	theRNG().state = getTickCount();
	randn(noise, 0, stdDev);

	Scalar mean, dev;
	meanStdDev(noise, mean, dev);

	//add noise to input without overflows - the fastest way
	Mat output = dst.clone();
	uchar *pInput, *pOutput;
	char *pNoise;
	for (int i = 0; i < dst.rows; i++)
	{
		pInput = dst.ptr<uchar>(i);
		pOutput = output.ptr<uchar>(i);
		pNoise = noise.ptr<char>(i);
		for (int j = 0; j < dst.cols; j++)
		{
			char change = pNoise[j];
			if (change != 0)
			{
				int out = pInput[j] + (int)change;
				pOutput[j] = uchar(out > 255 ? 255 : out < 0 ? 0 : out); //clamp to (0,255)
			}
		}
	}

	output.copyTo(dst);
	return Point2f(mean[0], dev[0]);
}

String getName(int number, const String& folder)
{
	String name = folder + "\\img";
	for (int dec = 10; dec <= pow(10, 3); dec *= 10)
	{
		if (number < dec) name += "0";
	}
	name += to_string(number);
	name += ".png";
	return name;
}

