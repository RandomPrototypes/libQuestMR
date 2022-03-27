#pragma once

#include <vector>
#include <string>

#include "config.h"

#ifdef LIBQUESTMR_USE_OPENCV
#include <opencv2/opencv.hpp>
#endif

namespace libQuestMR
{

class LQMR_EXPORTS QuestFrameData
{
public:
    int frame;
    double time;
    double head_pos[3];
    double head_rot[4];
    double left_hand_pos[3];
    double left_hand_rot[4];
    double right_hand_pos[3];
    double right_hand_rot[4];
    double raw_pos[3];
    double raw_rot[4];
    int lht;
    int lhv;
    int rht;
    int rhv;

    std::string toString(double *list, int size);

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
