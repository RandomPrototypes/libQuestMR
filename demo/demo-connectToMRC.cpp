#include <stdio.h>
#include <libQuestMR/QuestFrameData.h>
#include <libQuestMR/QuestCommunicator.h>

using namespace libQuestMR;

void connectToMRC(const char *ipAddr)
{
    std::shared_ptr<QuestCommunicator> questCom = createQuestCommunicator();
    if(!questCom->connect(ipAddr)) {
    	printf("Can not connect to Oculus Mixed Reality Capture app!!\nPlease verify that the app is opened on your quest and the ip address is correct!!\n");
    	return ;
    }
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
        printf("message type: %s (%u) size:%d\n", message.getTypeStr(), message.type, size);
        if(message.type == QuestMessageTypeId::poseUpdate)
        {
            QuestFrameData frame;
            frame.parse(message.data.c_str(), size);
            printf("frame data:\n%s\n", frame.toString().c_str());
        } else if(size == 4) {
        	printf("data: %d\n", questCom->toUInt32(message.data.c_str()));
        } else if(size > 0) {
        	printf("raw data:\n%.*s\n\n", size, message.data.c_str());
        }
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
