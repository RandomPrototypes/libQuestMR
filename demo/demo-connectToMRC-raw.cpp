#include <stdio.h>
#include <libQuestMR/QuestFrameData.h>
#include <libQuestMR/QuestCommunicator.h>
#include <libQuestMR/QuestStringUtil.h>


using namespace libQuestMR;

void connectToMRC_raw(const char *ipAddr)
{
    BufferedSocket questCom;
    std::vector<char> data;
    std::vector<char> message;
    if(questCom.connect(ipAddr, 25671))
    {
        while(true){
            char buf[255];
            int length = questCom.readData(buf, 255);
            if(length <= 0)
                break;
            addToBuffer(data, buf, length);
            int nextMessageStart = QuestCommunicator::findMessageStart(&data[0], data.size(), 1);
            while(nextMessageStart > 0)
            {
                extractFromStart(message, data, nextMessageStart);
                message.push_back('\0');
                for(int i = 4; i < message.size() && i < 12; i++)
                    printf("%02x ", message[i]);
                printf(", size : %d\n", (int)message.size() - 1 - 12);
                nextMessageStart = QuestCommunicator::findMessageStart(&data[0], data.size(), 1);
            }
        }
    } else {
    	printf("Can not connect to Oculus Mixed Reality Capture app!!\nPlease verify that the app is opened on your quest and the ip address is correct!!\n");
    }
}

int main(int argc, char** argv) 
{
	if(argc < 2) {
		printf("usage: demo-connectToMRC_raw ipAddr\n");
	} else {
		connectToMRC_raw(argv[1]);
    }
    return 0;
}
