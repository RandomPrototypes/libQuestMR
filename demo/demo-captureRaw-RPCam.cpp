#include <stdio.h>
#include <thread>

#include <libQuestMR/QuestVideoMngr.h>
#include <libQuestMR/BackgroundSubtractor.h>
#include <RPCameraInterface/ImageFormatConverter.h>
#include <RPCameraInterface/OpenCVConverter.h>
#include <RPCameraInterface/VideoEncoder.h>
#include "RPCam_helper.h"

using namespace libQuestMR;
using namespace RPCameraInterface;

void captureFromQuest(const char *ipAddr, const char *recordName)
{
	bool recordDirectlyFromQuest = true;

	int bitrate = 5000000;

	std::shared_ptr<CameraInterface> cam;
	ImageFormat srcFormat;
	if(!configureCamera(cam, &srcFormat)) {
		printf("error configuring camera!!\n");
		return ;
	}
	
    std::shared_ptr<QuestVideoMngr> mngr = createQuestVideoMngr();
    std::shared_ptr<QuestVideoSourceBufferedSocket> videoSrc = createQuestVideoSourceBufferedSocket();
    if(!videoSrc->Connect(ipAddr)) {
    	printf("can not connect to quest\n");
    	return;
    }
    mngr->attachSource(videoSrc);
	if(recordDirectlyFromQuest)
		mngr->setRecording(".", recordName);

	std::shared_ptr<QuestVideoMngrThreadData> questVideoMngrThreadData = createQuestVideoMngrThreadData(mngr);
	std::thread questVideoMngrThread(QuestVideoMngrThreadFunc, questVideoMngrThreadData.get());

	//Choose the conversion format and initialize the converter
    ImageFormat dstFormat(ImageType::BGR24, srcFormat.width, srcFormat.height);
    ImageFormatConverter converter(srcFormat, dstFormat);
    
    std::shared_ptr<ImageData> imgData2 = createImageData();

	std::shared_ptr<VideoEncoder> videoEncoder_quest;
	if(!recordDirectlyFromQuest)
		videoEncoder_quest = createVideoEncoder();
	std::shared_ptr<VideoEncoder> videoEncoder_cam = createVideoEncoder();
	videoEncoder_cam->setUseFrameTimestamp(false);
	FILE *timestampFile = NULL;
	bool init = false;

    while(true)
    {     
     	//Obtain the frame
        std::shared_ptr<ImageData> imgData = cam->getNewFrame(true);
		ImageFormat imgFormat = imgData->getImageFormat();
		//Conver to the output format (BGR 720x480)
        converter.convertImage(imgData, imgData2);
		cv::Mat frame(imgData2->getImageFormat().height, imgData2->getImageFormat().width, CV_8UC3, imgData2->getDataPtr());


		uint64_t timestamp;
        cv::Mat questImg = questVideoMngrThreadData->getMostRecentImg(&timestamp);

        printf("quest: %dx%d, camera %dx%d\n", questImg.cols, questImg.rows, imgFormat.width, imgFormat.height);

		if(recordName != NULL && !questImg.empty() && imgFormat.width > 0) {
			if(!init) {
				if(!recordDirectlyFromQuest)
					videoEncoder_quest->open((std::string(recordName)+"_quest.mp4").c_str(), questImg.rows, questImg.cols, 30, "", bitrate);
				videoEncoder_cam->open((std::string(recordName)+"_cam.mp4").c_str(), srcFormat.height, srcFormat.width, 30, "", bitrate);
				timestampFile = fopen((std::string(recordName)+"_camTimestamp.txt").c_str(), "w");
				init = true;
			} else {
				if(!recordDirectlyFromQuest)
					videoEncoder_quest->write(createImageDataFromMat(questImg, imgData->getTimestamp(), false));
				videoEncoder_cam->write(imgData2);
				fprintf(timestampFile, "%s\n", std::to_string(imgData->getTimestamp()).c_str());
			}
		}

		if(!frame.empty())
        {
        	cv::Mat frame2;
        	cv::resize(frame, frame2, cv::Size(frame.cols/2, frame.rows/2));
        	cv::imshow("cam", frame2);
        }

		if(!questImg.empty())
        {
        	cv::Mat questImg2;
        	cv::resize(questImg, questImg2, cv::Size(questImg.cols/2, questImg.rows/2));
        	cv::imshow("questImg", questImg);
        }

		int key = cv::waitKey(10);
		if(key > 0)
			break;
    }
	questVideoMngrThreadData->setFinishedVal(true);
	questVideoMngrThread.join();
	if(init) {
		if(!recordDirectlyFromQuest)
			videoEncoder_quest->release();
		videoEncoder_cam->release();
		fclose(timestampFile);
	}
    mngr->detachSource();
    videoSrc->Disconnect();
}

int main(int argc, char** argv) 
{
	if(argc < 3) {
		printf("usage: demo-captureRaw-RPCam ipAddr recordName\n");
	} else {
		captureFromQuest(argv[1], argv[2]);
    }
    return 0;
}
