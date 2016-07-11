#pragma once
#include "Method.h"
#include "PeakFinderFactory.h"
#include <iostream>

class TemplateMatching : public Method
{
protected:
	Ptr<PeakFinder> mPeakFinder;

	
	vector<Mat> generatePyramid(const Mat& img, const Rect& ROI) const
	{
		vector<Mat> result;
		result.push_back(img(ROI));
		Mat lower;
		for (int i = 1; i <= mLayers; i++)
		{
			double f = pow(2, -i);
			resize(img(ROI), lower, Size(), f, f);
			result.push_back(lower);
		}
		return result;
	}

public:
	explicit TemplateMatching(const Mat& first) : Method("TemplateMatching"), mTempRatio(0.5), mMaxShift(0.15), mLayers(0), mSegmentsVertically(1), mSegmentsHorizontally(1)
	{
		mPeakFinder = PeakFinderFactory::getPeakFinder(SPATIAL, SAD);

		int width = first.cols;
		int height = first.rows;

		//calculate ROI for template
		double borderRatio = (1.0 - mTempRatio) * 0.5;
		Point tl = Point(floor(width * borderRatio), floor(height * borderRatio));
		Point br = Point(ceil(width * (1.0 - borderRatio)), ceil(height * (1.0 - borderRatio)));
		mTemplateROI = Rect(tl, br);
		cout << "Template area set to: " << mTemplateROI << endl;

		//calculate ROI for search area
		borderRatio = (1.0 - mTempRatio - 2 * mMaxShift) * 0.5;
		tl = Point(floor(width * borderRatio), floor(height * borderRatio));
		br = Point(ceil(width * (1.0 - borderRatio)), ceil(height * (1.0 - borderRatio)));
		mSearchROI = Rect(tl, br);
		cout << "Search area set to: " << mSearchROI << endl;

		mTemplates = generatePyramid(first, mTemplateROI);
	}

	Point3f getDisplacement(const Mat& img) override
	{
		vector<Mat> pyramid = generatePyramid(img, mSearchROI);

		Point bestLoc = mPeakFinder->findPeak(pyramid[mLayers], mTemplates[mLayers]);

		for (int i = mLayers - 1; i >= 0; i--)
		{
			bestLoc *= 2;
			Point border = Point(1, 1);
			if (bestLoc == Point(0, 0)) border = Point(0, 0);

			bestLoc += mPeakFinder->findPeak(match(pyramid[i](Rect(bestLoc - border, mTemplates[i].size() + Size(2, 2))), mTemplates[i])) - border;
		}
		bestLoc += mSearchROI.tl() - mTemplateROI.tl();

		mTemplates = generatePyramid(img, mTemplateROI);

		return Point3f(bestLoc);
	}
};
