#include <libQuestMR/QuestCommunicator.h>
#include <BufferedSocket/DataPacket.h>
#include <string.h>
#include <sstream>


namespace libQuestMR
{

class QuestCommunicatorImpl : public QuestCommunicator
{
public:
    QuestCommunicatorImpl();
    virtual ~QuestCommunicatorImpl();

    virtual void onError(const char *errorMsg);

    //connect to the quest calibration app, port should be 25671
    virtual bool connect(const char *address, int port = QUEST_CALIB_DEFAULT_PORT);

    virtual bool readHeader(char *header, bool loopUntilFound = true);

    virtual void createHeader(char *buffer, unsigned int type, unsigned int length);

    //apparently, the data is not sent in network byte order but in the opposite
    //so, to stay platform independent, I first reverse the order (to get it in network byte order)
    //and then convert it to UInt32 using ntohl to stay valid for any platform
    virtual uint32_t toUInt32(const char *data) const;

    virtual void fromUInt32(char *data, uint32_t val) const;

    //Read the socket until one full message is obtained (blocking call).
    //Return false in case of communication error
    virtual bool readMessage(QuestCommunicatorMessage *output);

    //Send the message to the socket
    //Return false in case of communication error
    virtual bool sendMessage(const QuestCommunicatorMessage& msg);

    //Send data to the socket
    //Will close the socket if the number of bytes sent is incorrect
    //Returns the number of bytes sent
    virtual int sendRawData(const char *data, int length);

    //Disconnect the socket
    virtual void disconnect();
    
    static const char messageStartID[4];
private:
	std::shared_ptr<BufferedSocket> sock;
};

const char QuestCommunicatorImpl::messageStartID[4] = {(char)0x6b, (char)0xa7, (char)0x83, (char)0x52};

QuestCommunicator::~QuestCommunicator()
{
}

QuestCommunicatorImpl::QuestCommunicatorImpl()
{
	sock = createBufferedSocket();
}

QuestCommunicatorImpl::~QuestCommunicatorImpl()
{
}

void QuestCommunicatorImpl::onError(const char *errorMsg)
{
    printf("%s\n", errorMsg);
}

bool QuestCommunicatorImpl::connect(const char *address, int port)
{
    return sock->connect(address, port);
}

bool QuestCommunicatorImpl::readHeader(char *header, bool loopUntilFound)
{
    if(sock->readNBytes(header, 12) != 12)
        return false;
    if(!loopUntilFound)
        return findMessageStart(header, 12) == 0;
    
    while(findMessageStart(header, 12) < 0)
    {
        for(int i = 0; i < 4; i++)
            header[i] = header[i+8];
        if(sock->readNBytes(header+4, 8) != 8)
            return false;
    }
    int offset = findMessageStart(header, 12);
    if(offset == 0)
        return true;
    for(int i = 0; i < 12-offset; i++)
        header[i] = header[i+offset];
    return (sock->readNBytes(header+offset, 12-offset) == 12-offset);
}

void QuestCommunicatorImpl::createHeader(char *buffer, unsigned int type, unsigned int length)
{
    for(int i = 0; i < 4; i++)
        buffer[i] = messageStartID[i];
    fromUInt32(buffer+4, (uint32_t)type);
    fromUInt32(buffer+8, (uint32_t)length);
}

uint32_t QuestCommunicatorImpl::toUInt32(const char *data) const
{
    //little endian uchar* to uint32 conversion
    return convertBytesToUInt32((unsigned char*)data, false);
}

void QuestCommunicatorImpl::fromUInt32(char *data, uint32_t val) const
{
    //little endian uint32 to uchar* conversion
    convertUInt32ToBytes(val, (unsigned char*)data, false);
}

bool QuestCommunicatorImpl::readMessage(QuestCommunicatorMessage *output)
{
    char header[12];
    if(!readHeader(header, false))
    {
        onError("Can not read header");
        sock->disconnect();
        return false;
    }
    output->type = toUInt32(header+4);
    uint32_t length = toUInt32(header+8);
    std::vector<char> data;
    data.resize(length+1);
    data[length] = 0;
    if(sock->readNBytes(&data[0], length) != length)
    {
        onError("Can not read message content");
        return false;
    }
    output->data = PortableString(&data[0], data.size());
    return true;
}

bool QuestCommunicatorImpl::sendMessage(const QuestCommunicatorMessage& msg)
{
    int totalSize = 12+(int)msg.data.size();
    char *data = new char[totalSize];
    createHeader(data, msg.type, (int)msg.data.size());
    const char *msgData = msg.data.c_str();
    for(size_t i = 0; i < msg.data.size(); i++)
        data[i+12] = msgData[i];
    int ret = sendRawData(data, totalSize);
    delete [] data;
    return ret == totalSize;
}

int QuestCommunicatorImpl::sendRawData(const char *data, int length) 
{
    int sizeSent = sock->sendData(data, length);
    if(sizeSent != length)
    {
        onError("send() sent a different number of bytes than expected");
        sock->disconnect();
    }
    return sizeSent;
}

void QuestCommunicatorImpl::disconnect()
{
    sock->disconnect();
}









class QuestCommunicatorThreadDataImpl : public QuestCommunicatorThreadData
{
public:
    QuestCommunicatorThreadDataImpl(std::shared_ptr<QuestCommunicator> questCom);
    ~QuestCommunicatorThreadDataImpl();

    //Thread-safe function to set if the communication is finished
    virtual void setFinishedVal(bool val);

    //Thread-safe function to know if the communication is finished
    virtual bool isFinished();

    //Thread-safe function to set the calibration data
    virtual void setCalibData(const PortableString& data);

	//request upload calib data to the quest.
	//Non-blocking, you should wait until isCalibDataUploaded() is true for the upload to be done.
    virtual void sendCalibDataToQuest(const PortableString& data);
    
    //true when the calib data has been uploaded to the quest
    virtual bool isCalibDataUploaded();

    //Thread-safe function to get the latest calibration data
    virtual PortableString getCalibData();

    //Thread-safe function to check if there is a frame data available on the queue
    virtual bool hasNewFrameData();

    //Thread-safe function to push a FrameData to the queue
    virtual void pushFrameData(const QuestFrameData& data);

    //Thread-safe function to get a FrameData from the queue, returns false if empty
    virtual bool getFrameData(QuestFrameData *data);

    //Thread-safe function to set the value of the trigger
    virtual void setTriggerCount(uint32_t val);

    //Thread-safe function to get the value of the trigger
    virtual uint32_t getTriggerCount();

    virtual void threadFunc();
private:
    std::mutex mutex;
    std::shared_ptr<QuestCommunicator> questCom;
    std::queue<QuestFrameData> listFrameData;
    PortableString calibData;
    bool needUploadCalibData;
    bool finished;
    uint32_t triggerCount;
    int maxQueueSize;
};

QuestCommunicatorThreadData::~QuestCommunicatorThreadData()
{
}

QuestCommunicatorThreadDataImpl::QuestCommunicatorThreadDataImpl(std::shared_ptr<QuestCommunicator> questCom)
    :questCom(questCom)
{
    finished = false;
    triggerCount = 0;
    needUploadCalibData = false;
    maxQueueSize = 10;
}

QuestCommunicatorThreadDataImpl::~QuestCommunicatorThreadDataImpl()
{
}

void QuestCommunicatorThreadDataImpl::setCalibData(const PortableString& data)
{
    mutex.lock();
    this->calibData = data;
    mutex.unlock();
}

PortableString QuestCommunicatorThreadDataImpl::getCalibData()
{
    mutex.lock();
    PortableString data = calibData;
    mutex.unlock();
    return data;
}

void QuestCommunicatorThreadDataImpl::sendCalibDataToQuest(const PortableString& data)
{
    mutex.lock();
    calibData = data;
    needUploadCalibData = true;
    mutex.unlock();
}

bool QuestCommunicatorThreadDataImpl::isCalibDataUploaded()
{
	return !needUploadCalibData;
}

bool QuestCommunicatorThreadDataImpl::hasNewFrameData()
{
    mutex.lock();
    bool val = !listFrameData.empty();
    mutex.unlock();
    return val;
}

void QuestCommunicatorThreadDataImpl::pushFrameData(const QuestFrameData& data)
{
    mutex.lock();
    listFrameData.push(data);
    while(listFrameData.size() >= maxQueueSize)
        listFrameData.pop();
    mutex.unlock();
}

bool QuestCommunicatorThreadDataImpl::getFrameData(QuestFrameData *data)
{
    bool val = true;
    mutex.lock();
    if(listFrameData.empty()) {
        val = false;
    } else {
        *data = listFrameData.front();
        listFrameData.pop();
    }
    mutex.unlock();
    return val;
}

void QuestCommunicatorThreadDataImpl::setTriggerCount(uint32_t val)
{
    mutex.lock();
    triggerCount = val;
    mutex.unlock();
}

uint32_t QuestCommunicatorThreadDataImpl::getTriggerCount()
{
    mutex.lock();
    uint32_t val = triggerCount;
    mutex.unlock();
    return val;
}

void QuestCommunicatorThreadDataImpl::setFinishedVal(bool val)
{
    mutex.lock();
    finished = val;
    mutex.unlock();
}

bool QuestCommunicatorThreadDataImpl::isFinished()
{
    mutex.lock();
    bool val = finished;
    mutex.unlock();
    return val;
}



std::string toString(const std::vector<char>& listChars)
{
    std::stringstream ss;
    for(int i = 0; i < listChars.size(); i++)
        if(listChars[i] != 0)
            ss << listChars[i];
    return ss.str();
}


void QuestCommunicatorThreadDataImpl::threadFunc()
{
    while(!isFinished())
    {
        QuestCommunicatorMessage message;
        if(needUploadCalibData)
        {
            mutex.lock();
            message.data = calibData;
            message.type = 36;
            bool ret = questCom->sendMessage(message);
            needUploadCalibData = false;
            mutex.unlock();
            if(!ret)
                break;
            else continue;
        }
        if(!questCom->readMessage(&message))
            break;
        if(message.type == 33)
        {
            QuestFrameData frame;
            frame.parse(message.data.c_str(), (int)message.data.size()-1);
            //printf("frame data:\n%s\n", frame.toString().c_str());

            pushFrameData(frame);
        } else if(message.type == 34 && message.data.size() >= 4) {
            //printf("trigger pressed\n");
            setTriggerCount(questCom->toUInt32(message.data.c_str()));
        } else if(message.type == 36) {
            setCalibData(message.data.c_str());
        } else {
            /*printf("message type:%u size:%u\n", message.type, message.data.size()-1);
            for(size_t i = 0; i < message.data.size(); i++)
                printf("%c", message.data[i]);
            printf("\n");*/
        }
    }
}


void QuestCommunicatorThreadFunc(QuestCommunicatorThreadData *data)
{
    data->threadFunc();
}




extern "C" 
{
	int findMessageStart(const char *buffer, int length, int start)
	{
		for(int i = start; i+4 < length; i++)
		{
		    if(buffer[i] == QuestCommunicatorImpl::messageStartID[0] && buffer[i+1] == QuestCommunicatorImpl::messageStartID[1]
		        && buffer[i+2] == QuestCommunicatorImpl::messageStartID[2] && buffer[i+3] == QuestCommunicatorImpl::messageStartID[3])
		    {
		        return i;
		    }
		}
		return -1;
	}

	QuestCommunicator *createQuestCommunicatorRawPtr()
	{
		return new QuestCommunicatorImpl();
	}
	
	void deleteQuestCommunicatorRawPtr(QuestCommunicator *com)
	{
		delete com;
	}
	
	QuestCommunicatorThreadData *createQuestCommunicatorThreadDataRawPtr(std::shared_ptr<QuestCommunicator> com)
	{
		return new QuestCommunicatorThreadDataImpl(com);
	}
	
	void deleteQuestCommunicatorThreadDataRawPtr(QuestCommunicatorThreadData *comThreadData)
	{
		delete comThreadData;
	}

}


}
