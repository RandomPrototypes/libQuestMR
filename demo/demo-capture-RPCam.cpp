#include <stdio.h>

#include <libQuestMR/QuestVideoMngr.h>
#include <RPCameraInterface/ImageFormatConverter.h>
#include "RPCam_helper.h"

using namespace libQuestMR;
using namespace RPCameraInterface;

cv::Mat mergeImg(cv::Mat fgImg, cv::Mat bgImg, cv::Mat fgMask)
{
	cv::Mat result = fgImg.clone();
	for(int i = 0; i < fgImg.rows && i < bgImg.rows; i++)
	{
		unsigned char *resultPtr = result.ptr<unsigned char>(i);
		unsigned char *bgPtr = bgImg.ptr<unsigned char>(i);
		unsigned char *fgPtr = fgImg.ptr<unsigned char>(i);
		unsigned char *fgMaskPtr = fgMask.ptr<unsigned char>(i);
		for(int j = 0; j < fgImg.cols && j < bgImg.cols; j++)
		{
			if(fgMaskPtr[j] == 0)
			{
				resultPtr[j*3] = bgPtr[j*3];
				resultPtr[j*3+1] = bgPtr[j*3+1];
				resultPtr[j*3+2] = bgPtr[j*3+2];
			}
		}
	}
	return result;
}

void captureFromQuest(const char *ipAddr)
{
	std::shared_ptr<CameraInterface> cam;
	ImageFormat srcFormat;
	if(!configureCamera(cam, &srcFormat)) {
		printf("error configuring camera!!\n");
		return ;
	}
	
	cv::Ptr<cv::BackgroundSubtractor> pBackSub = cv::createBackgroundSubtractorMOG2();
	
    QuestVideoMngr mngr;
    QuestVideoSourceBufferedSocket videoSrc;
    if(!videoSrc.Connect(ipAddr)) {
    	printf("can not connect to quest\n");
    	return;
    }
    mngr.attachSource(&videoSrc);
    
    
    //Choose the conversion format and initialize the converter
    ImageFormat dstFormat(ImageType::BGR24, srcFormat.width, srcFormat.height);
    ImageFormatConverter converter(srcFormat, dstFormat);
    
    std::shared_ptr<ImageData> imgData2 = std::make_shared<ImageData>();
    
    while(true)
    {
        mngr.VideoTickImpl();
        cv::Mat questImg = mngr.getMostRecentImg();
     
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
        
        cv::Mat fgMask;
        pBackSub->apply(frame, fgMask);
        
        printf("quest: %dx%d, camera %dx%d\n", questImg.cols, questImg.rows, frame.cols, frame.rows);
        if(!frame.empty())
        {
        	cv::Mat frame2;
        	cv::resize(frame, frame2, cv::Size(frame.cols/2, frame.rows/2));
        	cv::imshow("cam", frame2);
        }
        
		cv::Mat img = mergeImg(frame, questImg, fgMask);
        if(!img.empty())
        {
            //cv::resize(img, img, cv::Size(img.cols/2, img.rows/2));
            cv::imshow("img", img);
            //cv::imshow("questFG", questImg(cv::Rect(0,0,questImg.cols/2,questImg.rows)));
            //cv::imshow("questBG", questImg(cv::Rect(questImg.cols/2,0,questImg.cols/2,questImg.rows)));
            cv::imshow("img", img);
            int key = cv::waitKey(10);
            if(key > 0)
                break;
        }
    }
    mngr.detachSource();
    videoSrc.Disconnect();
}

int main(int argc, char** argv) 
{
	if(argc < 2) {
		printf("usage: demo-capture-RPCam ipAddr\n");
	} else {
		captureFromQuest(argv[1]);
    }
    return 0;
}
