#include <libQuestMR/QuestFrameData.h>
#include <libQuestMR/QuestStringUtil.h>


namespace libQuestMR
{

std::string QuestFrameData::toString(double *list, int size)
{
    std::string str;
    for(size_t i = 0; i < size; i++)
    {
        str += std::to_string(list[i]);
        if(i+1 < size)
            str += ", ";
    }
    return str;
}

std::string QuestFrameData::toString()
{
    std::string str;
    str += "frame: "+std::to_string(frame)+"\n";
    str += "time: "+std::to_string(time)+"\n";
    str += "head_pos: "+toString(head_pos, 3)+"\n";
    str += "head_rot: "+toString(head_rot, 4)+"\n";
    str += "left_hand_pos: "+toString(left_hand_pos, 3)+"\n";
    str += "left_hand_rot: "+toString(left_hand_rot, 4)+"\n";
    str += "right_hand_pos: "+toString(right_hand_pos, 3)+"\n";
    str += "right_hand_rot: "+toString(right_hand_rot, 4)+"\n";
    str += "raw_pos: "+toString(raw_pos, 3)+"\n";
    str += "raw_rot: "+toString(raw_rot, 4)+"\n";
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
            parseVectorDouble(str, length, &currentPos, 3, head_pos);
        else if(name == "head_rot")
            parseVectorDouble(str, length, &currentPos, 4, head_rot);
        else if(name == "left_hand_pos")
            parseVectorDouble(str, length, &currentPos, 3, left_hand_pos);
        else if(name == "left_hand_rot")
            parseVectorDouble(str, length, &currentPos, 4, left_hand_rot);
        else if(name == "right_hand_pos")
            parseVectorDouble(str, length, &currentPos, 3, right_hand_pos);
        else if(name == "right_hand_rot")
            parseVectorDouble(str, length, &currentPos, 4, right_hand_rot);
        else if(name == "raw_pos")
            parseVectorDouble(str, length, &currentPos, 3, raw_pos);
        else if(name == "raw_rot")
            parseVectorDouble(str, length, &currentPos, 4, raw_rot);
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
    return lht && lhv;//todo : add check if data got read correctly
}

bool QuestFrameData::isRightHandValid() const
{
    return rht && rhv;//todo : add check if data got read correctly
}

bool QuestFrameData::isHeadValid() const
{
    return true;//todo : add check if data got read correctly
}

#ifdef LIBQUESTMR_USE_OPENCV
cv::Point3d QuestFrameData::getLeftHandPos() const
{
    return cv::Point3d(left_hand_pos[0], left_hand_pos[1], left_hand_pos[2]);
}

cv::Point3d QuestFrameData::getRightHandPos() const
{
    return cv::Point3d(right_hand_pos[0], right_hand_pos[1], right_hand_pos[2]);
}

cv::Point3d QuestFrameData::getHeadPos() const
{
    return cv::Point3d(head_pos[0], head_pos[1], head_pos[2]);
}
#endif
}
