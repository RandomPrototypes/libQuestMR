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

int thresholdedSquaredDiff(int a, int b, int thresh)
{
	int diff = abs(b-a);
	if(diff > thresh)
		return thresh*thresh + diff - thresh;
	else return diff*diff;
}

std::vector<uint64_t> rectifyTimestamps(const std::vector<uint64_t>& listTimestamp)
{
	std::vector<int> list(listTimestamp.size());
	for(size_t i = 0; i < list.size(); i++)
		list[i] = static_cast<int>(listTimestamp[i] - listTimestamp[0]);

	std::vector<int> listDelta;
	for(size_t i = 1; i < list.size(); i++) {
		listDelta.push_back(list[i] - list[i-1]);
	}
	std::sort(listDelta.begin(), listDelta.end());
	int medianDelta = listDelta[listDelta.size() / 2];
	printf("medianDelta %d ms\n", medianDelta);
	std::vector<int> list2 = list;
	for(int i = 0; i < 1000; i++) {
		std::vector<int> list3 = list2;
		int totalCost = 0;
		int thresh1 = medianDelta/2;
		int thresh2 = thresh1*4;
		for(size_t j = 0; j < list.size(); j++)
		{
			int confidence = 0;
			if(j > 0)
				confidence += std::max(0, medianDelta - abs(medianDelta - (list[j] - list[j-1]))) / 2;
			if(j + 1 < list.size())
				confidence += std::max(0, medianDelta - abs(medianDelta - (list[j+1] - list[j]))) / 2;
			float cost[3];
			for(int k = 0; k < 3; k++) {
				int t = list2[j] + k - 1;
				int diff = abs(t - list[j]);
				cost[k] = thresholdedSquaredDiff(t, list[j], thresh1) * confidence / medianDelta;
				if(j > 0)
					cost[k] += thresholdedSquaredDiff(t, list2[j-1] + medianDelta, thresh2);
				if(j + 1 < list2.size())
					cost[k] += thresholdedSquaredDiff(t, list2[j+1] - medianDelta, thresh2);
			}
			if(cost[0] < cost[1] && cost[0] < cost[2])
				list3[j] = list2[j]-1;
			if(cost[2] < cost[0] && cost[2] < cost[1])
				list3[j] = list2[j]+1;
			totalCost += cost[1];
		}
		list2 = list3;
		printf("totalCost %d\n", totalCost);
	}
	std::vector<uint64_t> listTimestamp2(list2.size());
	for(size_t i = 0; i < list2.size(); i++)
		listTimestamp2[i] = static_cast<uint64_t>(list2[i] + listTimestamp[0]);
	return listTimestamp2;
}

bool loadQuestRecordedTimestamps(const char *filename, std::vector<uint64_t> *listTimestamp, std::vector<uint32_t> *listType = NULL, std::vector<uint32_t> *listSize = NULL)
{
	if(listTimestamp != NULL)
		listTimestamp->clear();
	if(listType != NULL)
		listType->clear();
	if(listSize != NULL)
		listSize->clear();
	
	std::ifstream input(filename);
	if(!input.good())
		return false;
	std::string line;
	while(std::getline(input, line))
	{
		uint64_t timestamp;
		uint32_t type;
		uint32_t size;
		std::string timestampStr, typeStr, sizeStr;
		std::istringstream str(line);
		if(!std::getline(str, timestampStr, ',') || !std::getline(str, typeStr, ',') || !std::getline(str, sizeStr, ','))
			break;

		std::istringstream(timestampStr) >> timestamp;
		std::istringstream(typeStr) >> type;
		std::istringstream(sizeStr) >> size;
		
		if(listTimestamp != NULL)
			listTimestamp->push_back(timestamp);
		if(listType != NULL)
			listType->push_back(type);
		if(listSize != NULL)
			listSize->push_back(size);
	}
	return true;
}

void rectifyTimestamps(const char *filename, const char *outputFilename)
{
	std::vector<uint64_t> listTimestamp;
	std::vector<uint32_t> listType;
	std::vector<uint32_t> listSize;
	loadQuestRecordedTimestamps(filename, &listTimestamp, &listType, &listSize);

	std::vector<uint64_t> listVideoDataTimestamp;
	std::vector<int> listVideoDataId;
	for(size_t i = 0; i < listTimestamp.size(); i++) {
		if(listType[i] == 11) {
			listVideoDataTimestamp.push_back(listTimestamp[i]);
			listVideoDataId.push_back(i);
		}
	}
	
	std::vector<uint64_t> listVideoDataTimestamp2 = rectifyTimestamps(listVideoDataTimestamp);


	std::vector<uint64_t> listTimestamp2 = listTimestamp;
	for(size_t i = 0; i < listVideoDataId.size(); i++)
		listTimestamp2[listVideoDataId[i]] = listVideoDataTimestamp2[i];

	for(size_t i = 1; i < listVideoDataId.size(); i++) {
		int id1 = listVideoDataId[i-1];
		int id2 = listVideoDataId[i];
		uint64_t delta = listTimestamp[id2] - listTimestamp[id1];
		for(int j = id1+1; j < id2; j++) {
			if(delta > 0) {
				listTimestamp2[j] = listTimestamp2[id1] + (listTimestamp[j] - listTimestamp[id1]) * (listTimestamp2[id2] - listTimestamp2[id1]) / delta;
			} else {
				listTimestamp2[j] = listTimestamp2[id1];
			}
		}
	}
	int firstVideoId = listVideoDataId[0];
	int lastVideoId = (int)listVideoDataId.size()-1;
	for(int j = 0; j < firstVideoId; j++)
		listTimestamp2[j] = listTimestamp[j] + listTimestamp2[firstVideoId] - listTimestamp[firstVideoId];
	for(int j = lastVideoId+1; j < (int)listTimestamp.size(); j++)
		listTimestamp2[j] = listTimestamp[j] + listTimestamp2[lastVideoId] - listTimestamp[lastVideoId];
	
	std::ofstream outputFile(outputFilename);
	for(size_t i = 0; i < listTimestamp.size(); i++)
		outputFile << listTimestamp2[i] << "," << listType[i] << "," << listSize[i] << "\n";
	std::ofstream outputFile2("debugCmpTimestamp.csv");
	for(size_t i = 0; i < listVideoDataTimestamp.size(); i++)
		outputFile2 << listVideoDataTimestamp[i] << "," << listVideoDataTimestamp2[i] << "\n";
}

void processRawCaptureAudio(const char *recordName)
{
	int bitrate = 8000000;

	std::shared_ptr<QuestVideoMngr> mngr;
	std::shared_ptr<QuestVideoSourceFile> videoSrc;
	mngr = createQuestVideoMngr();
	videoSrc = createQuestVideoSourceFile();
	videoSrc->open((std::string(recordName)+"_quest.questMRVideo").c_str());
	mngr->attachSource(videoSrc);
	rectifyTimestamps((std::string(recordName)+"_questTimestamp.txt").c_str(), (std::string(recordName)+"_questTimestamp2.txt").c_str());
    mngr->setRecordedTimestampSource((std::string(recordName)+"_questTimestamp2.txt").c_str());
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
				printf("sampleRate %d, timestamp %lu\n", listAudioData[i]->getSampleRate(), listAudioData[0]->getDeviceTimestamp());
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
