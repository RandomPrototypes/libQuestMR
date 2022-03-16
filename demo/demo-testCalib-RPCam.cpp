#include <stdio.h>
#include <thread>
#include <opencv2/opencv.hpp>
#include <libQuestMR/QuestCalibData.h>
#include <libQuestMR/QuestCommunicator.h>
#include <libQuestMR/QuestFrameData.h>
#include <RPCameraInterface/CameraInterface.h>
#include <RPCameraInterface/ImageFormatConverter.h>
#include "RPCam_helper.h"

using namespace libQuestMR;
using namespace RPCameraInterface;

void testCalib(const char *ipAddr, const char *calibFilename)
{
	printf("testCalib: ip %s, calib %s\n", ipAddr, calibFilename);
	QuestCalibData calibData;
    calibData.loadXMLFile(calibFilename);
	
	std::shared_ptr<CameraInterface> cam;
	ImageFormat srcFormat;
	if(!configureCamera(cam, &srcFormat)) {
		printf("error configuring camera!!\n");
		return ;
	}
    
    QuestCommunicator questCom;
    if(!questCom.connect(ipAddr, 25671)) {
    	printf("can not connect to Quest\n");
    	return ;
    }
    QuestCommunicatorThreadData questComData(&questCom);
    std::thread questComThread(QuestCommunicatorThreadFunc, &questComData);
    
	
	int maxImgSize = 1280;
	
	//Choose the conversion format and initialize the converter
    ImageFormat dstFormat(ImageType::BGR24, srcFormat.width, srcFormat.height);
    ImageFormatConverter converter(srcFormat, dstFormat);
    
    std::shared_ptr<ImageData> imgData2 = std::make_shared<ImageData>();
	
	std::vector<cv::Mat> listImg;
	std::vector<cv::Point3d> listRightHandPos;
	
	cv::Mat frame, frame_gray;
	cv::Mat K, distCoeffs;
	QuestFrameData frameData;
    bool hasFrameData = false;
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
        
        if(questComData.hasNewFrameData())
        {
            while(questComData.hasNewFrameData())
                questComData.getFrameData(&frameData);
            hasFrameData = true;
        }
        
        cv::Mat img = frame.clone();
        if(hasFrameData)
        {
        	cv::Point2d head = calibData.projectToCam(cv::Point3d(frameData.head_pos[0], frameData.head_pos[1], frameData.head_pos[2]));
		    cv::Point2d leftHand = calibData.projectToCam(cv::Point3d(frameData.left_hand_pos[0], frameData.left_hand_pos[1], frameData.left_hand_pos[2]));
		    cv::Point2d rightHand = calibData.projectToCam(cv::Point3d(frameData.right_hand_pos[0], frameData.right_hand_pos[1], frameData.right_hand_pos[2]));
		    
		    cv::circle(img, head, 5, cv::Scalar(0,0,255), 2);
		    cv::circle(img, leftHand, 5, cv::Scalar(255,0,0), 2);
		    cv::circle(img, rightHand, 5, cv::Scalar(0,255,0), 2);	
        }

        if(frame.cols > maxImgSize || frame.rows > maxImgSize) {
        	float ratio = static_cast<float>(maxImgSize) / std::max(frame.cols, frame.rows);
        	cv::resize(img, img, cv::Size(cvRound(frame.cols * ratio), cvRound(frame.rows * ratio)));
        }

        cv::imshow("img", img);
        int key = cv::waitKey(20);
        if(key > 0)
        	break;
	}
	
	questComData.setFinishedVal(true);
    questComThread.join();
}

int main(int argc, char **argv) 
{
	if(argc < 3) {
		printf("usage: demo-calibrateCameraExtrinsic-RPCam ipAddr calibFile\n");
	} else {
		testCalib(argv[1], argv[2]);
	}
    return 0;
}
