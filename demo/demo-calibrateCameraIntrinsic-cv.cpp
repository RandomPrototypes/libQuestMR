#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <libQuestMR/QuestCalibData.h>
#include "calibration_helper.h"

using namespace libQuestMR;

void captureAndCalibrateIntrinsic(int deviceID, cv::Size chessboardSize, const char *outputFilename)
{
	cv::VideoCapture cap;
	cap.open(deviceID);
	if (!cap.isOpened()) {
		printf("can not open device %d\n", deviceID);
		return ;
	}
	
	cap.set(cv::CAP_PROP_FRAME_WIDTH, 1280);
	cap.set(cv::CAP_PROP_FRAME_HEIGHT, 720);
	
	int maxImgSize = 1000;
	
	std::vector<std::vector<cv::Point2f> > listImgCorners;
	std::vector<cv::Mat> listImg;
	
	cv::Mat frame, frame_gray;
	cv::Mat K, distCoeffs;
	std::vector<cv::Mat> rvecs, tvecs;
	while(true) 
	{
		cap.read(frame);
		if (frame.empty()) {
            printf("error : empty frame grabbed");
            break;
        }
        
        cv::Mat img = frame.clone();
        
        cv::cvtColor(frame, frame_gray, cv::COLOR_BGR2GRAY);
        
        std::vector<cv::Point2f> corners;
        bool success = cv::findChessboardCorners(frame_gray, chessboardSize, corners, cv::CALIB_CB_ADAPTIVE_THRESH | cv::CALIB_CB_FAST_CHECK | cv::CALIB_CB_NORMALIZE_IMAGE);

	    if(success)
	    {
			cv::cornerSubPix(frame_gray, corners, cv::Size(11,11), cv::Size(-1,-1), cv::TermCriteria( cv::TermCriteria::EPS+cv::TermCriteria::COUNT, 30, 0.0001 ));
			cv::drawChessboardCorners(img, chessboardSize, corners, success);
	    }
        
        if(frame.cols > maxImgSize || frame.rows > maxImgSize) {
        	float ratio = static_cast<float>(maxImgSize) / std::max(frame.cols, frame.rows);
        	cv::resize(img, img, cv::Size(cvRound(frame.cols * ratio), cvRound(frame.rows * ratio)));
        }
        cv::putText(img, "nb images: "+std::to_string(listImg.size()), cv::Point(20, 20),  cv::FONT_HERSHEY_DUPLEX, 0.7, cv::Scalar(0, 0, 255), 2);
        cv::imshow("img", img);
        int key = cv::waitKey(20);
        if(key == ' ' && success) {
        	listImg.push_back(frame.clone());
        	listImgCorners.push_back(corners);
        } else if(key == 'c' && listImg.size() >= 5) {
        	double rms = calibrateIntrisic(listImgCorners, chessboardSize, listImg[0].size(), K, distCoeffs, rvecs, tvecs);
        	printf("rms : %lf\n", rms);
        	break;
        }
	}
	for(size_t i = 0; i < listImg.size(); i++) {
		std::vector<cv::Point2f> points2D = projectChessboard(chessboardSize, K, distCoeffs, rvecs[i], tvecs[i]);
		cv::Mat frame = listImg[i];
		cv::Mat img = frame.clone();
		cv::drawChessboardCorners(img, chessboardSize, points2D, true);
		if(frame.cols > maxImgSize || frame.rows > maxImgSize) {
        	float ratio = static_cast<float>(maxImgSize) / std::max(frame.cols, frame.rows);
        	cv::resize(img, img, cv::Size(cvRound(frame.cols * ratio), cvRound(frame.rows * ratio)));
        }
		cv::imshow("img", img);
		cv::waitKey(0);
	}
	if(distCoeffs.rows < 8) {
		cv::Mat tmp = cv::Mat::zeros(8, 1, distCoeffs.type());
		distCoeffs.copyTo(tmp(cv::Rect(0,0,1,distCoeffs.rows)));
		distCoeffs = tmp;
	}
	QuestCalibData calibData;
	calibData.image_width = listImg[0].cols;
	calibData.image_height = listImg[0].rows;
	calibData.setCameraMatrix(K);
	calibData.setDistCoeffs(distCoeffs);
	std::string xmlStr = calibData.generateXMLString().str();
	FILE *file = fopen(outputFilename, "w");
	if(file) {
		fwrite(xmlStr.c_str(), 1, xmlStr.size(), file);
		fclose(file);
	}
}

int main(int argc, char **argv) 
{
	if(argc < 5) {
		printf("usage: demo-calibrateCameraIntrinsic-cv deviceID chessboardRows chessboardCols outputFile\n");
	} else {
		captureAndCalibrateIntrinsic(std::stoi(argv[1]), cv::Size(std::stoi(argv[2]), std::stoi(argv[3])), argv[4]);
	}
	return 0;
}
