#pragma once

#include <vector>
#include <string>

#include "config.h"

#ifdef LIBQUESTMR_USE_OPENCV
#include <opencv2/opencv.hpp>
#endif

namespace libQuestMR
{

class QuestFrameData
{
public:
    int frame;
    double time;
    std::vector<double> head_pos;
    std::vector<double> head_rot;
    std::vector<double> left_hand_pos;
    std::vector<double> left_hand_rot;
    std::vector<double> right_hand_pos;
    std::vector<double> right_hand_rot;
    std::vector<double> raw_pos;
    std::vector<double> raw_rot;
    int lht;
    int lhv;
    int rht;
    int rhv;

    std::string toString(const std::vector<double>& list);

    std::string toString();

    void parse(const char* str, int length);

    bool isLeftHandValid() const;
    bool isRightHandValid() const;
    bool isHeadValid() const;

    #ifdef LIBQUESTMR_USE_OPENCV
    cv::Point3d getLeftHandPos() const;
    cv::Point3d getRightHandPos() const;
    cv::Point3d getHeadPos() const;
    #endif
};


}