#include <libQuestMR/QuestVideoMngr.h>
#include <libQuestMR/QuestVideoTimestampRectifier.h>
#include "frame.h"
#include <fstream>

namespace libQuestMR
{

int thresholdedSquaredDiff(int a, int b, int thresh)
{
	int diff = abs(b-a);
	if(diff > thresh)
		return thresh*thresh + diff - thresh;
	else return diff*diff;
}

std::vector<uint64_t> rectifyVideoTimestamps(const std::vector<uint64_t>& listTimestamp)
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
	//printf("medianDelta %d ms\n", medianDelta);
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
		//printf("totalCost %d\n", totalCost);
	}
	std::vector<uint64_t> listTimestamp2(list2.size());
	for(size_t i = 0; i < list2.size(); i++)
		listTimestamp2[i] = static_cast<uint64_t>(list2[i] + listTimestamp[0]);
	return listTimestamp2;
}

std::vector<uint64_t> rectifyTimestamps(const std::vector<uint64_t>& listTimestamp, const std::vector<uint32_t>& listType)
{
    std::vector<uint64_t> listVideoDataTimestamp;
	std::vector<int> listVideoDataId;
	for(size_t i = 0; i < listTimestamp.size(); i++) {
		if((Frame::PayloadType)listType[i] == Frame::PayloadType::VIDEO_DATA) {
			listVideoDataTimestamp.push_back(listTimestamp[i]);
			listVideoDataId.push_back(i);
		}
	}
    std::vector<uint64_t> listVideoDataTimestamp2 = rectifyVideoTimestamps(listVideoDataTimestamp);

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
    return listTimestamp2;
}

void rectifyTimestamps(const char *filename, const char *outputFilename)
{
	std::vector<uint64_t> listTimestamp;
	std::vector<uint32_t> listType;
	std::vector<uint32_t> listSize;
	loadQuestRecordedTimestamps(filename, &listTimestamp, &listType, &listSize);

    std::vector<uint64_t> listTimestamp2 = rectifyTimestamps(listTimestamp, listType);
	
	std::ofstream outputFile(outputFilename);
	for(size_t i = 0; i < listTimestamp.size(); i++)
		outputFile << listTimestamp2[i] << "," << listType[i] << "," << listSize[i] << "\n";
	//std::ofstream outputFile2("debugCmpTimestamp.csv");
	//for(size_t i = 0; i < listVideoDataTimestamp.size(); i++)
	//	outputFile2 << listVideoDataTimestamp[i] << "," << listVideoDataTimestamp2[i] << "\n";
}

}

