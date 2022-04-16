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

void captureFromQuest(const char *ipAddr, const char *outputFile)
{
	int mask_subsample_factor = 2;

	std::shared_ptr<CameraInterface> cam;
	ImageFormat srcFormat;
	if(!configureCamera(cam, &srcFormat)) {
		printf("error configuring camera!!\n");
		return ;
	}
	
	printf("Choose background subtraction method:\n");
	for(int i = 0; i < getBackgroundSubtractorCount(); i++)
		printf("%d: %s\n", i, getBackgroundSubtractorName(i).c_str());
	int bgMethodId = -1;
	while(bgMethodId < 0 || bgMethodId >= getBackgroundSubtractorCount())
	{
		printf("Which method? ");
		scanf("%d", &bgMethodId);
	}
	//std::shared_ptr<libQuestMR::BackgroundSubtractor> backgroundSub = createBackgroundSubtractorOpenCV(cv::createBackgroundSubtractorMOG2());
	//std::shared_ptr<libQuestMR::BackgroundSubtractor> backgroundSub = createBackgroundSubtractorChromaKey(20, 50, true);
	//std::shared_ptr<libQuestMR::BackgroundSubtractor> backgroundSub = createBackgroundSubtractorRobustVideoMattingONNX("../rvm_mobilenetv3_fp32.onnx", true);
	std::shared_ptr<libQuestMR::BackgroundSubtractor> backgroundSub = createBackgroundSubtractor(bgMethodId);
	
    std::shared_ptr<QuestVideoMngr> mngr = createQuestVideoMngr();
    std::shared_ptr<QuestVideoSourceBufferedSocket> videoSrc = createQuestVideoSourceBufferedSocket();
    if(!videoSrc->Connect(ipAddr)) {
    	printf("can not connect to quest\n");
    	return;
    }
    mngr->attachSource(videoSrc);

	printf("create thread...\n");
	std::shared_ptr<QuestVideoMngrThreadData> questVideoMngrThreadData = createQuestVideoMngrThreadData(mngr);
	std::thread questVideoMngrThread(QuestVideoMngrThreadFunc, questVideoMngrThreadData.get());

    
    
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
		uint64_t timestamp;
        cv::Mat questImg = questVideoMngrThreadData->getMostRecentImg(&timestamp);
     
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
		if(mask_subsample_factor > 1) {
			cv::Mat frameLow;
			cv::resize(frame, frameLow, cv::Size(frame.cols/mask_subsample_factor, frame.rows/mask_subsample_factor));
			backgroundSub->apply(frameLow, fgMask);
			cv::resize(fgMask, fgMask, frame.size());
		} else {
			backgroundSub->apply(frame, fgMask);
		}

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
	questVideoMngrThreadData->setFinishedVal(true);
	questVideoMngrThread.join();
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
		captureFromQuest(argv[1], argc >= 3 ? argv[2] : NULL);
    }
    return 0;
}
