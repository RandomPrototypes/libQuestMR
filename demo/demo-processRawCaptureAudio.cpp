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
	double accAudioLength = 0;

	double firstTimestamp = -1;

	while(videoSrc->isValid())
	{
		cv::Mat questImg;
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

		if(!firstFrame && quest_timestamp <= last_timestamp) {
			printf("skip frame, frame %d %d, timestamp %llu %llu\n", last_frameId, frameId, (unsigned long long)last_timestamp, (unsigned long long)quest_timestamp);
			last_frameId = frameId;
			continue;
		}

		cv::Mat img = questImg(cv::Rect(0,0,questImg.cols/2,questImg.rows)).clone();

		cv::imshow("img", img);

		if(firstFrame)
			videoEncoder->open("testResultWithAudio.mp4", img.rows, img.cols, 30, "", bitrate);
		for(int i = 0; i < nbAudioFrames; i++) {
			std::vector<float> audioData = remainingAudio;
			remainingAudio.clear();
			int size = listAudioData[i]->getDataLength() / sizeof(float);
			const float *data = (const float*)listAudioData[i]->getData();
			uint32_t sampleRate = listAudioData[i]->getSampleRate();
			int nbChannels = listAudioData[i]->getNbChannels();
			int nbSamples = (size*48000/sampleRate)/nbChannels;
			if(nbChannels == 1) {
				for(int j = 0; j < 2*nbSamples; j++)
					audioData.push_back(data[(j/2)*sampleRate/48000]);
			} else if(sampleRate != 48000) {
				for(int j = 0; j < nbSamples; j++) {
					audioData.push_back(data[(j*sampleRate/48000)*2]);
					audioData.push_back(data[(j*sampleRate/48000)*2+1]);
				}
			} else {
				for(int j = 0; j < size; j++)
					audioData.push_back(data[j]);
			}
			int nbAudioPacket = audioData.size() / 2048;
			videoEncoder->write_audio(&audioData[0], nbAudioPacket * 2048, listAudioData[i]->getLocalTimestamp());
			audioData.erase(audioData.begin(), audioData.begin() + nbAudioPacket * 2048);
			accAudioLength += static_cast<double>(size)*1000/(2*48000);
			remainingAudio = audioData;
		}
		videoEncoder->write(createImageDataFromMat(img, quest_timestamp, false));

		int key = cv::waitKey(10);
		if(key > 0)
			break;
		firstFrame = false;
		last_timestamp = quest_timestamp;
		last_frameId = frameId;
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
