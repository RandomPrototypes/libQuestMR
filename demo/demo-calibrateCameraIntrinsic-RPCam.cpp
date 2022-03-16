#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <libQuestMR/QuestCalibData.h>
#include <RPCameraInterface/CameraInterface.h>
#include <RPCameraInterface/ImageFormatConverter.h>
#include "calibration_helper.h"

using namespace libQuestMR;
using namespace RPCameraInterface;

void captureAndCalibrateIntrinsic(cv::Size chessboardSize, const char *outputFilename)
{
	//Obtain a camera enumerator using default backend
	std::shared_ptr<CameraEnumerator> camEnum = getCameraEnumerator();
    camEnum->detectCameras();
    printf("%d cameras detected\n", camEnum->count());
    for(int i = 0; i < camEnum->count(); i++) {
        printf("%d: %s, %s\n", i, camEnum->getCameraId(i).c_str(), camEnum->getCameraName(i).c_str());
    }
	
	int cameraId = -1;
	while(cameraId < 0 || cameraId >= camEnum->count())
	{
		printf("Which camera? ");
		scanf("%d", &cameraId);
	}
	
	//Obtain a camera interface using the same backend as the enumerator
    std::shared_ptr<CameraInterface> cam = getCameraInterface(camEnum->backend);
    //Open the camera using the id from the enumerator
    cam->open(camEnum->getCameraId(cameraId).c_str());
    //Get the list of available formats
    std::vector<ImageFormat> listFormats = cam->getAvailableFormats();
    for(size_t i = 0; i < listFormats.size(); i++) {
    	ImageFormat& format = listFormats[i];
        printf("%d: %dx%d (%s)\n", (int)i, format.width, format.height, toString(format.type).c_str());
    }
    
    int formatId = -1;
	while(formatId < 0 || formatId >= listFormats.size())
	{
		printf("Which format? ");
		scanf("%d", &formatId);
	}
	
	//Select the format
    cam->selectFormat(listFormats[formatId]);
    if(!cam->startCapturing())
        printf("error in startCapturing()\n");
    
	
	int maxImgSize = 1280;
	
	//Choose the conversion format and initialize the converter
    ImageFormat dstFormat(ImageType::BGR24, listFormats[formatId].width, listFormats[formatId].height);
    ImageFormatConverter converter(listFormats[formatId], dstFormat);
    
    std::shared_ptr<ImageData> imgData2 = std::make_shared<ImageData>();
	
	std::vector<std::vector<cv::Point2f> > listImgCorners;
	std::vector<cv::Mat> listImg;
	
	cv::Mat frame, frame_gray;
	cv::Mat K, distCoeffs;
	std::vector<cv::Mat> rvecs, tvecs;
	while(true) 
	{
		//Obtain the frame
        std::shared_ptr<ImageData> imgData = cam->getNewFrame(true);
        //Conver to the output format (BGR 720x480)
        converter.convertImage(imgData, imgData2);
        //Create OpenCV Mat for visualization
        cv::Mat frame(imgData2->imageFormat.height, imgData2->imageFormat.width, CV_8UC3, imgData2->data);
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
	std::string xmlStr = calibData.generateXMLString();
	FILE *file = fopen(outputFilename, "w");
	if(file) {
		fwrite(xmlStr.c_str(), 1, xmlStr.size(), file);
		fclose(file);
	}
}

int main(int argc, char **argv) 
{
	if(argc < 4) {
		printf("usage: demo-calibrateCameraIntrinsic-RPCam chessboardRows chessboardCols outputFile\n");
	} else {
		captureAndCalibrateIntrinsic(cv::Size(std::stoi(argv[1]), std::stoi(argv[2])), argv[3]);
	}
    return 0;
}
