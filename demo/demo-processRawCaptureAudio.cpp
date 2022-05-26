#include <stdio.h>
#include <thread>

#include <libQuestMR/QuestVideoMngr.h>
#include <libQuestMR/QuestVideoTimestampRectifier.h>
#include <libQuestMR/BackgroundSubtractor.h>
#include <RPCameraInterface/ImageFormatConverter.h>
#include <RPCameraInterface/OpenCVConverter.h>
#include <RPCameraInterface/VideoEncoder.h>
#include "RPCam_helper.h"
#include <fstream>

using namespace libQuestMR;
using namespace RPCameraInterface;

void processRawCaptureAudio(const char *recordName)
{
	int bitrate = 8000000;

	std::shared_ptr<QuestVideoMngr> mngr;
	std::shared_ptr<QuestVideoSourceFile> videoSrc;
	mngr = createQuestVideoMngr();
	videoSrc = createQuestVideoSourceFile();
	videoSrc->open((std::string(recordName)+".questMRVideo").c_str());
	mngr->attachSource(videoSrc);
	//rectifyTimestamps((std::string(recordName)+"_questTimestamp.txt").c_str(), (std::string(recordName)+"_questTimestamp2.txt").c_str());
    mngr->setRecordedTimestampFile((std::string(recordName)+"_questTimestamp.txt").c_str(), true);
	//return ;

	std::shared_ptr<VideoEncoder> videoEncoder = createVideoEncoder();
	videoEncoder->setUseFrameTimestamp(true);

	bool firstFrame = true;
	uint64_t last_timestamp = 0;
	int last_frameId = -1;

	std::vector<float> remainingAudio;

	while(videoSrc->isValid())
	{
		cv::Mat questImg;
		printf("VideoTickImpl\n");
		uint64_t quest_timestamp;
		int frameId = last_frameId;
		while(frameId == last_frameId && videoSrc->isValid()) {
			mngr->VideoTickImpl();
			questImg = mngr->getMostRecentImg(&quest_timestamp, &frameId);
		}
		if(!videoSrc->isValid())
			break;
		QuestAudioData **listAudioData;
		int nbAudioFrames = mngr->getMostRecentAudio(&listAudioData);
		if(questImg.empty())
			continue;

		if(!firstFrame && quest_timestamp <= last_timestamp)
			continue;

		cv::Mat img = questImg(cv::Rect(0,0,questImg.cols/2,questImg.rows)).clone();

		cv::imshow("img", img);

		if(firstFrame)
			videoEncoder->open("testResultWithAudio.mp4", img.rows, img.cols, 30, "", bitrate);
		std::vector<float> audioData = remainingAudio;
		remainingAudio.clear();
		int recordSampleRate = 44100;
		if(nbAudioFrames > 0){
			for(int i = 0; i < nbAudioFrames; i++) {
				int size = listAudioData[i]->getDataLength() / sizeof(float);
				const float *data = (const float*)listAudioData[i]->getData();
				printf("sampleRate %d, timestamp %s\n", listAudioData[i]->getSampleRate(), std::to_string(listAudioData[0]->getDeviceTimestamp()).c_str());
				int size2 = size;// * recordSampleRate / listAudioData[i]->getSampleRate();
				for(int j = 0; j < size2; j++)
					audioData.push_back(data[j*size/size2]);
			}
			
			printf("write_audio\n");
			int nbAudioPacket = audioData.size() / 2048;
			videoEncoder->write_audio(&audioData[0], nbAudioPacket * 2048, listAudioData[0]->getLocalTimestamp());
			audioData.erase(audioData.begin(), audioData.begin() + nbAudioPacket * 2048);
		}
		remainingAudio = audioData;
		printf("write_video\n");
		videoEncoder->write(createImageDataFromMat(img, quest_timestamp, false));

		int key = cv::waitKey(10);
		if(key > 0)
			break;
		firstFrame = false;
		last_timestamp = quest_timestamp;
    }
	printf("release\n");
	videoEncoder->release();
}

int main(int argc, char** argv) 
{
	if(argc < 2) {
		printf("usage: demo-processRawCaptureAudio recordName\n");
	} else {
		processRawCaptureAudio(argv[1]);
    }
    return 0;
}
