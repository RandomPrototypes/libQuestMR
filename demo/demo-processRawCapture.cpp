#include <stdio.h>
#include <thread>

#include <libQuestMR/QuestVideoMngr.h>
#include <libQuestMR/BackgroundSubtractor.h>
#include <RPCameraInterface/ImageFormatConverter.h>
#include <RPCameraInterface/OpenCVConverter.h>
#include <RPCameraInterface/VideoEncoder.h>
#include "RPCam_helper.h"
#include <fstream>

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

std::vector<cv::Point> selectShape(const char *windowName, cv::Mat img)
{
	cv::namedWindow(windowName);
	std::vector<cv::Point> list;
	bool wasLeftDown = false;
	MouseData mouseData;
    cv::setMouseCallback(windowName, MouseCallBackFunc, &mouseData);

	while(true) {
		if(mouseData.leftDown)
			wasLeftDown = true;
		else if(wasLeftDown) {
			list.push_back(cv::Point(mouseData.x, mouseData.y));
			wasLeftDown = false;
		}
		cv::Mat img2 = img.clone();
		cv::putText(img2, "select the play area", cv::Point(20, 20),  cv::FONT_HERSHEY_DUPLEX, 0.7, cv::Scalar(0, 0, 255), 2);
		for(size_t i = 0; i < list.size(); i++)
		{
			cv::circle(img2, list[i], 5, cv::Scalar(0,0,255), 2);
			if(i+1 < list.size())
				cv::line(img2, list[i], list[i+1], cv::Scalar(0,0,255), 2);
			else cv::line(img2, list[i], cv::Point(mouseData.x, mouseData.y), cv::Scalar(0,0,255), 2);
		}
		if(list.size() >= 3)
			cv::line(img2, list[0], cv::Point(mouseData.x, mouseData.y), cv::Scalar(0,0,255), 2);
		cv::circle(img2, cv::Point(mouseData.x, mouseData.y), 5, cv::Scalar(0,255,0), 2);
		cv::imshow(windowName, img2);
		int key = cv::waitKey(10);
		if(list.size() >= 4 && key > 0)
			break;
	}
	cv::setMouseCallback(windowName, NULL, NULL);
	return list;
}

static void on_trackbar( int, void* )
{
}

void processRawCapture(const char *recordName, const char *outputVideo)
{
	int mask_subsample_factor = 1;
	int bitrate = 8000000;

	bool removeGameBackground = false;
	unsigned char gameBackgroundR = 52, gameBackgroundG = 75, gameBackgroundB = 115;
	int threshGameBackground = 20;

	printf("Choose background subtraction method:\n");
	for(int i = 0; i < getBackgroundSubtractorCount(); i++)
		printf("%d: %s\n", i, getBackgroundSubtractorName(i).c_str());
	int bgMethodId = -1;
	while(bgMethodId < 0 || bgMethodId >= getBackgroundSubtractorCount())
	{
		printf("Which method? ");
		scanf("%d", &bgMethodId);
	}
	std::shared_ptr<libQuestMR::BackgroundSubtractor> backgroundSub = createBackgroundSubtractor(bgMethodId);

	std::shared_ptr<VideoEncoder> videoEncoder;
	if(outputVideo != NULL) {
		videoEncoder = createVideoEncoder();
		videoEncoder->setUseFrameTimestamp(true);
	}
	std::ifstream timestampFile((std::string(recordName)+"_timestamp.txt").c_str());
	cv::VideoCapture cap_quest((std::string(recordName)+"_quest.mp4").c_str());
	cv::VideoCapture cap_cam((std::string(recordName)+"_cam.mp4").c_str());

	std::vector<cv::Point> border;
	cv::Mat maskBorder;

	std::string timestampStr;

	cv::namedWindow("composedImg", cv::WINDOW_AUTOSIZE);
	std::vector<int> sliderVal(backgroundSub->getParameterCount());
	for(int i = 0; i < backgroundSub->getParameterCount(); i++) {
		if(backgroundSub->getParameterType(i) == libQuestMR::BackgroundSubtractorParamType::ParamTypeInt) {
			sliderVal[i] = backgroundSub->getParameterValAsInt(i);
			int maxVal = 100;
			if(sliderVal[i] > 75)
				maxVal = 255;
			if(sliderVal[i] >= 255)
				maxVal = 1000;
			cv::createTrackbar( backgroundSub->getParameterName(i).str().c_str(), "composedImg", &sliderVal[i], maxVal, on_trackbar );
		}
	}
	bool firstFrame = true;
	uint64_t last_timestamp = 0;
	while(std::getline(timestampFile, timestampStr))
	{
		uint64_t timestamp;
		std::istringstream iss(timestampStr);
		iss >> timestamp;
		printf("%s\n", timestampStr.c_str());

		for(int i = 0; i < backgroundSub->getParameterCount(); i++) {
			if(backgroundSub->getParameterType(i) == libQuestMR::BackgroundSubtractorParamType::ParamTypeInt) {
				backgroundSub->setParameterVal(i, sliderVal[i]);
			}
		}

		cv::Mat questImg, frame;
		cap_quest >> questImg;
		cap_cam >> frame;

		if(!firstFrame && timestamp <= last_timestamp)
			continue;

		if(frame.empty() || questImg.empty())
			break;

		if(firstFrame) {
			border = selectShape("composedImg", frame.clone());
			maskBorder = cv::Mat::zeros(frame.size(), CV_8UC1);
			const cv::Point* ppt[1] = { &border[0] };
			int npt[] = { (int)border.size() };
			cv::fillPoly(maskBorder, ppt, npt, 1, cv::Scalar( 255 ), cv::LINE_8);
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
		for(int i = 0; i < fgMask.rows; i++)
		{
			unsigned char *dst = fgMask.ptr<unsigned char>(i);
			unsigned char *src = maskBorder.ptr<unsigned char>(i);
			for(int j = 0; j < fgMask.cols; j++) {
				if(src[j] == 0)
					dst[j] = 0;
			}
		}
		if(removeGameBackground) {
			for(int i = 0; i < fgMask.rows && i < questImg.rows; i++)
			{
				unsigned char *dst = fgMask.ptr<unsigned char>(i);
				unsigned char *src = questImg.ptr<unsigned char>(i);
				for(int j = 0; j < fgMask.cols && j < questImg.cols; j++) {
					if(abs(src[j*3] - gameBackgroundB) < threshGameBackground
					&& abs(src[j*3+1] - gameBackgroundG) < threshGameBackground
					&& abs(src[j*3+2] - gameBackgroundR) < threshGameBackground)
						dst[j] = 255;
				}
			}
		}

		cv::Mat composedImg = composeMixedRealityImg(questImg, frame, fgMask);
		cv::imshow("composedImg", composedImg);

		if(outputVideo != NULL) {
			if(firstFrame)
				videoEncoder->open(outputVideo, composedImg.rows, composedImg.cols, 30, "", bitrate);
			videoEncoder->write(createImageDataFromMat(composedImg, timestamp, false));
		}
		int key = cv::waitKey(10);
		if(key > 0)
			break;
		firstFrame = false;
		last_timestamp = timestamp;
    }
	if(outputVideo != NULL)
		videoEncoder->release();
}

int main(int argc, char** argv) 
{
	if(argc < 2) {
		printf("usage: demo-processRawCapture recordName outputVideo\n");
	} else {
		processRawCapture(argv[1], argc >= 3 ? argv[2] : NULL);
    }
    return 0;
}
