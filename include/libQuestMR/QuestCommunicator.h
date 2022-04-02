#pragma once

#include <libQuestMR/config.h>
#include <libQuestMR/QuestFrameData.h>
#include <libQuestMR/PortableTypes.h>
#include <BufferedSocket/BufferedSocket.h>
#include <vector>
#include <string>
#include <queue>
#include <mutex>

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
    virtual ~QuestCommunicator();

    virtual void onError(const char *errorMsg) = 0;

    //connect to the quest calibration app, port should be 25671
    virtual bool connect(const char *address, int port = QUEST_CALIB_DEFAULT_PORT) = 0;

    virtual bool readHeader(char *header, bool loopUntilFound = true) = 0;

    virtual void createHeader(char *buffer, unsigned int type, unsigned int length) = 0;

    //apparently, the data is not sent in network byte order but in the opposite
    //so, to stay platform independent, I first reverse the order (to get it in network byte order)
    //and then convert it to UInt32 using ntohl to stay valid for any platform
    virtual uint32_t toUInt32(char *data) = 0;

    virtual void fromUInt32(char *data, uint32_t val) = 0;

    //Read the socket until one full message is obtained (blocking call).
    //Return false in case of communication error
    virtual bool readMessage(QuestCommunicatorMessage *output) = 0;

    //Send the message to the socket
    //Return false in case of communication error
    virtual bool sendMessage(const QuestCommunicatorMessage& msg) = 0;

    //Send data to the socket
    //Will close the socket if the number of bytes sent is incorrect
    //Returns the number of bytes sent
    virtual int sendRawData(const char *data, int length) = 0;

    //Disconnect the socket
    virtual void disconnect() = 0;
};

class LQMR_EXPORTS QuestCommunicatorThreadData
{
public:
    virtual ~QuestCommunicatorThreadData();

    //Thread-safe function to set if the communication is finished
    virtual void setFinishedVal(bool val) = 0;

    //Thread-safe function to know if the communication is finished
    virtual bool isFinished() = 0;

    //Thread-safe function to set the calibration data
    virtual void setCalibData(const PortableString& data) = 0;

	//request upload calib data to the quest.
	//Non-blocking, you should wait until isCalibDataUploaded() is true for the upload to be done.
    virtual void sendCalibDataToQuest(const PortableString& data) = 0;
    
    //true when the calib data has been uploaded to the quest
    virtual bool isCalibDataUploaded() = 0;

    //Thread-safe function to get the latest calibration data
    virtual PortableString getCalibData() = 0;

    //Thread-safe function to check if there is a frame data available on the queue
    virtual bool hasNewFrameData() = 0;

    //Thread-safe function to push a FrameData to the queue
    virtual void pushFrameData(const QuestFrameData& data) = 0;

    //Thread-safe function to get a FrameData from the queue, returns false if empty
    virtual bool getFrameData(QuestFrameData *data) = 0;

    //Thread-safe function to set the value of the trigger
    virtual void setTriggerVal(bool val) = 0;

    //Thread-safe function to get the value of the trigger
    virtual bool getTriggerVal() = 0;

    virtual void threadFunc() = 0;
};

LQMR_EXPORTS void QuestCommunicatorThreadFunc(QuestCommunicatorThreadData *data);

extern "C"
{
	//search for the message start "magic value": 0x6ba78352
	//mainly for debug purpose, you do not need it if the protocol is implemented correctly
	LQMR_EXPORTS int findMessageStart(const char *buffer, int length, int start = 0);
	
	LQMR_EXPORTS QuestCommunicator *createQuestCommunicatorRawPtr();
	LQMR_EXPORTS void deleteQuestCommunicatorRawPtr(QuestCommunicator *com);
	LQMR_EXPORTS QuestCommunicatorThreadData *createQuestCommunicatorThreadDataRawPtr(std::shared_ptr<QuestCommunicator> com);
	LQMR_EXPORTS void deleteQuestCommunicatorThreadDataRawPtr(QuestCommunicatorThreadData *comThreadData);
}

inline std::shared_ptr<QuestCommunicator> createQuestCommunicator()
{
	return std::shared_ptr<QuestCommunicator>(createQuestCommunicatorRawPtr(), deleteQuestCommunicatorRawPtr);
}

inline std::shared_ptr<QuestCommunicatorThreadData> createQuestCommunicatorThreadData(std::shared_ptr<QuestCommunicator> com)
{
	return std::shared_ptr<QuestCommunicatorThreadData>(createQuestCommunicatorThreadDataRawPtr(com), deleteQuestCommunicatorThreadDataRawPtr);
}

}
