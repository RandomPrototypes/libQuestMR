#pragma once

#include <vector>
#include <string>

#include "config.h"

namespace libQuestMR
{

//copy N bytes from data to the end of buffer.
void addToBuffer(std::vector<char>& buffer, const char *data, int N);
/**
 * Move N bytes from the start of buffer into output. 
 * The N first bytes of buffer will be removed.
 * Return false if the size of buffer is smaller than N
 */
bool extractFromStart(std::vector<char>& output, std::vector<char>& buffer, int N);
//Returns true if val is ' ', '\t', '\r', or '\n'
bool isEmptyChar(char val);
//Returns true if val is [a-z],[A-Z], or '_'
bool isStringChar(char val);
//Returns true if val is [0-9]
bool isDigitChar(char val);
void skipEmpty(const char *str, int length, int *currentPos);
void skipNonEmpty(const char *str, int length, int *currentPos);
void skipNonString(const char *str, int length, int *currentPos);
std::string parseString(const char *str, int length, int *currentPos);
std::string parseIntString(const char *str, int length, int *currentPos, bool with_minus);
int parseInt(const char *str, int length, int *currentPos);
std::string parseDoubleString(const char *str, int length, int *currentPos);
double parseDouble(const char *str, int length, int *currentPos);
std::vector<double> parseVectorDouble(const char *str, int length, int *currentPos, int nbElem);


}











