#include <libQuestMR/QuestVideoMngr.h>
#include <libQuestMR/QuestVideoTimestampRectifier.h>
#include "frame.h"
#include <fstream>
#include <sstream>

namespace libQuestMR
{

float thresholdedSquaredDiff(float a, float b, float thresh)
{
	float diff = fabs(b-a);
	if(diff > thresh)
		return thresh*thresh + diff - thresh;
	else return diff*diff;
}

std::vector<uint64_t> rectifyVideoTimestamps(const std::vector<uint64_t>& listTimestamp)
{
	if(listTimestamp.size() == 0)
		return listTimestamp;
	std::vector<int> list(listTimestamp.size());
	for(size_t i = 0; i < list.size(); i++)
		list[i] = static_cast<int>(listTimestamp[i] - listTimestamp[0]);

	std::vector<int> listDelta;
	for(size_t i = 1; i < list.size(); i++) {
		listDelta.push_back(list[i] - list[i-1]);
		//printf("%d\n", listDelta[listDelta.size()-1]);
	}
	std::sort(listDelta.begin(), listDelta.end());
	float medianDelta = listDelta[listDelta.size() / 2];
	//medianDelta = 23.73;//21.375;
	//printf("medianDelta %f ms\n", medianDelta);
	std::vector<int> list2 = list;
	for(int i = 0; i < 1000; i++) {
		std::vector<int> list3 = list2;
		float totalCost = 0;
		float thresh1 = medianDelta/2;
		float thresh2 = thresh1*4;
		for(size_t j = 0; j < list.size(); j++)
		{
			float confidence = 0;
			if(j > 0)
				confidence += std::max(0.0f, medianDelta - fabs(medianDelta - (list[j] - list[j-1]))) / 2;
			if(j + 1 < list.size())
				confidence += std::max(0.0f, medianDelta - fabs(medianDelta - (list[j+1] - list[j]))) / 2;
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
		//printf("totalCost %f\n", totalCost);
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
	if(listVideoDataTimestamp.size() == 0)
		return listTimestamp;
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
	/*std::ofstream outputFile2("debugCmpTimestamp.csv");
	for(size_t i = 0; i < listTimestamp.size(); i++)
		outputFile2 << listTimestamp[i] << "," << listTimestamp2[i] << "\n";*/
    return listTimestamp2;
}

int mergeCluster(int clusterId1, int clusterId2, std::vector<int> *listClusterIds, std::vector<double> *listClustersMean, std::vector<int> *listClustersSize, int *nbClusters)
{
	if(clusterId1 > clusterId2)
		return mergeCluster(clusterId2, clusterId1, listClusterIds, listClustersMean, listClustersSize, nbClusters);
	//printf("merge cluster %d and %d\n", clusterId1, clusterId2);
	for(size_t i = 0; i < listClusterIds->size(); i++) {
		if((*listClusterIds)[i] < clusterId2) {
			continue;
		} else if((*listClusterIds)[i] > clusterId2) {
			(*listClusterIds)[i]--;
		} else {
			(*listClusterIds)[i] = clusterId1;
		}
	}
	(*listClustersMean)[clusterId1] = ((*listClustersMean)[clusterId1] * (*listClustersSize)[clusterId1] + (*listClustersMean)[clusterId2] * (*listClustersSize)[clusterId2]) / ((*listClustersSize)[clusterId1] + (*listClustersSize)[clusterId2]);
	(*listClustersSize)[clusterId1] += (*listClustersSize)[clusterId2];
	for(size_t i = clusterId2; i+1 < listClustersMean->size(); i++) {
		(*listClustersMean)[i] = (*listClustersMean)[i+1];
		(*listClustersSize)[i] = (*listClustersSize)[i+1];
	}
	(*listClustersMean).pop_back();
	(*listClustersSize).pop_back();
	(*nbClusters)--;
	return clusterId1;
}

void filterClusters(std::vector<int> *listClusterIds, std::vector<double> *listClustersMean, std::vector<int> *listClustersSize, int *nbClusters, double threshold)
{
	bool restart = true;
	while(restart) {
		restart = false;
		for(size_t i = 0; !restart && i < listClustersMean->size(); i++) {
			int insideSize = 0;
			for(size_t i2 = i+1; i2 < listClustersMean->size() && (insideSize < 10 || insideSize < (*listClustersSize)[i]); i2++) {
				if(fabs((*listClustersMean)[i]-(*listClustersMean)[i2]) < threshold && (insideSize < 10 || insideSize < (*listClustersSize)[i2]) && insideSize*10 < (*listClustersSize)[i] + (*listClustersSize)[i2]) {
					for(size_t i3 = i+1; i3 <= i2; i3++)
						mergeCluster(i, i+1, listClusterIds, listClustersMean, listClustersSize, nbClusters);
					restart = true;
					break;
				} else {
					insideSize += (*listClustersSize)[i2];
				}
			}
		}
	}
}

std::vector<uint64_t> rectifyAudioTimestamps(const std::vector<uint64_t>& listLocalTimestamp, const std::vector<double>& listQuestTimestamp, const std::vector<double>& listAudioRecordedLength)
{
	std::vector<uint64_t> result(listQuestTimestamp.size());
	std::vector<double> listOffset(listQuestTimestamp.size());
	for(size_t i = 0; i < listQuestTimestamp.size(); i++) {
		listOffset[i] = listQuestTimestamp[i] - listAudioRecordedLength[i];
		//printf("offset %d : %lf\n", i, listOffset[i]);
	}
	std::vector<int> listClusterIds(listQuestTimestamp.size());
	std::vector<double> listClustersMean(1);
	std::vector<int> listClustersSize(1);
	listClusterIds[0] = 0;
	listClustersMean[0] = listOffset[0];
	listClustersSize[0] = 1;
	int nbClusters = 1;
	double threshold = 30;
	for(size_t i = 1; i < listQuestTimestamp.size(); i++) {
		if(fabs(listOffset[i] - listClustersMean[nbClusters-1]) > threshold) {
			listClustersMean.push_back(listOffset[i]);
			listClustersSize.push_back(1);
			nbClusters++;
		} else {
			listClustersMean[nbClusters-1] = (listClustersMean[nbClusters-1] * listClustersSize[nbClusters-1] + listOffset[i]) / (listClustersSize[nbClusters-1] + 1);
			listClustersSize[nbClusters-1]++;
		}
		listClusterIds[i] = nbClusters-1;
	}
	filterClusters(&listClusterIds, &listClustersMean, &listClustersSize, &nbClusters, threshold);
	/*printf("nbClusters %d\n", nbClusters);
	for(int i = 0; i < nbClusters; i++) {
		printf("cluster %d, mean %lf, size %d\n", i, listClustersMean[i], listClustersSize[i]);
	}*/
	for(size_t i = 0; i < listClustersMean.size(); i++)
		listClustersMean[i] = 0;
	for(size_t i = 0; i < listLocalTimestamp.size(); i++)
		listClustersMean[listClusterIds[i]] += (listLocalTimestamp[i] - listAudioRecordedLength[i]);
	for(size_t i = 0; i < listClustersMean.size(); i++)
		listClustersMean[i] /= listClustersSize[i];
	if(nbClusters > 1 && listClustersSize[0] == 1) {
		listClusterIds[0] = 1; //the first audio timestamp seems to be wrong based on the captured data
		listClustersSize[0] = 0;
		listClustersSize[1]++;
	}
	for(size_t i = 0; i < listLocalTimestamp.size(); i++) {
		result[i] = listClustersMean[listClusterIds[i]] + listAudioRecordedLength[i] + 0.5;
		//printf("%lf\n", listAudioRecordedLength[i]);
		if(i > 0 && fabs(result[i]-result[i-1])>1000) {
			//printf("error %d\ncluster %d, mean %lf, accLength %lf\n", i, listClusterIds[i], listClustersMean[listClusterIds[i]], listAudioRecordedLength[i]);
		}
	}
	return result;
}

std::vector<uint64_t> rectifyTimestamps(const std::vector<uint64_t>& listTimestamp, const std::vector<uint32_t>& listType, const std::vector<uint32_t>& listDataLength, const std::vector<std::vector<std::string> >& listExtraData)
{
	int audioHeaderSize = 16;
	int nbChannels = 2;
	int audioSamplingRate = 48000;
	std::vector<uint64_t> listAudioDataTimestamp;
	std::vector<double> listAudioDataQuestTimestamp;
	std::vector<uint32_t> listAudioDataLength;
	std::vector<double> listAudioRecordedLength;
	std::vector<int> listAudioDataId;
	listAudioRecordedLength.push_back(0);
	for(size_t i = 0; i < listTimestamp.size(); i++) {
		if((Frame::PayloadType)listType[i] == Frame::PayloadType::AUDIO_DATA) {
			listAudioDataTimestamp.push_back(listTimestamp[i]);
			listAudioDataLength.push_back(listDataLength[i]);
			if(i < listExtraData.size() && listExtraData[i].size() > 0) {
				uint64_t questTimestamp;
				std::istringstream(listExtraData[i][0]) >> questTimestamp;
				std::istringstream(listExtraData[i][1]) >> nbChannels;
				listAudioDataQuestTimestamp.push_back(questTimestamp/1000.0);
				//printf("questTimestamp: %s = %llu\n", listExtraData[i][0].c_str(), (unsigned long long)questTimestamp);
			} else {
				return listTimestamp;
			}
			double recordedLength = listAudioRecordedLength[listAudioRecordedLength.size()-1];
			double length = static_cast<double>(listDataLength[i] - audioHeaderSize) * (1000.0 / (nbChannels*4*audioSamplingRate)); //16 bits header, 1s = 1000 ms, 2 channels, 4 bytes, 48000 hz
			listAudioRecordedLength.push_back(recordedLength+length);
			listAudioDataId.push_back(i);
		} else if((Frame::PayloadType)listType[i] == Frame::PayloadType::AUDIO_SAMPLERATE) {
			if(i < listExtraData.size() && listExtraData[i].size() > 0) {
				std::istringstream(listExtraData[i][0]) >> audioSamplingRate;
				//printf("audio sample rate: %d\n", audioSamplingRate);
			}
		}
	}

	if(listAudioDataTimestamp.size() < listTimestamp.size()/10)
		return rectifyTimestamps(listTimestamp, listType);//if we don't have enough audio timestamps, we use other method
	std::vector<uint64_t> listAudioDataTimestamp2 = rectifyAudioTimestamps(listAudioDataTimestamp, listAudioDataQuestTimestamp, listAudioRecordedLength);
	
	std::vector<uint64_t> listTimestamp2 = listTimestamp;
	for(size_t i = 0; i < listAudioDataId.size(); i++)
		listTimestamp2[listAudioDataId[i]] = listAudioDataTimestamp2[i];
	for(size_t i = 1; i < listAudioDataId.size(); i++) {
		int id1 = listAudioDataId[i-1];
		int id2 = listAudioDataId[i];
		uint64_t delta = listTimestamp[id2] - listTimestamp[id1];
		for(int j = id1+1; j < id2; j++) {
			/*if(delta > 0) {
				listTimestamp2[j] = listTimestamp2[id1] + (listTimestamp[j] - listTimestamp[id1]) * (listTimestamp2[id2] - listTimestamp2[id1]) / delta;
			} else*/ {
				listTimestamp2[j] = listTimestamp2[id1];
			}
		}
	}
	uint64_t lastTimestamp = listAudioDataTimestamp2[listAudioDataTimestamp2.size()-1];
	for(size_t i = listAudioDataId[listAudioDataId.size()-1]+1; i < listTimestamp2.size(); i++) {
		listTimestamp2[i] = lastTimestamp;
		if((Frame::PayloadType)listType[i] == Frame::PayloadType::VIDEO_DATA)
			lastTimestamp += 23;
	}

	/*{
		std::ofstream outputFile2("debugCmpTimestamp.csv");
		for(size_t i = 0; i < listTimestamp.size(); i++)
			//if((Frame::PayloadType)listType[i] == Frame::PayloadType::VIDEO_DATA)
				outputFile2 << listTimestamp[i] << "," << listTimestamp2[i] << "\n";
	}*/
	//exit(0);
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

