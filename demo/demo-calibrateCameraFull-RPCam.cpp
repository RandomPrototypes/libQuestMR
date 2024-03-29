#include <stdio.h>
#include <thread>
#include <opencv2/opencv.hpp>
#include <libQuestMR/QuestCalibData.h>
#include <libQuestMR/QuestCommunicator.h>
#include <libQuestMR/QuestFrameData.h>
#include <RPCameraInterface/CameraInterface.h>
#include <RPCameraInterface/ImageFormatConverter.h>
#include "calibration_helper.h"
#include "RPCam_helper.h"

using namespace libQuestMR;
using namespace RPCameraInterface;

class MouseData
{
public:
    int x,y;
    bool leftDown;

	MouseData()
		:leftDown(false), x(0), y(0)
	{
	}
};


void MouseCallBackFunc(int event, int x, int y, int flags, void* userdata)
{
    MouseData *data = (MouseData*)userdata;
    if(event == cv::EVENT_LBUTTONDOWN) {
        data->leftDown = true;
    } else if(event == cv::EVENT_LBUTTONUP) {
        data->leftDown = false;
    }
    data->x = x;
    data->y = y;
}

void captureAndCalibrateFull(const char *ipAddr, const char *outputFilename)
{
	printf("captureAndCalibrateFull: ip %s, output %s\n", ipAddr, outputFilename);
	
	std::shared_ptr<CameraInterface> cam;
	ImageFormat srcFormat;
	if(!configureCamera(cam, &srcFormat)) {
		printf("error configuring camera!!\n");
		return ;
	}
    
    std::shared_ptr<QuestCommunicator> questCom = createQuestCommunicator();
    if(!questCom->connect(ipAddr)) {
    	printf("can not connect to Quest\n");
    	return ;
    }
    std::shared_ptr<QuestCommunicatorThreadData> questComData = createQuestCommunicatorThreadData(questCom);
    std::thread questComThread(QuestCommunicatorThreadFunc, questComData.get());
    
	
	int maxImgSize = 1280;
	
	//Choose the conversion format and initialize the converter
    ImageFormat dstFormat(ImageType::BGR24, srcFormat.width, srcFormat.height);
    ImageFormatConverter converter(srcFormat, dstFormat);
    
    std::shared_ptr<ImageData> imgData2 = createImageData();
	
	std::vector<cv::Mat> listImg;
	std::vector<cv::Point3d> listRightHandPos;
	
	cv::Mat frame, frame_gray;
	cv::Mat K, distCoeffs;
	QuestFrameData frameData;
    bool hasFrameData = false;
	uint32_t currentTriggerCount = 0;
	while(true) 
	{
		//Obtain the frame
        std::shared_ptr<ImageData> imgData = cam->getNewFrame(true);
        //Conver to the output format (BGR 720x480)
        converter.convertImage(imgData, imgData2);
        //Create OpenCV Mat for visualization
        cv::Mat frame(imgData2->getImageFormat().height, imgData2->getImageFormat().width, CV_8UC3, imgData2->getDataPtr());
		if (frame.empty()) {
            printf("error : empty frame grabbed");
            break;
        }
        
        if(questComData->hasNewFrameData())
        {
            while(questComData->hasNewFrameData())
                questComData->getFrameData(&frameData);
            hasFrameData = true;
        }
        
        if(hasFrameData && questComData->getTriggerCount() > currentTriggerCount && frameData.isRightHandValid())
        {
        	listImg.push_back(frame.clone());
        	listRightHandPos.push_back(cv::Point3d(frameData.right_hand_pos[0], frameData.right_hand_pos[1], frameData.right_hand_pos[2]));
            currentTriggerCount = questComData->getTriggerCount();	
        }
        
        cv::Mat img = frame.clone();

        if(frame.cols > maxImgSize || frame.rows > maxImgSize) {
        	float ratio = static_cast<float>(maxImgSize) / std::max(frame.cols, frame.rows);
        	cv::resize(img, img, cv::Size(cvRound(frame.cols * ratio), cvRound(frame.rows * ratio)));
        }
        cv::putText(img, "nb images: "+std::to_string(listImg.size())+"/"+std::to_string(std::max(5, (int)listImg.size()))+", press 'c' to calibrate", cv::Point(20, 20),  cv::FONT_HERSHEY_DUPLEX, 0.7, cv::Scalar(0, 0, 255), 2);
        cv::imshow("img", img);
        int key = cv::waitKey(20);
        if(key == 'c' && listImg.size() >= 5)
        	break;
	}
	
	questComData->setFinishedVal(true);
	
	MouseData mouseData;
    cv::setMouseCallback("img", MouseCallBackFunc, &mouseData);
    
    std::vector<cv::Point2d> listPoints2D;
	
	for(size_t i = 0; i < listImg.size(); i++) {
		cv::Mat frame = listImg[i];
        float ratio = static_cast<float>(maxImgSize) / std::max(frame.cols, frame.rows);
        bool wasLeftDown = false;
		while(mouseData.leftDown || !wasLeftDown) {
			if(mouseData.leftDown)
				wasLeftDown = true;
			cv::Mat img = frame.clone();
			cv::circle(img, cv::Point(cvRound(mouseData.x / ratio), cvRound(mouseData.y / ratio)), 5, cv::Scalar(0,255,0), 2);
			if(frame.cols > maxImgSize || frame.rows > maxImgSize) {
		    	cv::resize(img, img, cv::Size(cvRound(frame.cols * ratio), cvRound(frame.rows * ratio)));
		    }
			cv::imshow("img", img);
			cv::waitKey(20);
		}
		listPoints2D.push_back(cv::Point2d(cvRound(mouseData.x / ratio), cvRound(mouseData.y / ratio)));
	}
	
	QuestCalibData calibData;
	calibData.setImageSize(listImg[0].size());
	for(int i = 0; i < 3; i++)
		calibData.raw_translation[i] = frameData.raw_pos[i];
	for(int i = 0; i < 4; i++)
		calibData.raw_rotation[i] = frameData.raw_rot[i];
	calibData.calibrateCamIntrinsicAndPose(listRightHandPos, listPoints2D, true);

	std::string xmlStr = calibData.generateXMLString().str();
	FILE *file = fopen(outputFilename, "w");
	if(file) {
		fwrite(xmlStr.c_str(), 1, xmlStr.size(), file);
		fclose(file);
	}
	printf("calibration finished!!!\n");
	
    questComThread.join();
}

int main(int argc, char **argv) 
{
	if(argc < 3) {
		printf("usage: demo-calibrateCameraFull-RPCam ipAddr outputFile\n");
	} else {
		captureAndCalibrateFull(argv[1], argv[2]);
	}
    return 0;
}
