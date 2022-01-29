#include "QuestFrameData.h"
#include "QuestStringUtil.h"


namespace libQuestMR
{

std::string QuestFrameData::toString(const std::vector<double>& list)
{
    std::string str;
    for(size_t i = 0; i < list.size(); i++)
    {
        str += std::to_string(list[i]);
        if(i+1 < list.size())
            str += ", ";
    }
    return str;
}

std::string QuestFrameData::toString()
{
    std::string str;
    str += "frame: "+std::to_string(frame)+"\n";
    str += "time: "+std::to_string(time)+"\n";
    str += "head_pos: "+toString(head_pos)+"\n";
    str += "head_rot: "+toString(head_rot)+"\n";
    str += "left_hand_pos: "+toString(left_hand_pos)+"\n";
    str += "left_hand_rot: "+toString(left_hand_rot)+"\n";
    str += "right_hand_pos: "+toString(right_hand_pos)+"\n";
    str += "right_hand_rot: "+toString(right_hand_rot)+"\n";
    str += "raw_pos: "+toString(raw_pos)+"\n";
    str += "raw_rot: "+toString(raw_rot)+"\n";
    str += "lht: "+std::to_string(lht)+"\n";
    str += "lhv: "+std::to_string(lhv)+"\n";
    str += "rht: "+std::to_string(rht)+"\n";
    str += "rhv: "+std::to_string(rhv)+"\n";
    return str;
}

void QuestFrameData::parse(const char* str, int length)
{
    int currentPos = 0;
    while(currentPos < length)
    {
        skipEmpty(str, length, &currentPos);
        std::string name = parseString(str, length, &currentPos);
        skipEmpty(str, length, &currentPos);
        if(name == "frame")
            frame = parseInt(str, length, &currentPos);
        else if(name == "time")
            time = parseDouble(str, length, &currentPos);
        else if(name == "head_pos")
            head_pos = parseVectorDouble(str, length, &currentPos, 3);
        else if(name == "head_rot")
            head_rot = parseVectorDouble(str, length, &currentPos, 4);
        else if(name == "left_hand_pos")
            left_hand_pos = parseVectorDouble(str, length, &currentPos, 3);
        else if(name == "left_hand_rot")
            left_hand_rot = parseVectorDouble(str, length, &currentPos, 4);
        else if(name == "right_hand_pos")
            right_hand_pos = parseVectorDouble(str, length, &currentPos, 3);
        else if(name == "right_hand_rot")
            right_hand_rot = parseVectorDouble(str, length, &currentPos, 4);
        else if(name == "raw_pos")
            raw_pos = parseVectorDouble(str, length, &currentPos, 3);
        else if(name == "raw_rot")
            raw_rot = parseVectorDouble(str, length, &currentPos, 4);
        else if(name == "lht")
            lht = parseInt(str, length, &currentPos);
        else if(name == "lhv")
            lhv = parseInt(str, length, &currentPos);
        else if(name == "rht")
            rht = parseInt(str, length, &currentPos);
        else if(name == "rhv")
            rhv = parseInt(str, length, &currentPos);
        else
        {
            printf("unknown type %s\n", name.c_str());
            skipNonString(str, length, &currentPos);
        }
        skipEmpty(str, length, &currentPos);
    }
}

bool QuestFrameData::isLeftHandValid() const
{
    return lht && lhv && left_hand_pos.size() == 3 && left_hand_rot.size() == 4;
}

bool QuestFrameData::isRightHandValid() const
{
    return rht && rhv && right_hand_pos.size() == 3 && right_hand_rot.size() == 4;
}

bool QuestFrameData::isHeadValid() const
{
    return head_pos.size() == 3 && head_rot.size() == 4;
}

#ifdef LIBQUESTMR_USE_OPENCV
cv::Point3d QuestFrameData::getLeftHandPos() const
{
    if(left_hand_pos.size() != 3)
        return cv::Point3d(0,0,0);
    return cv::Point3d(left_hand_pos[0], left_hand_pos[1], left_hand_pos[2]);
}

cv::Point3d QuestFrameData::getRightHandPos() const
{
    if(right_hand_pos.size() != 3)
        return cv::Point3d(0,0,0);
    return cv::Point3d(right_hand_pos[0], right_hand_pos[1], right_hand_pos[2]);
}

cv::Point3d QuestFrameData::getHeadPos() const
{
    if(head_pos.size() != 3)
        return cv::Point3d(0,0,0);
    return cv::Point3d(head_pos[0], head_pos[1], head_pos[2]);
}
#endif
}