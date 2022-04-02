#include "calibration_helper.h"

double calibrateIntrisic(const std::vector<std::vector<cv::Point2f> >& listImgCorners, cv::Size chessboardSize, cv::Size imgSize, cv::Mat& K, cv::Mat& distCoeffs, std::vector<cv::Mat>& rvecs, std::vector<cv::Mat>& tvecs)
{
	std::vector<cv::Point3f> corners3D;
	for(int i = 0; i < chessboardSize.height; i++)
	{
		for(int j = 0; j < chessboardSize.width; j++)
			corners3D.push_back(cv::Point3f((float)j,(float)i,0.0f));
	}
	
	std::vector<std::vector<cv::Point3f> > listPoints3D;
	for(int i = 0; i < listImgCorners.size(); i++)
		listPoints3D.push_back(corners3D);
	
	int flags = cv::CALIB_FIX_K3 | cv::CALIB_FIX_K4 | cv::CALIB_FIX_K5 | cv::CALIB_FIX_K6;// | cv::CALIB_ZERO_TANGENT_DIST;
    distCoeffs = cv::Mat::zeros(8, 1, CV_64F);
    /*
    //if opencv version >= 4
    int iFixedPoint = chessboardSize.width - 1;//-1
    std::vector<cv::Point3f> newObjPoints;
    return calibrateCameraRO(listPoints3D, listImgCorners, imgSize, iFixedPoint, K, distCoeffs, rvecs, tvecs, newObjPoints, flags | cv::CALIB_USE_LU);
	*/
	return calibrateCamera(listPoints3D, listImgCorners, imgSize, K, distCoeffs, rvecs, tvecs, flags);
}

std::vector<cv::Point2f> projectChessboard(cv::Size chessboardSize, const cv::Mat& K, const cv::Mat& distCoeffs, const cv::Mat& rvec, const cv::Mat& tvec)
{
	std::vector<cv::Point3f> corners3D;
	for(int i = 0; i < chessboardSize.height; i++)
	{
		for(int j = 0; j < chessboardSize.width; j++)
			corners3D.push_back(cv::Point3f(j,i,0));
	}
	std::vector<cv::Point2f> result;
	cv::projectPoints(corners3D, rvec, tvec, K, distCoeffs, result);
	return result;
}
