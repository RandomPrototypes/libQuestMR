#include <iostream>
#include <unistd.h>
#include <string.h>
#include <vector>
#include <sstream>
#include <thread>
#include <queue>

#include <libQuestMR/QuestVideoMngr.h>
#include <libQuestMR/QuestCalibData.h>
#include <libQuestMR/QuestFrameData.h>
#include <libQuestMR/QuestCommunicator.h>
#include <libQuestMR/QuestStringUtil.h>

using namespace libQuestMR;


void loadQuestCalib(const char *ipAddr, const char *recordFilename)
{
    QuestCommunicator questCom;
    if(!questCom.connect(ipAddr, 25671))
    {
        printf("can not connect to the quest\n");
        return ;
    }

    QuestCommunicatorThreadData questComData(&questCom);
    std::thread questComThread(QuestCommunicatorThreadFunc, &questComData);

    while(true)
    {
        std::string calibData = questComData.getCalibData();
        if(!calibData.empty())
        {
            FILE *file = fopen(recordFilename, "w");
            if(file) {
            	fwrite(calibData.c_str(), 1, calibData.size(), file);
            	fclose(file);
            	printf("calibration data saved to %s!!\n", recordFilename);
            } else {
            	printf("can not open file %s!!\n", recordFilename);
            }
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    questComData.setFinishedVal(true);
    questComThread.join();
}

int main(int argc, char** argv)
{
	if(argc < 2) {
		printf("usage: demo-loadQuestCalib ipAddr output_file\n");
	} else {
		loadQuestCalib(argv[1], argv[2]);
    }
    return 0;
}
