#include <iostream>
#include <string.h>
#include <vector>
#include <sstream>
#include <thread>
#include <queue>

#include <libQuestMR/QuestCalibData.h>
#include <libQuestMR/QuestCommunicator.h>

using namespace libQuestMR;


void loadAndUploadQuestCalib(const char *ipAddr)
{
    std::shared_ptr<QuestCommunicator> questCom = createQuestCommunicator();
    if(!questCom->connect(ipAddr)) {
    	printf("Can not connect to Oculus Mixed Reality Capture app!!\nPlease verify that the app is opened on your quest and the ip address is correct!!\n");
    	return ;
    }

    QuestCalibData calibData;
    bool gotCalibData = false;
    int count = 20;
    while(true)
    {
        QuestCommunicatorMessage message;
        if(!questCom->readMessage(&message)) {
        	printf("can not read message!!\n");
            break;
        }
        int size = static_cast<int>(message.data.size());
        //if(message.type == QuestMessageTypeId::poseUpdate || message.type == QuestMessageTypeId::dataVersion)
        //	continue;
        if(gotCalibData) {
            count--;
            if(count == 0) {
                QuestCommunicatorMessage message;
                message.data = calibData.generateXMLString();
                message.type = QuestMessageTypeId::calibrationData;
                printf("send calib data :\n %s\n\n", message.data.c_str());
                if(questCom->sendMessage(message)) {
                    printf("Uploading calibration data...\n");
                } else {
                    printf("Can not send message to Oculus Mixed Reality Capture app!!\n");
                    return ;
                }
            }
        }

        printf("message type: %s (%u) size:%d\n", message.getTypeStr(), message.type, size);
        if(message.type == QuestMessageTypeId::poseUpdate || message.type == QuestMessageTypeId::dataVersion) {
            continue;
        } else if(message.type == QuestMessageTypeId::calibrationData) {
            printf("calib data :\n %s\n\n", message.data.c_str());
            calibData.loadXMLString(message.data.c_str());
            gotCalibData = true;
        }
        if(message.type == QuestMessageTypeId::operationComplete) {
            printf("Calibration data successfully uploaded\n");
            return ;
        }
    }
}

int main(int argc, char** argv)
{
    std::string ip_addr;
    std::cout << "ip address:\n";
    std::cin >> ip_addr;
	loadAndUploadQuestCalib(ip_addr.c_str());
    std::cout << "press enter to exit:\n";
    std::cin.get();
    while(std::cin.get() != '\n')
        ;
    return 0;
}
