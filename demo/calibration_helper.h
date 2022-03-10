#pragma once


#include <opencv2/opencv.hpp>

double calibrateIntrisic(const std::vector<std::vector<cv::Point2f> >& listImgCorners, cv::Size chessboardSize, cv::Size imgSize, cv::Mat& K, cv::Mat& distCoeffs, std::vector<cv::Mat>& rvecs, std::vector<cv::Mat>& tvecs);

std::vector<cv::Point2f> projectChessboard(cv::Size chessboardSize, const cv::Mat& K, const cv::Mat& distCoeffs, const cv::Mat& rvec, const cv::Mat& tvec);
