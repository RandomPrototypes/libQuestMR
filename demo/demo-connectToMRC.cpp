#include <stdio.h>
#include <libQuestMR/QuestFrameData.h>
#include <libQuestMR/QuestCommunicator.h>

using namespace libQuestMR;

void connectToMRC(const char *ipAddr)
{
    QuestCommunicator questCom;
    if(questCom.connect(ipAddr, 25671))
    {
        while(true)
        {
            QuestCommunicatorMessage message;
            if(!questCom.readMessage(&message))
                break;
            printf("message type:%u size:%d\n", message.type, (int)message.data.size()-1);
            if(message.type == 33)
            {
                QuestFrameData frame;
                frame.parse(&(message.data[0]), message.data.size()-1);
                printf("frame data:\n%s\n", frame.toString().c_str());
            }
            else printf("raw data:\n%.*s\n\n", (int)message.data.size()-1, &(message.data[0]));
        }
    } else {
    	printf("Can not connect to Oculus Mixed Reality Capture app!!\nPlease verify that the app is opened on your quest and the ip address is correct!!\n");
    }
}

int main(int argc, char** argv) 
{
	if(argc < 2) {
		printf("usage: demo-connectToMRC ipAddr\n");
	} else {
		connectToMRC(argv[1]);
    }
    return 0;
}