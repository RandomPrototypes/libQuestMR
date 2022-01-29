#pragma once

#include <vector>
#include <string>
#include <queue>
#include <mutex>
#include "config.h"
#include "QuestFrameData.h"

namespace libQuestMR
{

class QuestCommunicatorMessage
{
public:
    unsigned int type;
    std::vector<char> data;
};

class QuestCommunicator
{
public:
    int sock;
    const char messageStartID[4] = {(char)0x6b, (char)0xa7, (char)0x83, (char)0x52};


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

    //Read N bytes from the socket.
    //Block until N bytes have been read unless there is communication error.
    int readNBytes(char *buffer, int N);

    //Read data from the socket.
    //Will close the socket in case of communcation error and return negative number
    //Returns the number of byte read in case of success
    int readRawData(char *buffer, int bufferSize);

    //Send data to the socket
    //Will close the socket if the number of bytes sent is incorrect
    //Returns the number of bytes sent
    int sendRawData(char *data, int length);

    //Disconnect the socket
    void disconnect();

    //search for the message start "magic value": 0x6ba78352
    //mainly for debug purpose, you do not need it if the protocol is implemented correctly
    int findMessageStart(char *buffer, int length, int start = 0);
};

class QuestCommunicatorThreadData
{
public:
    QuestCommunicatorThreadData(QuestCommunicator *questCom);

    //Thread-safe function to set if the communication is finished
    void setFinishedVal(bool val);

    //Thread-safe function to know if the communication is finished
    bool isFinished();

    //Thread-safe function to set the calibration data
    void setCalibData(const std::string& data);

    void sendCalibDataToQuest(const std::string& data);

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

void QuestCommunicatorThreadFunc(QuestCommunicatorThreadData *data);


}