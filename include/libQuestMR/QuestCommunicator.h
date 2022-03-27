#pragma once

#include <vector>
#include <string>
#include <queue>
#include <mutex>
#include "config.h"
#include "QuestFrameData.h"
#include "BufferedSocket.h"
#include "PortableTypes.h"

namespace libQuestMR
{

class LQMR_EXPORTS QuestCommunicatorMessage
{
public:
    unsigned int type;
    PortableString data;
};

class LQMR_EXPORTS QuestCommunicator
{
public:
    BufferedSocket sock;
    static const char messageStartID[4];


    QuestCommunicator();

    void onError(std::string errorMsg);

    //connect to the quest calibration app, port should be 25671
    bool connect(std::string address, int port = QUEST_CALIB_DEFAULT_PORT);

    bool readHeader(char *header, bool loopUntilFound = true);

    void createHeader(char *buffer, unsigned int type, unsigned int length);

    //apparently, the data is not sent in network byte order but in the opposite
    //so, to stay platform independent, I first reverse the order (to get it in network byte order)
    //and then convert it to UInt32 using ntohl to stay valid for any platform
    uint32_t toUInt32(char *data);

    void fromUInt32(char *data, uint32_t val);

    //Read the socket until one full message is obtained (blocking call).
    //Return false in case of communication error
    bool readMessage(QuestCommunicatorMessage *output);

    //Send the message to the socket
    //Return false in case of communication error
    bool sendMessage(const QuestCommunicatorMessage& msg);

    //Send data to the socket
    //Will close the socket if the number of bytes sent is incorrect
    //Returns the number of bytes sent
    int sendRawData(char *data, int length);

    //Disconnect the socket
    void disconnect();

    //search for the message start "magic value": 0x6ba78352
    //mainly for debug purpose, you do not need it if the protocol is implemented correctly
    static int findMessageStart(char *buffer, int length, int start = 0);
};

class LQMR_EXPORTS QuestCommunicatorThreadData
{
public:
    QuestCommunicatorThreadData(QuestCommunicator *questCom);

    //Thread-safe function to set if the communication is finished
    void setFinishedVal(bool val);

    //Thread-safe function to know if the communication is finished
    bool isFinished();

    //Thread-safe function to set the calibration data
    void setCalibData(const std::string& data);

	//request upload calib data to the quest.
	//Non-blocking, you should wait until isCalibDataUploaded() is true for the upload to be done.
    void sendCalibDataToQuest(const std::string& data);
    
    //true when the calib data has been uploaded to the quest
    bool isCalibDataUploaded();

    //Thread-safe function to get the latest calibration data
    std::string getCalibData();

    //Thread-safe function to check if there is a frame data available on the queue
    bool hasNewFrameData();

    //Thread-safe function to push a FrameData to the queue
    void pushFrameData(const QuestFrameData& data);

    //Thread-safe function to get a FrameData from the queue, returns false if empty
    bool getFrameData(QuestFrameData *data);

    //Thread-safe function to set the value of the trigger
    void setTriggerVal(bool val);

    //Thread-safe function to get the value of the trigger
    bool getTriggerVal();

    void threadFunc();
private:
    std::mutex mutex;
    QuestCommunicator *questCom;
    std::queue<QuestFrameData> listFrameData;
    std::string calibData;
    bool needUploadCalibData;
    bool finished;
    bool triggerVal;
    int maxQueueSize;
};

LQMR_EXPORTS void QuestCommunicatorThreadFunc(QuestCommunicatorThreadData *data);


}
