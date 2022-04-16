#include <stdio.h>

#include <libQuestMR/QuestVideoMngr.h>
#include <libQuestMR/BackgroundSubtractor.h>
#include <RPCameraInterface/ImageFormatConverter.h>
#include <RPCameraInterface/OpenCVConverter.h>
#include <RPCameraInterface/VideoEncoder.h>
#include "RPCam_helper.h"


using namespace libQuestMR;
using namespace RPCameraInterface;

void demoSegmentation(const char *outputFile)
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

		cv::Mat frameYCrCb;
		cv::cvtColor(frame, frameYCrCb, cv::COLOR_BGR2YCrCb);
		frameYCrCb = frameYCrCb(cv::Rect(frameYCrCb.cols/4,frameYCrCb.rows/4,frameYCrCb.cols/2,frameYCrCb.rows/2));
		int mean_cr = 0, mean_cb = 0;
		for(int i = 0; i < frameYCrCb.rows; i++) {
			unsigned char *ptr = frameYCrCb.ptr<unsigned char>(i);
			for(int j = 0; j < frameYCrCb.cols; j++) {
				mean_cr += ptr[1];
				mean_cb += ptr[2];
				ptr += 3;
			}
		}
		mean_cr /= frameYCrCb.cols * frameYCrCb.rows;
		mean_cb /= frameYCrCb.cols * frameYCrCb.rows;
		printf("mean cr: %d, cb: %d\n", mean_cr, mean_cb);

		cv::Mat fgMask;
        backgroundSub->apply(frame, fgMask);

		printf("timestamp %ld\n", imgData2->getTimestamp());

		if(outputFile != NULL)
			videoEncoder->write(createImageDataFromMat(frame, imgData2->getTimestamp(), false));
		cv::imshow("img", fgMask);
		int key = cv::waitKey(10);
		if(key > 0)
			break;
	}
	if(outputFile != NULL)
		videoEncoder->release();
}

int main(int argc, char **argv) 
{
	demoSegmentation(argc >= 2 ? argv[1] : NULL);
    return 0;
}
