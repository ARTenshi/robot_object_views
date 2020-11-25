///3D FRONT LINE FINDER
#include "erasers_tools/linefinder.h"

//Data structures for changing the point of view
limits::limits()
{
	minX = 0;
	minY = 0;
	minZ = 0;
	X = 0;
	Y = 0;
	Z = 0;
	maxX = 0;
	maxY = 0;
	maxZ = 0;
	deltaX = 0;
	deltaY = 0;
	deltaZ = 0;
};

limits::~limits()
{
};

infoPixel::infoPixel()
{
	i = 0;
	j = 0;

	x = 0;
	y = 0;
	z = 0;
};

infoPixel::~infoPixel()
{
};

pointOfViewParameters::pointOfViewParameters()
{
	debug = false;
	fullData = true;
	color = true;

	pixelArray = NULL;
	angle = 0;

	k = 0;
};
pointOfViewParameters::~pointOfViewParameters()
{
};
//******************

//Change coordinate system
//x-axis to the right
//y-axis going up
//z-axis depth
cv::Mat getXYZPoint (cv::Mat xyzCloud, int x, int y)
{
	cv::Mat point = cv::Mat::zeros(1, 3, CV_64FC1);

	point.at<double>(0) = -1*xyzCloud.at<cv::Vec3f>(y,x)[1];
	point.at<double>(1) = xyzCloud.at<cv::Vec3f>(y,x)[2];
	point.at<double>(2) = xyzCloud.at<cv::Vec3f>(y,x)[0];
	
	return point;
};

//Change coordinate system
//x-axis depth
//y-axis to the left
//z-axis going up
cv::Mat getCloudPoint (cv::Mat point)
{
	cv::Mat pcPoint = cv::Mat::zeros(1, 3, CV_64FC1);

	pcPoint.at<double>(0) = point.at<double>(2);
	pcPoint.at<double>(1) = -1*point.at<double>(0);
	pcPoint.at<double>(2) = point.at<double>(1);
	
	return pcPoint;
};

//Perspective correction of a point with respect to the Kinect(TM)'s camera plane
cv::Mat pointRotation (cv::Mat point, double angle)
{
	double RAD = 3.141592/180.0;
	double COS = cos(-1*angle*RAD);
	double SIN = sin(-1*angle*RAD);

	cv::Mat pointRotated = cv::Mat::zeros(1, 3, CV_64FC1);

	pointRotated.at<double>(0) = point.at<double>(0);
	pointRotated.at<double>(1) = point.at<double>(1)*COS - point.at<double>(2)*SIN;
	pointRotated.at<double>(2) = point.at<double>(1)*SIN + point.at<double>(2)*COS;

	return pointRotated;
};

// Calculate the normal module of point(x, y) 
// with respect to the canonincal (natural) horizontal vector
double horizontalNormalModule (cv::Mat xyzCloud, int x, int y, double angle)
{
	double XYZr = 0, hmod = 0;

	cv::Mat point_0 = getXYZPoint (xyzCloud, x, y);
	cv::Mat point_1 = getXYZPoint (xyzCloud, x-2, y-2);
	cv::Mat point_2 = getXYZPoint (xyzCloud, x+2, y+2);
	cv::Mat point_3 = getXYZPoint (xyzCloud, x+2, y-2);
	cv::Mat point_4 = getXYZPoint (xyzCloud, x-2, y+2);

	if ( (point_1.at<double>(2) != 0) && (point_2.at<double>(2) != 0) && 
		 (point_3.at<double>(2) != 0) && (point_4.at<double>(2) != 0))
	{
		cv::Mat point_a = cv::Mat::zeros(1, 3, CV_64FC1);
		cv::Mat point_b = cv::Mat::zeros(1, 3, CV_64FC1);
		cv::Mat point_c = cv::Mat::zeros(1, 3, CV_64FC1);

		//Get the normal to the patch around the point
		point_a = point_2 - point_1;
		point_b = point_3 - point_4;
		point_c = point_a.cross(point_b);

		//Get the vertical proyection to the natural horizont plane's normal
		cv::Mat point_r = pointRotation (point_c, angle);
		XYZr = cv::norm(point_r);
		hmod = fabs(point_r.at<double>(1)/XYZr);
	}

	return hmod;
}

// Changes XY view to XZ
int changeViewPerspective ( cv::Mat bgrImg, cv::Mat xyzCloud, pointOfViewParameters &povParams)
{
	int i, j, x, y, numVectors = 0;

	povParams.src = bgrImg.clone();
	int srcH = povParams.src.rows;
	int srcW = povParams.src.cols;

	if (povParams.color == true)
		povParams.pov = cv::Mat::zeros(povParams.src.size(), CV_8UC3);
	else
		povParams.pov = cv::Mat::zeros(povParams.src.size(), CV_8UC1);

	if (povParams.pixelArray == NULL)
		povParams.pixelArray = (infoPixel *)malloc(srcH*srcW*sizeof(infoPixel));

	//Use whole height range
	if (povParams.fullData)
	{
		for (y = 0; y < srcH; y++)
			for (x = 0; x < srcW; x++)
			{
				cv::Mat point_r = pointRotation (getXYZPoint (xyzCloud, x, y), povParams.angle);

				if ((!std::isnan(point_r.at<double>(2)))&&
					(point_r.at<double>(2) < povParams.areaLimits.maxZ) &&
					(fabs(point_r.at<double>(0)) < povParams.areaLimits.maxX/2))
				{
					//Convert from 3D metric units to pixel units
					i = (int)(((point_r.at<double>(0) + povParams.areaLimits.maxX/2)*srcW)/povParams.areaLimits.maxX);
					j = (int)(srcH - (point_r.at<double>(2) * srcH)/povParams.areaLimits.maxZ);

					if (i < 0) i = 0;
					if (i >= srcW) i = srcW - 1;

					if (j < 0) j = 0;
					if (j >= srcH) j = srcH - 1;

					//Use RGB mask
					if (povParams.color == true)
					{
						povParams.pov.at<cv::Vec3b>(j,i)[0] = povParams.src.at<cv::Vec3b>(y,x)[0];
						povParams.pov.at<cv::Vec3b>(j,i)[1] = povParams.src.at<cv::Vec3b>(y,x)[1];
						povParams.pov.at<cv::Vec3b>(j,i)[2] = povParams.src.at<cv::Vec3b>(y,x)[2];
					}
					//Use B&W mask
					else
						povParams.pov.at<uchar>(j,i) = 255;

					//Save data
					povParams.pixelArray[j*srcW + i].i = x;
					povParams.pixelArray[j*srcW + i].j = y;
					povParams.pixelArray[j*srcW + i].x = point_r.at<double>(0);
					povParams.pixelArray[j*srcW + i].y = point_r.at<double>(1);
					povParams.pixelArray[j*srcW + i].z = point_r.at<double>(2);
				}

				if (std::isnan(povParams.pixelArray[y*srcW + x].x))
				{
					povParams.pixelArray[y*srcW + x].x = 0;
					povParams.pixelArray[y*srcW + x].y = 0;
					povParams.pixelArray[y*srcW + x].z = 0;
				}
			}
	}
	//Use limited height range
	else
	{
		for (y = 0; y < srcH; y++)
			for (x = 0; x < srcW; x++)
			{
				cv::Mat point_r = pointRotation (getXYZPoint (xyzCloud, x, y), povParams.angle);

				if (!std::isnan(point_r.at<double>(2)) && 
					(point_r.at<double>(2) < povParams.areaLimits.maxZ) &&
					(fabs(point_r.at<double>(0)) < povParams.areaLimits.maxX/2) &&
					(point_r.at<double>(1) > povParams.areaLimits.minY) &&
					(point_r.at<double>(1) < povParams.areaLimits.maxY))
				{
					//Convert from 3D metric units to pixel units
					i = (int)(((point_r.at<double>(0) + povParams.areaLimits.maxX/2)*srcW)/povParams.areaLimits.maxX);
					j = (int)(srcH - (point_r.at<double>(2) * srcH)/povParams.areaLimits.maxZ);

					if (i < 0) i = 0;
					if (i >= srcW) i = srcW - 1;

					if (j < 0) j = 0;
					if (j >= srcH) j = srcH - 1;

					//Use color mask
					if (povParams.color == true)
					{
						povParams.pov.at<cv::Vec3b>(j,i)[0] = povParams.src.at<cv::Vec3b>(y,x)[0];
						povParams.pov.at<cv::Vec3b>(j,i)[1] = povParams.src.at<cv::Vec3b>(y,x)[1];
						povParams.pov.at<cv::Vec3b>(j,i)[2] = povParams.src.at<cv::Vec3b>(y,x)[2];
					}
					//Use B&W mask
					else
						povParams.pov.at<uchar>(j,i) = 255;

					//Save data
					povParams.pixelArray[j*srcW + i].i = x;
					povParams.pixelArray[j*srcW + i].j = y;
					povParams.pixelArray[j*srcW + i].x = point_r.at<double>(0);
					povParams.pixelArray[j*srcW + i].y = point_r.at<double>(1);
					povParams.pixelArray[j*srcW + i].z = point_r.at<double>(2);
				}

				if (std::isnan(povParams.pixelArray[y*srcW + x].x))
				{
					povParams.pixelArray[y*srcW + x].x = 0;
					povParams.pixelArray[y*srcW + x].y = 0;
					povParams.pixelArray[y*srcW + x].z = 0;
				}
			}
	}
	return 1;
};

// Changes XY view to XZ
int changeViewPerspective ( pcl::PointCloud<pcl::PointXYZRGB>::Ptr pclCloud, pointOfViewParameters &povParams)
{
	int i, j, x, y, numVectors = 0;

	int height = pclCloud->height;
	int width = pclCloud->width;

	povParams.src = cv::Mat::zeros(height, width, CV_8UC3);

	if (povParams.color == true)
		povParams.pov = cv::Mat::zeros(povParams.src.size(), CV_8UC3);
	else
		povParams.pov = cv::Mat::zeros(povParams.src.size(), CV_8UC1);

	if (povParams.pixelArray == NULL)
		povParams.pixelArray = (infoPixel *)malloc(height*width*sizeof(infoPixel));

	//Use whole height range
	if (povParams.fullData)
	{
		//for (y = height-1; y >= 0; y--)
		for (y = 0; y < height; y++)
			for (x = 0; x < width; x++)
			{
				pcl::PointXYZRGB p = pclCloud->at(x,y);

				povParams.src.at<cv::Vec3b>(y,x)[0] = (int)p.b;
				povParams.src.at<cv::Vec3b>(y,x)[1] = (int)p.g;
				povParams.src.at<cv::Vec3b>(y,x)[2] = (int)p.r;

				cv::Mat point = cv::Mat::zeros(1, 3, CV_64FC1);

				point.at<double>(0) = (double)p.x;
				point.at<double>(1) = (double)p.y;
				point.at<double>(2) = (double)p.z;

				cv::Mat point_r = pointRotation (point, povParams.angle);

				if ((!std::isnan(point_r.at<double>(2)))&&
					(point_r.at<double>(2) < povParams.areaLimits.maxZ) &&
					(fabs(point_r.at<double>(0)) < povParams.areaLimits.maxX/2))
				{
					//Convert from 3D metric units to pixel units
					i = (int)(((point_r.at<double>(0) + povParams.areaLimits.maxX/2)*width)/povParams.areaLimits.maxX);
					j = (int)(height - (point_r.at<double>(2) * height)/povParams.areaLimits.maxZ);

					if (i < 0) i = 0;
					if (i >= width) i = width - 1;

					if (j < 0) j = 0;
					if (j >= height) j = height - 1;

					//Use RGB mask
					if (povParams.color == true)
					{
						povParams.pov.at<cv::Vec3b>(j,i)[0] = (int)p.b;
						povParams.pov.at<cv::Vec3b>(j,i)[1] = (int)p.g;
						povParams.pov.at<cv::Vec3b>(j,i)[2] = (int)p.r;
					}
					//Use B&W mask
					else
						povParams.pov.at<uchar>(j,i) = 255;

					//Save data
					povParams.pixelArray[j*width + i].i = x;
					povParams.pixelArray[j*width + i].j = y;
					povParams.pixelArray[j*width + i].x = point_r.at<double>(0);
					povParams.pixelArray[j*width + i].y = point_r.at<double>(1);
					povParams.pixelArray[j*width + i].z = point_r.at<double>(2);
				}

				if (std::isnan(povParams.pixelArray[y*width + x].x))
				{
					povParams.pixelArray[y*width + x].x = 0;
					povParams.pixelArray[y*width + x].y = 0;
					povParams.pixelArray[y*width + x].z = 0;
				}
			}
	}
	//Use limited height range
	else
	{
		//for (y = height-1; y >= 0; y--)
		for (y = 0; y < height; y++)
			for (x = 0; x < width; x++)
			{
				pcl::PointXYZRGB p = pclCloud->at(x,y);

				povParams.src.at<cv::Vec3b>(y,x)[0] = (int)p.b;
				povParams.src.at<cv::Vec3b>(y,x)[1] = (int)p.g;
				povParams.src.at<cv::Vec3b>(y,x)[2] = (int)p.r;

				cv::Mat point = cv::Mat::zeros(1, 3, CV_64FC1);

				point.at<double>(0) = (double)p.x;
				point.at<double>(1) = (double)p.y;
				point.at<double>(2) = (double)p.z;

				cv::Mat point_r = pointRotation (point, povParams.angle);

				if (!std::isnan(point_r.at<double>(2)) && 
					(point_r.at<double>(2) < povParams.areaLimits.maxZ) &&
					(fabs(point_r.at<double>(0)) < povParams.areaLimits.maxX/2) &&
					(point_r.at<double>(1) > povParams.areaLimits.minY) &&
					(point_r.at<double>(1) < povParams.areaLimits.maxY))
				{

					//Convert from 3D metric units to pixel units
					i = (int)(((point_r.at<double>(0) + povParams.areaLimits.maxX/2)*width)/povParams.areaLimits.maxX);
					j = (int)(height - (point_r.at<double>(2) * height)/povParams.areaLimits.maxZ);

					if (i < 0) i = 0;
					if (i >= width) i = width - 1;

					if (j < 0) j = 0;
					if (j >= height) j = height - 1;

					//Use color mask
					if (povParams.color == true)
					{
						if ((int)p.b == 0 && (int)p.g == 0 && (int)p.r == 0)
						{
							povParams.pov.at<cv::Vec3b>(j,i)[0] = 1;
							povParams.pov.at<cv::Vec3b>(j,i)[1] = 1;
							povParams.pov.at<cv::Vec3b>(j,i)[2] = 1;
						} 
						else
						{
							povParams.pov.at<cv::Vec3b>(j,i)[0] = (int)p.b;
							povParams.pov.at<cv::Vec3b>(j,i)[1] = (int)p.g;
							povParams.pov.at<cv::Vec3b>(j,i)[2] = (int)p.r;
						}
					}
					//Use B&W mask
					else
						povParams.pov.at<uchar>(j,i) = 255;

					//Save data
					povParams.pixelArray[j*width + i].i = x;
					povParams.pixelArray[j*width + i].j = y;
					povParams.pixelArray[j*width + i].x = point_r.at<double>(0);
					povParams.pixelArray[j*width + i].y = point_r.at<double>(1);
					povParams.pixelArray[j*width + i].z = point_r.at<double>(2);
				}

				if (std::isnan(povParams.pixelArray[y*width + x].x))
				{
					povParams.pixelArray[y*width + x].x = 0;
					povParams.pixelArray[y*width + x].y = 0;
					povParams.pixelArray[y*width + x].z = 0;
				}
			}
	}
	return 1;
};

int pointOfViewParameters2PointCloud(pointOfViewParameters povParams, pcl::PointCloud<pcl::PointXYZRGB>::Ptr &povCloud)
{
	povCloud.reset(new pcl::PointCloud<pcl::PointXYZRGB>);

	int srcH = povParams.src.rows;
	int srcW = povParams.src.cols;

	for (int j = 0; j < srcH; j++)
		for (int i = 0; i < srcW; i++)
		{
			pcl::PointXYZRGB point = pcl::PointXYZRGB 
			(povParams.pov.at<cv::Vec3b>(j,i)[2],
			povParams.pov.at<cv::Vec3b>(j,i)[1],
			povParams.pov.at<cv::Vec3b>(j,i)[0]);

			point.x = povParams.pixelArray[j*srcW + i].x;
			point.y = povParams.pixelArray[j*srcW + i].y;
			point.z = povParams.pixelArray[j*srcW + i].z;
			
			povCloud->push_back(point);
		}
	
	povCloud->width = (uint32_t)srcW;
	povCloud->height = (uint32_t)srcH;
	return 1;
}

// Finds the closest point to bottom center (Kinect(TM)'s origin) in the XZ view
cv::Mat centerPoint(pointOfViewParameters &povParams)
{
	cv::Mat result;

	cv::Mat point2d = cv::Mat::zeros(1, 3, CV_64FC1);
	cv::Mat point3d = cv::Mat::zeros(1, 3, CV_64FC1);

	if (povParams.color)
		povParams.front = povParams.pov.clone();
	else
		cv::cvtColor(povParams.pov, povParams.front, CV_GRAY2BGR);

	int imgH = povParams.front.rows;
	int imgW = povParams.front.cols;

	double dist1 = 1000000, dist2 = 0;
	int X1 = (int)(imgW/2), X2 = 0;
	int Y1 = imgH, Y2 = 0;

	bool found = false;
	for (int i = 0; i < imgW; i++)
		for (int j = imgH - 1; j >= 0; j--)
		{
			if ((povParams.front.at<cv::Vec3b>(j,i)[0] != 0)||
				(povParams.front.at<cv::Vec3b>(j,i)[1] != 0)||
				(povParams.front.at<cv::Vec3b>(j,i)[2] != 0))
			{
				povParams.front.at<cv::Vec3b>(j,i)[0] = 0;
				povParams.front.at<cv::Vec3b>(j,i)[1] = 0;
				povParams.front.at<cv::Vec3b>(j,i)[2] = 255;

				dist2 = (X1 - i)*(X1 - i) + (Y1 - j)*(Y1 - j);
				if (dist2 < dist1)
				{
					dist1 = dist2;
					X2 = i;
					Y2 = j;

					found = true;
				}

				j = -1;
			}
		}

	if (found)
	{
		//Draw closest points
		cv::Point pt1 = cv::Point(X1,Y1);
		cv::Point pt2 = cv::Point(X2,Y2);

		cv::Scalar blue = cv::Scalar(255,0,0);
		int thickness = 2;
		int connectivity = 8;

		if ((pt2.x != 0)&&(pt2.y != 0)&&(pt2.y < 440))
		{
			cv::line(povParams.front, pt1, pt2, blue, thickness, connectivity);

			point2d.at<double>(0) = (double)povParams.pixelArray[Y2*imgW + X2].i;
			point2d.at<double>(1) = (double)povParams.pixelArray[Y2*imgW + X2].j;

			point3d.at<double>(0) = povParams.pixelArray[Y2*imgW + X2].x;
			point3d.at<double>(1) = povParams.pixelArray[Y2*imgW + X2].y;
			point3d.at<double>(2) = povParams.pixelArray[Y2*imgW + X2].z;
		}

		result.push_back(point2d);
		result.push_back(point3d);
	}

	return result;
};

// Finds the closest point to the canera plane in the XZ view
cv::Mat frontPoint(pointOfViewParameters &povParams)
{
	cv::Mat result;

	cv::Mat point = cv::Mat::zeros(1, 3, CV_64FC1);
	cv::Mat point2d = cv::Mat::zeros(1, 3, CV_64F);
	cv::Mat point3d = cv::Mat::zeros(1, 3, CV_64F);

	if (povParams.color)
		povParams.front = povParams.pov.clone();
	else
		cv::cvtColor(povParams.pov, povParams.front, CV_GRAY2BGR);

	int imgH = povParams.front.rows;
	int imgW = povParams.front.cols;

	double dist1 = 1000000, dist2 = 0;
	int X1 = (int)(imgW/2);
	int Y1 = imgH;

	bool found = false;
	for (int i = 0; i < imgW; i++)
		for (int j = imgH - 1; j >= 0; j--)
		{
			if ( (povParams.front.at<cv::Vec3b>(j,i)[0] != 0)||
			     (povParams.front.at<cv::Vec3b>(j,i)[1] != 0)||
			     (povParams.front.at<cv::Vec3b>(j,i)[2] != 0) )
			{
				povParams.front.at<cv::Vec3b>(j,i)[0] = 0;
				povParams.front.at<cv::Vec3b>(j,i)[1] = 0;
				povParams.front.at<cv::Vec3b>(j,i)[2] = 255;

				if (povParams.pixelArray[j*imgW + i].z < dist1)
				{
					point2d.at<double>(0) = i;
					point2d.at<double>(1) = j;

					point3d.at<double>(0) = povParams.pixelArray[j*imgW + i].x;
					point3d.at<double>(1) = povParams.pixelArray[j*imgW + i].y;
					point3d.at<double>(2) = povParams.pixelArray[j*imgW + i].z;

					dist1 = povParams.pixelArray[j*imgW + i].z;

					found = true;
				}

				j = -1;
			}
		}

	if (found)
	{
		int idxi = (int)point2d.at<double>(0);
		int mini = (int)point2d.at<double>(0);
		for (int i = idxi; i >= 0; i--)
			for (int j = imgH - 1; j >= 0; j--)
			{
				if ( (povParams.front.at<cv::Vec3b>(j,i)[0] != 0)||
				     (povParams.front.at<cv::Vec3b>(j,i)[1] != 0)||
				     (povParams.front.at<cv::Vec3b>(j,i)[2] != 0) )
				{
					if (fabs(point3d.at<double>(2) - povParams.pixelArray[j*imgW + i].z) < 0.02)
						mini = i;
					else
						i = -1;

					j = -1;
				}
			}

		cv::Mat closest_points;
		for (int i = mini; i < imgW; i++)
			for (int j = imgH - 1; j >= 0; j--)
			{
				if ((povParams.front.at<cv::Vec3b>(j,i)[0] != 0)||
					(povParams.front.at<cv::Vec3b>(j,i)[1] != 0)||
					(povParams.front.at<cv::Vec3b>(j,i)[2] != 0))
				{
					if (fabs(point3d.at<double>(2) - povParams.pixelArray[j*imgW + i].z) < 0.02)
					{
						cv::Mat point_ = cv::Mat::zeros(1, 3, CV_64F);
						point_.at<double>(0) = i;
						point_.at<double>(1) = j;
						closest_points.push_back(point_);
					}
					else
						i = imgW;

					j = -1;
				}
			}

		int pt_idx = (int)(closest_points.rows/2);
		point2d = closest_points.row(pt_idx);

		cv::Mat pt_;
		pt_ = closest_points.row(0);
		cv::Point pt1_ = cv::Point(int(pt_.at<double>(0)), int(pt_.at<double>(1)));
		pt_ = closest_points.row(closest_points.rows-1);
		cv::Point pt2_ = cv::Point(int(pt_.at<double>(0)), int(pt_.at<double>(1)));

		if ((pt2_.x != 0)&&(pt2_.y != 0)&&(pt2_.y < 440))
		{
			cv::line(povParams.front, pt1_, pt2_, cv::Scalar(0,255,0), 2, 8);
		}


		int pt_i = (int)point2d.at<double>(0);
		int pt_j = (int)point2d.at<double>(1);
		point3d.at<double>(0) = povParams.pixelArray[pt_j*imgW + pt_i].x;
		point3d.at<double>(1) = povParams.pixelArray[pt_j*imgW + pt_i].y;
		point3d.at<double>(2) = povParams.pixelArray[pt_j*imgW + pt_i].z;


		//Draw closest points
		cv::Point pt1 = cv::Point(X1,Y1);
		cv::Point pt2 = cv::Point( int(point2d.at<double>(0)), int(point2d.at<double>(1)) );

		cv::Scalar blue = cv::Scalar(255,0,0);
		int thickness = 2;
		int connectivity = 8;

		if ((pt2.x != 0)&&(pt2.y != 0)&&(pt2.y < 440))
		{
			cv::line(povParams.front, pt1, pt2, blue, thickness, connectivity);
		}

		point2d.at<double>(0) = (double)povParams.pixelArray[pt2.y*imgW + pt2.x].i;
		point2d.at<double>(1) = (double)povParams.pixelArray[pt2.y*imgW + pt2.x].j;

		result.push_back(point2d);
		result.push_back(point3d);
	}

	return result;
};

// Finds the closest line to bottom center (Kinect(TM)'s origin) in the XZ view
cv::Mat frontLine(pointOfViewParameters &povParams, bool dist3d = false)
{
	cv::Mat line;

	cv::Mat points2d;
	cv::Mat point2d = cv::Mat::zeros(1, 2, CV_32S);

	cv::Mat points3d;
	cv::Mat point3d = cv::Mat::zeros(1, 3, CV_64F);

	cv::Mat points3d2d;
	cv::Mat point3d2d = cv::Mat::zeros(1, 3, CV_64F);

	if (povParams.color)
		povParams.front = povParams.pov.clone();
	else
		cv::cvtColor(povParams.pov, povParams.front, CV_GRAY2BGR);

	int imgH = povParams.front.rows;
	int imgW = povParams.front.cols;

	for (int i = 0; i < imgW; i++)
		for (int j = imgH - 1; j >= 0; j--)
		{
			if ( (povParams.front.at<cv::Vec3b>(j,i)[0] != 0)||
			     (povParams.front.at<cv::Vec3b>(j,i)[1] != 0)||
			     (povParams.front.at<cv::Vec3b>(j,i)[2] != 0) )
			{
				povParams.front.at<cv::Vec3b>(j,i)[0] = 0;
				povParams.front.at<cv::Vec3b>(j,i)[1] = 0;
				povParams.front.at<cv::Vec3b>(j,i)[2] = 255;

				point2d.at<int>(0) = i;
				point2d.at<int>(1) = j;

				points2d.push_back(point2d);

				point3d.at<double>(0) = povParams.pixelArray[j*imgW + i].x;
				point3d.at<double>(1) = povParams.pixelArray[j*imgW + i].y;
				point3d.at<double>(2) = povParams.pixelArray[j*imgW + i].z;

				points3d.push_back(point3d);

				j = -1;
			}
		}

	if (!points2d.empty() && points2d.rows > 1)
	{
		//Find 3D line using pixel information
		if (dist3d)
		{
			//Find 3D line using metric information
			std::vector<int> line3d = lineRANSAC(points3d);

			//Get extreme points
			cv::Mat p1 = points3d.row(line3d[0]);
			cv::Mat p2 = points3d.row(line3d[1]);

			//Convert 3D metric points to 2D pixel points
			int X1 = (int)(((p1.at<double>(0) + povParams.areaLimits.maxX/2)*imgW)/povParams.areaLimits.maxX);
			int Y1 = (int)(imgH - (p1.at<double>(2)*imgH)/povParams.areaLimits.maxZ);

			if (X1 < 0) X1 = 0;
			if (X1 >= imgW) X1 = imgW - 1;

			if (Y1 < 0) Y1 = 0;
			if (Y1 >= imgH) Y1 = imgH - 1;

			int X2 = (int)(((p2.at<double>(0) + povParams.areaLimits.maxX/2)*imgW)/povParams.areaLimits.maxX);
			int Y2 = (int)(imgH - (p2.at<double>(2)*imgH)/povParams.areaLimits.maxZ);

			if (X2 < 0) X2 = 0;
			if (X2 >= imgW) X2 = imgW - 1;

			if (Y2 < 0) Y2 = 0;
			if (Y2 >= imgH) Y2 = imgH - 1;

			//Draw fitted line
			cv::Point pt1 = cv::Point(X1,Y1);
			cv::Point pt2 = cv::Point(X2,Y2);

			cv::Scalar blue = cv::Scalar(255, 0,0);
			int thickness = 2;
			int connectivity = 8;
			cv::line(povParams.front, pt1, pt2, blue, thickness, connectivity);

			//Left 3D Point
			line.push_back(points3d.row(line3d[0]));

			//Right 3D Point
			line.push_back(points3d.row(line3d[1]));

			//Left 3D2D Point
			point3d2d.at<double>(0) = (double)povParams.pixelArray[Y1*imgW + X1].i;
			point3d2d.at<double>(1) = (double)povParams.pixelArray[Y1*imgW + X1].j;
			line.push_back(point3d2d);

			//Right 3D2D Point
			point3d2d.at<double>(0) = (double)povParams.pixelArray[Y2*imgW + X2].i;
			point3d2d.at<double>(1) = (double)povParams.pixelArray[Y2*imgW + X2].j;
			line.push_back(point3d2d);
		}
		else
		{
			//Find 2D line using pixel information
			std::vector<int> line2d = lineRANSAC(points2d);

			//Get extreme points
			cv::Mat pointleft, pointright;

			pointleft = points2d.row(line2d[0]);
			cv::Point pt1 = cv::Point(pointleft.at<int>(0),pointleft.at<int>(1));

			pointright = points2d.row(line2d[1]);
			cv::Point pt2 = cv::Point(pointright.at<int>(0),pointright.at<int>(1));

			//Get extreme points
			cv::Scalar blue = cv::Scalar(255,0,0);
			int thickness = 2;
			int connectivity = 8;
			cv::line(povParams.front, pt1, pt2, blue, thickness, connectivity);

			//Left 3D Point
			line.push_back(points3d.row(line2d[0]));

			//Right 3D Point
			line.push_back(points3d.row(line2d[1]));
		}
	}

	return line;
};

