#include <iostream>
#include <string.h>
#include <vector>
#include <sstream>
#include <thread>
#include <queue>

#include <libQuestMR/QuestCalibData.h>
#include <libQuestMR/QuestCommunicator.h>

using namespace libQuestMR;


void uploadQuestCalib(const char *ipAddr, const char *calibFilename)
{
	QuestCalibData calibData;
    calibData.loadXMLFile(calibFilename);
    
    QuestCommunicator questCom;
    if(!questCom.connect(ipAddr, 25671))
    {
        printf("can not connect to the quest\n");
        return ;
    }

    QuestCommunicatorThreadData questComData(&questCom);
    std::thread questComThread(QuestCommunicatorThreadFunc, &questComData);
    
    //request upload of the calibration data
    questComData.sendCalibDataToQuest(calibData.generateXMLString());

	//wait until the calibration data is uploaded
    while(!questComData.isCalibDataUploaded())
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    printf("calibration data uploaded\n");
    questComData.setFinishedVal(true);
    questComThread.join();
}

int main(int argc, char** argv)
{
	if(argc < 2) {
		printf("usage: demo-uploadQuestCalib ipAddr calib_file\n");
	} else {
		uploadQuestCalib(argv[1], argv[2]);
    }
    return 0;
}
