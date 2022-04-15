#include <stdio.h>

#include <libQuestMR/QuestVideoMngr.h>
#include <RPCameraInterface/ImageFormatConverter.h>
#include <RPCameraInterface/OpenCVConverter.h>
#include <RPCameraInterface/VideoEncoder.h>
#include "RPCam_helper.h"

using namespace libQuestMR;
using namespace RPCameraInterface;

void testRecord(const char *ipAddr, const char *outputFile)
{
	std::shared_ptr<CameraInterface> cam;
	ImageFormat srcFormat;
	if(!configureCamera(cam, &srcFormat)) {
		printf("error configuring camera!!\n");
		return ;
	}
	std::shared_ptr<VideoEncoder> videoEncoder = createVideoEncoder();
	videoEncoder->setUseFrameTimestamp(true);
	if(outputFile != NULL)
		videoEncoder->open(outputFile, srcFormat.height, srcFormat.width, 30);

	//Choose the conversion format and initialize the converter
    ImageFormat dstFormat(ImageType::BGR24, srcFormat.width, srcFormat.height);
    ImageFormatConverter converter(srcFormat, dstFormat);
	
	std::shared_ptr<ImageData> imgData2 = createImageData();
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

		printf("timestamp %d\n", imgData2->getTimestamp());

		if(outputFile != NULL)
			videoEncoder->write(createImageDataFromMat(frame, imgData2->getTimestamp(), false));
		cv::imshow("img", frame);
		int key = cv::waitKey(10);
		if(key > 0)
			break;
	}
	if(outputFile != NULL)
		videoEncoder->release();
}

void captureFromQuest(const char *ipAddr, const char *outputFile)
{
	std::shared_ptr<CameraInterface> cam;
	ImageFormat srcFormat;
	if(!configureCamera(cam, &srcFormat)) {
		printf("error configuring camera!!\n");
		return ;
	}
	
	cv::Ptr<cv::BackgroundSubtractor> pBackSub = cv::createBackgroundSubtractorMOG2();
	
    std::shared_ptr<QuestVideoMngr> mngr = createQuestVideoMngr();
    std::shared_ptr<QuestVideoSourceBufferedSocket> videoSrc = createQuestVideoSourceBufferedSocket();
    if(!videoSrc->Connect(ipAddr)) {
    	printf("can not connect to quest\n");
    	return;
    }
    mngr->attachSource(videoSrc);
    
    
    //Choose the conversion format and initialize the converter
    ImageFormat dstFormat(ImageType::BGR24, srcFormat.width, srcFormat.height);
    ImageFormatConverter converter(srcFormat, dstFormat);
    
    std::shared_ptr<ImageData> imgData2 = createImageData();

	std::shared_ptr<VideoEncoder> videoEncoder = createVideoEncoder();
	videoEncoder->setUseFrameTimestamp(true);
	if(outputFile != NULL)
		videoEncoder->open(outputFile, srcFormat.height, srcFormat.width);
    while(true)
    {
        mngr->VideoTickImpl();
        cv::Mat questImg = mngr->getMostRecentImg();
     
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
        
        cv::Mat fgMask;
        pBackSub->apply(frame, fgMask);
        
        printf("quest: %dx%d, camera %dx%d\n", questImg.cols, questImg.rows, frame.cols, frame.rows);
        if(!frame.empty())
        {
        	cv::Mat frame2;
        	cv::resize(frame, frame2, cv::Size(frame.cols/2, frame.rows/2));
        	cv::imshow("cam", frame2);
        }
        
		if(!questImg.empty()) {
			cv::Mat composedImg = composeMixedRealityImg(questImg, frame, fgMask);
			cv::imshow("composedImg", composedImg);
			if(outputFile != NULL)
				videoEncoder->write(createImageDataFromMat(composedImg, imgData2->getTimestamp(), false));
		}
        
		int key = cv::waitKey(10);
		if(key > 0)
			break;
    }
	if(outputFile != NULL)
		videoEncoder->release();
    mngr->detachSource();
    videoSrc->Disconnect();
}

int main(int argc, char** argv) 
{
	if(argc < 2) {
		printf("usage: demo-capture-RPCam ipAddr file\n");
	} else {
		//captureFromQuest(argv[1], argc >= 2 ? argv[2] : NULL);
		testRecord(argv[1], argc >= 2 ? argv[2] : NULL);
    }
    return 0;
}
