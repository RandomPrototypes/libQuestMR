#pragma once

#include "config.h"

#include <vector>
#include <string>

#include "config.h"

namespace libQuestMR
{

//copy N bytes from data to the end of buffer.
RP_EXPORTS void addToBuffer(std::vector<char>& buffer, const char *data, int N);
/**
 * Move N bytes from the start of buffer into output. 
 * The N first bytes of buffer will be removed.
 * Return false if the size of buffer is smaller than N
 */
RP_EXPORTS bool extractFromStart(std::vector<char>& output, std::vector<char>& buffer, int N);
//Returns true if val is ' ', '\t', '\r', or '\n'
RP_EXPORTS bool isEmptyChar(char val);
//Returns true if val is [a-z],[A-Z], or '_'
RP_EXPORTS bool isStringChar(char val);
//Returns true if val is [0-9]
RP_EXPORTS bool isDigitChar(char val);
RP_EXPORTS void skipEmpty(const char *str, int length, int *currentPos);
RP_EXPORTS void skipNonEmpty(const char *str, int length, int *currentPos);
RP_EXPORTS void skipNonString(const char *str, int length, int *currentPos);
RP_EXPORTS std::string parseString(const char *str, int length, int *currentPos);
RP_EXPORTS std::string parseIntString(const char *str, int length, int *currentPos, bool with_minus);
RP_EXPORTS int parseInt(const char *str, int length, int *currentPos);
RP_EXPORTS std::string parseDoubleString(const char *str, int length, int *currentPos);
RP_EXPORTS double parseDouble(const char *str, int length, int *currentPos);
RP_EXPORTS std::vector<double> parseVectorDouble(const char *str, int length, int *currentPos, int nbElem);


}











