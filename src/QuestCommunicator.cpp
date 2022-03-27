#include <libQuestMR/QuestCommunicator.h>
#include <string.h>
#include <sstream>


namespace libQuestMR
{

const char QuestCommunicator::messageStartID[4] = {(char)0x6b, (char)0xa7, (char)0x83, (char)0x52};

QuestCommunicator::QuestCommunicator()
{
}

void QuestCommunicator::onError(std::string errorMsg)
{
    printf("%s\n", errorMsg.c_str());
}

bool QuestCommunicator::connect(std::string address, int port)
{
    return sock.connect(address, port);
}

bool QuestCommunicator::readHeader(char *header, bool loopUntilFound)
{
    if(sock.readNBytes(header, 12) != 12)
        return false;
    if(!loopUntilFound)
        return findMessageStart(header, 12) == 0;
    
    while(findMessageStart(header, 12) < 0)
    {
        for(int i = 0; i < 4; i++)
            header[i] = header[i+8];
        if(sock.readNBytes(header+4, 8) != 8)
            return false;
    }
    int offset = findMessageStart(header, 12);
    if(offset == 0)
        return true;
    for(int i = 0; i < 12-offset; i++)
        header[i] = header[i+offset];
    return (sock.readNBytes(header+offset, 12-offset) == 12-offset);
}

void QuestCommunicator::createHeader(char *buffer, unsigned int type, unsigned int length)
{
    for(int i = 0; i < 4; i++)
        buffer[i] = messageStartID[i];
    fromUInt32(buffer+4, (uint32_t)type);
    fromUInt32(buffer+8, (uint32_t)length);
}

//apparently, the data is not sent in network byte order but in the opposite
//so, to stay platform independent, I first reverse the order (to get it in network byte order)
//and then convert it to UInt32 using ntohl to stay valid for any platform
uint32_t QuestCommunicator::toUInt32(char *data)
{
    DataPacket dataPkt;
    char tmp[4] = {data[3], data[2], data[1], data[0]};
    return DataPacket::convertToUInt32((unsigned char*)tmp);
    //return ntohl(*(uint32_t*)tmp);
}

void QuestCommunicator::fromUInt32(char *data, uint32_t val)
{
    char tmp[4];
    DataPacket::convertUInt32ToBytes(val, (unsigned char*)tmp);
    //uint32_t val_n = htonl(val);
    //char *tmp = (char*)&val_n;
    data[0] = tmp[3];
    data[1] = tmp[2];
    data[2] = tmp[1];
    data[3] = tmp[0];
}

bool QuestCommunicator::readMessage(QuestCommunicatorMessage *output)
{
    char header[12];
    if(!readHeader(header, false))
    {
        onError("Can not read header");
        sock.disconnect();
        return false;
    }
    output->type = toUInt32(header+4);
    uint32_t length = toUInt32(header+8);
    printf("%u bytes to read\n", length);
    std::vector<char> data;
    data.resize(length+1);
    data[length] = 0;
    if(sock.readNBytes(&data[0], length) != length)
    {
        onError("Can not read message content");
        return false;
    }
    output->data = PortableString(&data[0], data.size());
    return true;
}

bool QuestCommunicator::sendMessage(const QuestCommunicatorMessage& msg)
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

int QuestCommunicator::sendRawData(char *data, int length) 
{
    int sizeSent = sock.sendData(data, length);
    if(sizeSent != length)
    {
        onError("send() sent a different number of bytes than expected");
        sock.disconnect();
    }
    return sizeSent;
}

void QuestCommunicator::disconnect()
{
    sock.disconnect();
}

int QuestCommunicator::findMessageStart(char *buffer, int length, int start)
{
    for(int i = start; i+4 < length; i++)
    {
        if(buffer[i] == messageStartID[0] && buffer[i+1] == messageStartID[1]
            && buffer[i+2] == messageStartID[2] && buffer[i+3] == messageStartID[3])
        {
            return i;
        }
    }
    return -1;
}






QuestCommunicatorThreadData::QuestCommunicatorThreadData(QuestCommunicator *questCom)
    :questCom(questCom)
{
    finished = false;
    triggerVal = false;
    needUploadCalibData = false;
    maxQueueSize = 10;
}

void QuestCommunicatorThreadData::setCalibData(const std::string& data)
{
    mutex.lock();
    this->calibData = data;
    mutex.unlock();
}

std::string QuestCommunicatorThreadData::getCalibData()
{
    mutex.lock();
    std::string data = calibData;
    mutex.unlock();
    return data;
}

void QuestCommunicatorThreadData::sendCalibDataToQuest(const std::string& data)
{
    mutex.lock();
    calibData = data;
    needUploadCalibData = true;
    mutex.unlock();
}

bool QuestCommunicatorThreadData::isCalibDataUploaded()
{
	return !needUploadCalibData;
}

bool QuestCommunicatorThreadData::hasNewFrameData()
{
    mutex.lock();
    bool val = !listFrameData.empty();
    mutex.unlock();
    return val;
}

void QuestCommunicatorThreadData::pushFrameData(const QuestFrameData& data)
{
    mutex.lock();
    listFrameData.push(data);
    while(listFrameData.size() >= maxQueueSize)
        listFrameData.pop();
    mutex.unlock();
}

bool QuestCommunicatorThreadData::getFrameData(QuestFrameData *data)
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

void QuestCommunicatorThreadData::setTriggerVal(bool val)
{
    mutex.lock();
    triggerVal = val;
    mutex.unlock();
}

bool QuestCommunicatorThreadData::getTriggerVal()
{
    mutex.lock();
    bool val = triggerVal;
    mutex.unlock();
    return val;
}

void QuestCommunicatorThreadData::setFinishedVal(bool val)
{
    mutex.lock();
    finished = val;
    mutex.unlock();
}

bool QuestCommunicatorThreadData::isFinished()
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


void QuestCommunicatorThreadData::threadFunc()
{
    while(!isFinished())
    {
        QuestCommunicatorMessage message;
        if(needUploadCalibData)
        {
            mutex.lock();
            message.data = PortableString(&calibData[0], calibData.size());
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
        } else if(message.type == 34) {
            //printf("trigger pressed\n");
            setTriggerVal(true);
        } else if(message.type == 36) {
            setCalibData(toString(message.data));
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


}
