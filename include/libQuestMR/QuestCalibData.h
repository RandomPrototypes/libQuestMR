#pragma once

#include <string>
#include <vector>

#include "tinyxml2.h"

#include "config.h"

#ifdef LIBQUESTMR_USE_OPENCV
#include <opencv2/opencv.hpp>
#include <opencv2/core/quaternion.hpp>
#endif

namespace libQuestMR
{

class QuestCalibData
{
public:
    std::string camera_id;
    std::string camera_name;
    int image_width, image_height;//image size
    std::vector<double> camera_matrix;//3x3 intrinsic matrix
    std::vector<double> distortion_coefficients;//8x1 distortion coeffs matrix
    std::vector<double> translation;//3x1 translation (x,y,z)
    std::vector<double> rotation;//4x1 quaternion (x,y,z,w)
    int attachedDevice;
    int camDelayMs;
    unsigned char chromaKeyColorRed, chromaKeyColorGreen, chromaKeyColorBlue;
    double chromaKeySimilarity;
    double chromaKeySmoothRange;
    double chromaKeySpillRange;
    std::vector<double> raw_translation;//3x1 raw translation (x,y,z), pose of the tracking space
    std::vector<double> raw_rotation;//4x1 raw quaternion (x,y,z,w), pose of the tracking space

    QuestCalibData();

    void loadXML(tinyxml2::XMLDocument& doc);

    std::string generateXMLString() const;

    void loadXMLFile(const char *filename);

    void loadXMLString(const char *str);

#ifdef LIBQUESTMR_USE_OPENCV
    cv::Mat getCameraMatrix() const;
    void setCameraMatrix(const cv::Mat& K);
    cv::Mat getDistCoeffs() const;
    void setDistCoeffs(const cv::Mat& distCoeffs);
    cv::Mat getTranslation() const;
    cv::Mat getRotation() const;
    cv::Mat getRawTranslation() const;
    cv::Mat getRawRotation() const;
    cv::Mat getExtrinsicMat() const;
    cv::Mat getRawExtrinsicMat() const;
    cv::Mat getProjectionMat(bool withFlipX = true) const;
    cv::Mat getFlipXMat() const;

    cv::Point2d projectToCam(cv::Point3d p) const;

    bool calibrateCamPose(const std::vector<cv::Point3d>& listPoint3d, const std::vector<cv::Point2d>& listPoint2d);
#endif
};

#ifdef LIBQUESTMR_USE_OPENCV
//from 4x1 (x,y,z,w) quaternion mat to 3x3 rotation matrix 
cv::Mat quaternion2rotationMat(const cv::Mat& quaternion);
//from 4x1 (x,y,z,w) quaternion mat to 3x1 rodrigues mat.
//TODO : not tested, need to verify if this works properly
cv::Mat quaternion2rvec(const cv::Mat& quaternion);
#endif

}
