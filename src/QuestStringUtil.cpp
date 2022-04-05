#include <libQuestMR/QuestStringUtil.h>

namespace libQuestMR
{

void addToBuffer(std::vector<char>& buffer, const char *data, int N)
{
    int prev_size = (int)buffer.size();
    buffer.resize(prev_size + N);
    for(int i = 0; i < N; i++)
        buffer[prev_size+i] = data[i];
}

bool extractFromStart(std::vector<char>& output, std::vector<char>& buffer, int N)
{
    if(N > buffer.size())
        return false;
    output.resize(N);
    for(int i = 0; i < N; i++)
        output[i] = buffer[i];
    buffer.erase(buffer.begin(), buffer.begin()+N);
    return true;
}

bool isEmptyChar(char val)
{
    return val == ' ' || val == '\t' || val == '\r' || val == '\n';
}

bool isStringChar(char val)
{
    return (val >= 'a' && val <= 'z')
        || (val >= 'A' && val <= 'Z')
        || val == '_';
}

bool isDigitChar(char val)
{
    return (val >= '0' && val <= '9');
}

void skipEmpty(const char *str, int length, int *currentPos)
{
    while(*currentPos < length && isEmptyChar(str[*currentPos]))
        (*currentPos)++;
}

void skipNonEmpty(const char *str, int length, int *currentPos)
{
    while(*currentPos < length && !isEmptyChar(str[*currentPos]))
        (*currentPos)++;
}

void skipNonString(const char *str, int length, int *currentPos)
{
    while(*currentPos < length && !isStringChar(str[*currentPos]))
        (*currentPos)++;
}

std::string parseString(const char *str, int length, int *currentPos)
{
    std::string result = "";
    while(*currentPos < length && isStringChar(str[*currentPos]))
    {
        result += str[*currentPos];
        (*currentPos)++;
    }
    return result;
}

std::string parseIntString(const char *str, int length, int *currentPos, bool with_minus)
{
    std::string result = "";
    if(with_minus && *currentPos < length && str[*currentPos] == '-')
    {
        result += '-';
        (*currentPos)++;
    }
    while(*currentPos < length && isDigitChar(str[*currentPos]))
    {
        result += str[*currentPos];
        (*currentPos)++;
    }
    return result;
}

int parseInt(const char *str, int length, int *currentPos)
{
    return std::atoi(parseIntString(str, length, currentPos, true).c_str());
}

std::string parseDoubleString(const char *str, int length, int *currentPos)
{
    std::string result = parseIntString(str, length, currentPos, true);
    if(*currentPos >= length || (str[*currentPos] != '.' && str[*currentPos] != 'E' && str[*currentPos] != 'e'))
        return result;
    if(str[*currentPos] == '.') {
        result += '.';
        (*currentPos)++;
        result += parseIntString(str, length, currentPos, false);
        if(*currentPos >= length || (str[*currentPos] != 'E' && str[*currentPos] != 'e'))
            return result;
    }
    result += 'E';
    (*currentPos)++;
    result += parseIntString(str, length, currentPos, true);
    return result;
}

double parseDouble(const char *str, int length, int *currentPos)
{
    return std::atof(parseDoubleString(str, length, currentPos).c_str());
}

void parseVectorDouble(const char *str, int length, int *currentPos, int nbElem, double *result)
{
    for(int i = 0; i < nbElem; i++)
    {
        result[i] = parseDouble(str, length, currentPos);
        if(i+1 < nbElem)
        {
            skipEmpty(str, length, currentPos);
            if(*currentPos >= length || str[*currentPos] != ',')
            {
                printf("error parsing vector double\n");
                return ;
            }
            (*currentPos)++;
            skipEmpty(str, length, currentPos);
        }
    }
}






}
