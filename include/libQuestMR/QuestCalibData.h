#pragma once

#include <libQuestMR/config.h>
#include <libQuestMR/PortableTypes.h>
#include <string>
#include <vector>

#include "tinyxml2.h"

#ifdef LIBQUESTMR_USE_OPENCV
#include <opencv2/opencv.hpp>
#endif

namespace libQuestMR
{

class QuestCalibDataPrivateData;

class LQMR_EXPORTS QuestCalibData
{
public:
    PortableString camera_id;
    PortableString camera_name;
    int image_width, image_height;//image size
    double camera_matrix[9];//3x3 intrinsic matrix
    double distortion_coefficients[8];//8x1 distortion coeffs matrix
    double translation[3];//3x1 translation (x,y,z)
    double rotation[4];//4x1 quaternion (x,y,z,w)
    int attachedDevice;
    int camDelayMs;
    unsigned char chromaKeyColorRed, chromaKeyColorGreen, chromaKeyColorBlue;
    double chromaKeySimilarity;
    double chromaKeySmoothRange;
    double chromaKeySpillRange;
    double raw_translation[3];//3x1 raw translation (x,y,z), pose of the tracking space
    double raw_rotation[4];//4x1 raw quaternion (x,y,z,w), pose of the tracking space

    QuestCalibData();
    QuestCalibData(const QuestCalibData& other);
    QuestCalibData& operator=(const QuestCalibData& other);
    ~QuestCalibData();

    void copyFrom(const QuestCalibData& other);

    const char *getCameraId() const;
    void setCameraId(const char *id);
    const char *getCameraName() const;
    void setCameraName(const char *name);

    PortableString generateXMLString() const;

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

private:
	void loadXML(tinyxml2::XMLDocument& doc);
};

#ifdef LIBQUESTMR_USE_OPENCV
//from 4x1 (x,y,z,w) quaternion mat to 3x3 rotation matrix 
LQMR_EXPORTS cv::Mat quaternion2rotationMat(const cv::Mat& quaternion);
//from 4x1 (x,y,z,w) quaternion mat to 3x1 rodrigues mat.
//TODO : not tested, need to verify if this works properly
LQMR_EXPORTS cv::Mat quaternion2rvec(const cv::Mat& quaternion);
#endif
}
