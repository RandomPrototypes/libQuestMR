#include <unistd.h>
#include <string.h>
#include "BufferedSocket.h"

#if defined(USE_WINDOWS)
WSADATA BufferedSocket::wsaData;
#endif

BufferedSocket::BufferedSocket()
{
    buffer = NULL;
    setBufferSize(8*1024);
}

BufferedSocket::~BufferedSocket()
{
    delete [] buffer;
}

void BufferedSocket::setBufferSize(int size)
{
    if(buffer != NULL)
    {
        if(bufferSize == size || size < bufferFilledSize)
            return;
        char *buffer2 = new char [size];
        memcpy(buffer2, buffer, bufferFilledSize);
        delete [] buffer;
        buffer = buffer2;
        return ;
    }
    buffer = new char[size];
    bufferSize = size;
    bufferFilledSize = 0;
    bufferStartPos = 0;
}

void BufferedSocket::startup()
{
#if defined(USE_WINDOWS)
    WSAStartup(MAKEWORD(2,2),&wsaData);
#endif
}

void BufferedSocket::cleanup()
{
#if defined(USE_WINDOWS)
    WSACleanup();
#endif
}

void BufferedSocket::onError(std::string errorMsg)
{
    printf("%s\n", errorMsg.c_str());
}

void BufferedSocket::closeSockAndThrowError(std::string errorMsg)
{
    close(sock);
    sock = -1;
    onError(errorMsg);
}


bool BufferedSocket::connect(std::string address, int port)
{
    #if defined(USE_WINDOWS)
    sock=socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);
    sockaddr_in sockAddr;
    sockAddr.sin_family=AF_INET;
    sockAddr.sin_addr.S_un.S_addr=inet_addr(address.c_str());
    sockAddr.sin_port=htons(port);
    if(::connect(sock,(SOCKADDR*)&sockAddr,sizeof(SOCKADDR)) < 0)
    {
        closeSockAndThrowError("Failed to connect.");
        return false;
    }
    #elif defined(USE_LINUX)
    sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0) {
        onError("socket creation failed.");
        return false;
    }

    struct sockaddr_in sad;
    memset(&sad, 0, sizeof(sad));
    sad.sin_family = AF_INET;
    sad.sin_addr.s_addr = inet_addr(address.c_str());
    sad.sin_port = htons(port);
    
    if (::connect(sock, (struct sockaddr *) &sad, sizeof(sad)) < 0)
    {
        closeSockAndThrowError("Failed to connect.");
        return false;
    }
    #endif
    
    return true;
}

void BufferedSocket::disconnect()
{
#if defined(USE_WINDOWS)
    closesocket(sock);
#elif defined(USE_LINUX)
    close(sock);
#endif
    sock = -1;
}

int BufferedSocket::readData(char *outputBuf, int outputBufSize)
{
    if(bufferFilledSize == 0)
    {
        if(outputBufSize > bufferSize)
        {
            int sizeRead = recv(sock, outputBuf, outputBufSize, 0);
            if(sizeRead <= 0)
                closeSockAndThrowError("recv() failed or connection closed prematurely");
            return sizeRead;
        }
        else 
        {
            int sizeRead = recv(sock, buffer, bufferSize, 0);
            if(sizeRead <= 0)
            {
                closeSockAndThrowError("recv() failed or connection closed prematurely");
                return sizeRead;
            }
            bufferStartPos = 0;
            bufferFilledSize = sizeRead;
        }
    }

    int sizeRead = std::min(bufferFilledSize, outputBufSize);
    memcpy(outputBuf, buffer+bufferStartPos, sizeRead);
    bufferFilledSize -= sizeRead;
    bufferStartPos += sizeRead;
    if(bufferFilledSize == 0)
        bufferStartPos = 0;
    return sizeRead;
}

int BufferedSocket::sendData(const char *data, int length)
{
    int sizeSent = send(sock, data, length, 0);
    if(sizeSent != length)
        closeSockAndThrowError("send() sent a different number of bytes than expected");
    return sizeSent;
}

void BufferedSocket::removeAlreadyReadData()
{
    if(bufferStartPos > 0)
    {
        memmove(buffer, buffer+bufferStartPos, bufferFilledSize);
        bufferStartPos = 0;
    }
}

std::vector<char> BufferedSocket::readUntilStr(const char *str, int length)
{
    std::vector<char> result;
    while(true)
    {
        if(bufferFilledSize == 0)
        {
            bufferStartPos = 0;
            int sizeRead = recv(sock, buffer, bufferSize, 0);
            if(sizeRead <= 0) {
                closeSockAndThrowError("recv() failed or connection closed prematurely");
                return std::vector<char>();
            }
            bufferFilledSize += sizeRead;
        }
        if(buffer[bufferStartPos] != str[0])
        {
            result.push_back(buffer[bufferStartPos]);
            bufferStartPos++;
            bufferFilledSize--;
        } else {
            if(bufferFilledSize < length)
            {
                if(bufferSize - bufferStartPos < length)
                    removeAlreadyReadData();
                while(bufferFilledSize < length)
                {
                    int sizeRead = recv(sock, buffer+bufferStartPos, bufferSize-bufferStartPos, 0);
                    if(sizeRead <= 0) {
                        closeSockAndThrowError("recv() failed or connection closed prematurely");
                        return std::vector<char>();
                    }    
                    bufferFilledSize += sizeRead;
                }
            }

            bool found = true;
            for(int i = 1; i < length; i++)
                if(buffer[bufferStartPos + i] != str[i])
                {
                    found = false;
                    break;
                }
            if(found)
            {
                for(int i = 0; i < length; i++)
                    result.push_back(str[i]);
                bufferStartPos += length;
                bufferFilledSize -= length;
                return result;
            } else {
                result.push_back(buffer[bufferStartPos]);
                bufferStartPos++;
                bufferFilledSize--;
            }
        }        
    }

    return std::vector<char>();
}

int BufferedSocket::readNBytes(char *outputBuf, int N)
{
    int total = 0;
    while(total < N)
    {
        int sizeRead = readData(outputBuf + total, N - total);
        if(sizeRead <= 0)
            return sizeRead;
        total += sizeRead;
    }
    return total;
}

int BufferedSocket::sendNBytes(const char *buffer, int N)
{
    int total = 0;
    while(total < N)
    {
        int len = N - total;
        if(len > 10240)
            len = 10240;
        int sizeSent = send(sock, buffer + total, len, 0);
        if(sizeSent <= 0){
            closeSockAndThrowError("recv() failed or connection closed prematurely");
            return sizeSent;
        }
        total += len;
    }
    return total;
}



int32_t BufferedSocket::readInt32()
{
    uint32_t val = readUInt32();
    return *(int32_t*)&val;
}

u_int32_t BufferedSocket::readUInt32()
{
    unsigned char s[4];
    readNBytes((char*)s, 4);
    return ((uint32_t) s[3] << 0)
         | ((uint32_t) s[2] << 8)
         | ((uint32_t) s[1] << 16)
         | ((uint32_t) s[0] << 24);
}

int64_t BufferedSocket::readInt64()
{
    uint64_t val = readUInt64();
    return *(int64_t*)&val;
}

u_int64_t BufferedSocket::readUInt64()
{
    unsigned char s[8];
    readNBytes((char*)s, 8);
    return ((uint64_t) s[7] << 0)
         | ((uint64_t) s[6] << 8)
         | ((uint64_t) s[5] << 16)
         | ((uint64_t) s[4] << 24)
         | ((uint64_t) s[3] << 32)
         | ((uint64_t) s[2] << 40)
         | ((uint64_t) s[1] << 48)
         | ((uint64_t) s[0] << 56);
}

bool BufferedSocket::sendInt32(int32_t val)
{
    return sendUInt32(*(u_int32_t*)&val);
}

bool BufferedSocket::sendUInt32(u_int32_t val)
{
    unsigned char s[4];
    s[0] = (val >> 24) & 0xFF;
    s[1] = (val >> 16) & 0xFF;
    s[2] = (val >> 8) & 0xFF;
    s[3] = val & 0xFF;
    return sendNBytes((char*)s, 4) == 4;
}
bool BufferedSocket::sendInt64(int64_t val)
{
    return sendUInt64(*(u_int64_t*)&val);
}
bool BufferedSocket::sendUInt64(u_int64_t val)
{
    unsigned char s[8];
    s[0] = (val >> 56) & 0xFF;
    s[1] = (val >> 48) & 0xFF;
    s[2] = (val >> 40) & 0xFF;
    s[3] = (val >> 32) & 0xFF;
    s[4] = (val >> 24) & 0xFF;
    s[5] = (val >> 16) & 0xFF;
    s[6] = (val >> 8) & 0xFF;
    s[7] = val & 0xFF;
    return sendNBytes((char*)s, 8) == 8;
}

DataPacket::DataPacket()
{
    offset = 0;
}

void DataPacket::rewind()
{
    offset = 0;
}

bool DataPacket::readInt32(int32_t *out)
{
    return readUInt32((u_int32_t*)out);
}
bool DataPacket::readUInt32(u_int32_t *out)
{
    if(offset + 4 > data.size())
        return false;

    *out = ((uint32_t) data[offset+3] << 0)
         | ((uint32_t) data[offset+2] << 8)
         | ((uint32_t) data[offset+1] << 16)
         | ((uint32_t) data[offset] << 24);
    offset += 4;
    return true;
}
bool DataPacket::readInt64(int64_t *out)
{
    return readUInt64((u_int64_t*)out);
}
bool DataPacket::readUInt64(u_int64_t *out)
{
    if(offset + 8 > data.size())
        return false;

    *out = ((uint64_t) data[offset+7] << 0)
         | ((uint64_t) data[offset+6] << 8)
         | ((uint64_t) data[offset+5] << 16)
         | ((uint64_t) data[offset+4] << 24)
         | ((uint64_t) data[offset+3] << 32)
         | ((uint64_t) data[offset+2] << 40)
         | ((uint64_t) data[offset+1] << 48)
         | ((uint64_t) data[offset] << 56);
    offset += 8;
    return true;
}

void DataPacket::putInt32(int32_t val)
{
    putUInt32(*(u_int32_t*)&val);
}
void DataPacket::putUInt32(u_int32_t val)
{
    data.push_back((val >> 24) & 0xFF);
    data.push_back((val >> 16) & 0xFF);
    data.push_back((val >> 8) & 0xFF);
    data.push_back(val & 0xFF);
}
void DataPacket::putInt64(int64_t val)
{
    putUInt64(*(u_int64_t*)&val);
}
void DataPacket::putUInt64(u_int64_t val)
{
    data.push_back((val >> 56) & 0xFF);
    data.push_back((val >> 48) & 0xFF);
    data.push_back((val >> 40) & 0xFF);
    data.push_back((val >> 32) & 0xFF);
    data.push_back((val >> 24) & 0xFF);
    data.push_back((val >> 16) & 0xFF);
    data.push_back((val >> 8) & 0xFF);
    data.push_back(val & 0xFF);
}

void DataPacket::putNBytes(const char* buf, int N)
{
    int start = data.size();
    data.resize(start+N);
    for(int i = 0; i < N; i++)
        data[start+i] = buf[i];
}

int DataPacket::size()
{
    return data.size();
}

const char *DataPacket::getRawPtr()
{
    return &data[0];
}