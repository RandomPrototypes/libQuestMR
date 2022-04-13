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

    std::shared_ptr<QuestCommunicator> questCom = createQuestCommunicator();
    if(!questCom->connect(ipAddr)) {
    	printf("Can not connect to Oculus Mixed Reality Capture app!!\nPlease verify that the app is opened on your quest and the ip address is correct!!\n");
    	return ;
    }
    QuestCommunicatorMessage message;
    message.data = calibData.generateXMLString();
    message.type = QuestMessageTypeId::calibrationData;
    if(questCom->sendMessage(message)) {
        printf("Uploading calibration data...\n");
    } else {
        printf("Can not send message to Oculus Mixed Reality Capture app!!\n");
        return ;
    }
    while(true)
    {
        QuestCommunicatorMessage message;
        if(!questCom->readMessage(&message)) {
        	printf("can not read message!!\n");
            break;
        }
        if(message.type == QuestMessageTypeId::operationComplete) {
            printf("Calibration data successfully uploaded\n");
            return ;
        }
    }
}

int main(int argc, char** argv)
{
	if(argc < 3) {
		printf("usage: demo-uploadQuestCalib ipAddr calib_file\n");
	} else {
		uploadQuestCalib(argv[1], argv[2]);
    }
    return 0;
}
