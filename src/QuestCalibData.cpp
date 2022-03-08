#include <libQuestMR/QuestCalibData.h>
#include <libQuestMR/QuestStringUtil.h>
#include <sstream>

namespace libQuestMR
{

std::string getString(tinyxml2::XMLElement *rootElem, const char *nodeName)
{
    return rootElem->FirstChildElement(nodeName)->GetText();
}

int getInt(tinyxml2::XMLElement *rootElem, const char *nodeName)
{
    int val;
    rootElem->FirstChildElement(nodeName)->QueryIntText(&val);
    return val;
}

double getDouble(tinyxml2::XMLElement *rootElem, const char *nodeName)
{
    double val;
    rootElem->FirstChildElement(nodeName)->QueryDoubleText(&val);
    return val;
}

std::vector<std::string> splitStringByWhitespace(const std::string& str)
{
    std::vector<std::string> list;
    int currentPos = 0;
    skipEmpty(str.c_str(), str.length(), &currentPos);
    while(currentPos < str.length())
    {
        int start = currentPos;
        skipNonEmpty(str.c_str(), str.length(), &currentPos);
        std::string val = str.substr(start, currentPos - start);
        list.push_back(val);
        skipEmpty(str.c_str(), str.length(), &currentPos);
    }
    return list;
}

std::vector<double> getDoubleList(tinyxml2::XMLElement *rootElem, const char *nodeName)
{
    std::string str = getString(rootElem, nodeName);
    std::vector<std::string> list = splitStringByWhitespace(str);
    std::vector<double> listDouble(list.size());
    for(int i = 0; i < list.size(); i++)
    {
        std::istringstream os(list[i]);
        os >> listDouble[i];
    }
    return listDouble;
}

std::vector<double> getMatrix(tinyxml2::XMLElement *rootElem, const char *nodeName, int *rows = NULL, int *cols = NULL, std::string *type = NULL)
{
    tinyxml2::XMLElement *elem = rootElem->FirstChildElement(nodeName);
    if(rows != NULL)
        *rows = getInt(elem, "rows");
    if(cols != NULL)
        *cols = getInt(elem, "cols");
    if(type != NULL)
        *type = getString(elem, "dt");
    std::string data = getString(elem, "data");
    return getDoubleList(elem, "data");
}

template<typename T>
std::string makeXmlString(std::string tag, T val)
{
    std::stringstream ss;
    ss << "<" << tag << ">" << val << "</" << tag << ">";
    return ss.str();
}

template<>
std::string makeXmlString<unsigned char>(std::string tag, unsigned char val)
{
    std::stringstream ss;
    ss << "<" << tag << ">" << ((unsigned int)val) << "</" << tag << ">";
    return ss.str();
}

template<typename T>
std::string makeXmlStringFromMatrix(std::string tag, const std::vector<T>& data, int rows, int cols, std::string dt = "d")
{
    if(rows < 0)
        rows = data.size() / cols;
    if(cols < 0)
        cols = data.size() / rows;
    std::stringstream ss;
    ss << "<" << tag << " type_id=\"opencv-matrix\"><rows>" << rows << "</rows><cols>" << cols << "</cols><dt>" << dt << "</dt><data>";
    ss << std::scientific << std::setprecision(16);
    for(int i = 0; i < data.size(); i++)
        ss << data[i] << " ";
    ss << "</data></"+tag+">";
    return ss.str();
}




#ifdef LIBQUESTMR_USE_OPENCV
cv::Mat vec2mat(const std::vector<double>& vec, int rows, int cols)
{
    if(rows == -1)
        rows = vec.size() / cols;
    if(cols == -1)
        cols = vec.size() / rows;
    if(vec.size() != rows*cols)
    {
        printf("vec2mat error : %d != %d*%d\n", vec.size(), rows, cols);
        return cv::Mat();
    }
    cv::Mat mat(rows, cols, CV_64F);
    for(int i = 0; i < rows; i++)
        for(int j = 0; j < cols; j++)
            mat.at<double>(i, j) = vec[i*cols+j];
    return mat;
}

template<typename T>
std::vector<T> mat2vec(const cv::Mat& mat)
{
    std::vector<T> vec;
    for(int i = 0; i < mat.rows; i++)
    {
        const T *data = mat.ptr<T>(i);
        for(int j = 0; j < mat.cols; j++)
            vec.push_back(data[j]);
    }
    return vec;
}

cv::Mat quaternion2rotationMat(const cv::Mat& quaternion)
{
    double qx = quaternion.at<double>(0,0);
    double qy = quaternion.at<double>(1,0);
    double qz = quaternion.at<double>(2,0);
    double qw = quaternion.at<double>(3,0);

    cv::Quat<double> quat(qw, qx, qy, qz);
    return cv::Mat(quat.toRotMat3x3());


    /*double n = qw*qw + qx*qx + qy*qy + qz*qz;//q.lengthSquared();
    float s =  n == 0?  0 : 2 / n;
    float wx = s * qw * qx, wy = s * qw * qy, wz = s * qw * qz;
    float xx = s * qx * qx, xy = s * qx * qy, xz = s * qx * qz;
    float yy = s * qy * qy, yz = s * qy * qz, zz = s * qz * qz;

    double m[16] = { 1 - (yy + zz),         xy + wz ,         xz - wy ,0,
                        xy - wz ,    1 - (xx + zz),         yz + wx ,0,
                        xz + wy ,         yz - wx ,    1 - (xx + yy),0,
                            0 ,               0 ,               0 ,1  };
    
    cv::Mat R(3,3,CV_64F);
    R.at<double>(0,0) = 1 - (yy + zz);   R.at<double>(0,1) = xy + wz;   R.at<double>(0,2) = xz - wy;
    R.at<double>(1,0) = xy - wz;   R.at<double>(1,1) = 1 - (xx + zz);   R.at<double>(1,2) = yz + wx;
    R.at<double>(2,0) = xz + wy;   R.at<double>(2,1) = yz - wx;   R.at<double>(2,2) = 1 - (xx + yy);

    //cv::Mat R;
    //cv::Rodrigues(quaternion2rvec(quaternion), R);

    return R.t();*/
}

cv::Mat quaternion2rvec(const cv::Mat& quaternion)
{
    double qx = quaternion.at<double>(0,0);
    double qy = quaternion.at<double>(1,0);
    double qz = quaternion.at<double>(2,0);
    double qw = quaternion.at<double>(3,0);
    if(qw > 1)
    {
        printf("normalize quaternion\n");
        double norm = sqrt(qw*qw + qx*qx + qy*qy + qz*qz);
        qw /= norm;
        qx /= norm;
        qy /= norm;
        qz /= norm;
    }
    double angle = 2 * acos(qw);
    double s = sqrt(1-qw*qw);
    double x,y,z;
    if (s < 0.001) {
        x = 1;
        y = 0;
        z = 0;
    } else {
        x = qx / s;
        y = qy / s;
        z = qz / s;
    }
    cv::Mat result(3,1,CV_64F);
    result.at<double>(0,0) = angle*x;
    result.at<double>(1,0) = angle*y;
    result.at<double>(2,0) = angle*z;
    return result;
}
cv::Mat vec2Rt(const cv::Mat rvec, const cv::Mat& tvec)
{
    cv::Mat Rt = cv::Mat::eye(4,4,CV_64F);
    cv::Rodrigues(rvec, Rt(cv::Rect(0,0,3,3)));
    tvec.copyTo(Rt(cv::Rect(3,0,1,3)));
    return Rt;
}
#endif









QuestCalibData::QuestCalibData()
{
    image_width = 0;
    image_height = 0;
    attachedDevice = -1;
    camDelayMs = 0;
    chromaKeyColorRed = 0;
    chromaKeyColorGreen = 0;
    chromaKeyColorBlue = 0;
    chromaKeySimilarity = 0;
    chromaKeySmoothRange = 0;
    chromaKeySpillRange = 0;
}

void QuestCalibData::loadXML(tinyxml2::XMLDocument& doc)
{
    tinyxml2::XMLElement *rootElem = doc.FirstChildElement("opencv_storage");
    camera_id = getString(rootElem, "camera_id");
    camera_name = getString(rootElem, "camera_name");
    image_width = getInt(rootElem, "image_width");
    image_height = getInt(rootElem, "image_height");
    camera_matrix = getMatrix(rootElem, "camera_matrix");
    distortion_coefficients = getMatrix(rootElem, "distortion_coefficients");
    translation = getMatrix(rootElem, "translation");
    rotation = getMatrix(rootElem, "rotation");
    attachedDevice = getInt(rootElem, "attachedDevice");
    camDelayMs = getInt(rootElem, "camDelayMs");
    chromaKeyColorRed = getInt(rootElem, "chromaKeyColorRed");
    chromaKeyColorGreen = getInt(rootElem, "chromaKeyColorGreen");
    chromaKeyColorBlue = getInt(rootElem, "chromaKeyColorBlue");
    chromaKeySimilarity = getDouble(rootElem, "chromaKeySimilarity");
    chromaKeySmoothRange = getDouble(rootElem, "chromaKeySmoothRange");
    chromaKeySpillRange = getDouble(rootElem, "chromaKeySpillRange");
    raw_translation = getMatrix(rootElem, "raw_translation");
    raw_rotation = getMatrix(rootElem, "raw_rotation");
}

std::string QuestCalibData::generateXMLString() const
{
    std::stringstream ss;
    ss << "<?xml version=\"1.0\"?><opencv_storage>";
    ss << makeXmlString("camera_id", camera_id);
    ss << makeXmlString("camera_name", camera_name);
    ss << makeXmlString("image_width", image_width);
    ss << makeXmlString("image_height", image_height);
    ss << makeXmlStringFromMatrix("camera_matrix", camera_matrix, 3, 3);
    ss << makeXmlStringFromMatrix("distortion_coefficients", distortion_coefficients, -1, 1);
    ss << makeXmlStringFromMatrix("translation", translation, -1, 1);
    ss << makeXmlStringFromMatrix("rotation", rotation, -1, 1);
    ss << makeXmlString("attachedDevice", attachedDevice);
    ss << makeXmlString("camDelayMs", camDelayMs);
    ss << makeXmlString("chromaKeyColorRed", chromaKeyColorRed);
    ss << makeXmlString("chromaKeyColorGreen", chromaKeyColorGreen);
    ss << makeXmlString("chromaKeyColorBlue", chromaKeyColorBlue);
    ss << makeXmlString("chromaKeySimilarity", chromaKeySimilarity);
    ss << makeXmlString("chromaKeySmoothRange", chromaKeySmoothRange);
    ss << makeXmlString("chromaKeySpillRange", chromaKeySpillRange);
    ss << makeXmlStringFromMatrix("raw_translation", raw_translation, -1, 1);
    ss << makeXmlStringFromMatrix("raw_rotation", raw_rotation, -1, 1);
    ss << "</opencv_storage>";
    return ss.str();
}

void QuestCalibData::loadXMLFile(const char *filename)
{
    tinyxml2::XMLDocument doc;
    doc.LoadFile( filename );
    loadXML(doc);
}

void QuestCalibData::loadXMLString(const char *str)
{
    tinyxml2::XMLDocument doc;
    doc.Parse(str);
    loadXML(doc);
}

#ifdef LIBQUESTMR_USE_OPENCV
cv::Mat QuestCalibData::getCameraMatrix() const { return vec2mat(camera_matrix, 3, 3); }
cv::Mat QuestCalibData::getDistCoeffs() const { return vec2mat(distortion_coefficients, -1, 1); }
cv::Mat QuestCalibData::getTranslation() const { return vec2mat(translation, 3, 1); }
cv::Mat QuestCalibData::getRotation() const { return vec2mat(rotation, 4, 1); }
cv::Mat QuestCalibData::getRawTranslation() const { return vec2mat(raw_translation, 3, 1); }
cv::Mat QuestCalibData::getRawRotation() const { return vec2mat(raw_rotation, 4, 1); }
cv::Mat QuestCalibData::getExtrinsicMat() const
{
    cv::Mat Rt = cv::Mat::eye(4,4,CV_64F);
    cv::Mat r = quaternion2rotationMat(getRotation());
    r.copyTo(Rt(cv::Rect(0,0,3,3)));
    getTranslation().copyTo(Rt(cv::Rect(3,0,1,3)));
    return Rt.inv();
}
cv::Mat QuestCalibData::getRawExtrinsicMat() const
{
    cv::Mat Rt = cv::Mat::eye(4,4,CV_64F);
    cv::Mat r = quaternion2rotationMat(getRawRotation());
    r.copyTo(Rt(cv::Rect(0,0,3,3)));
    getRawTranslation().copyTo(Rt(cv::Rect(3,0,1,3)));
    return Rt.inv();
}

cv::Mat QuestCalibData::getProjectionMat(bool withFlipX) const
{
    cv::Mat P = getCameraMatrix() * getExtrinsicMat()(cv::Rect(0,0,4,3));
    if(withFlipX)
        return getFlipXMat() * P;
    else return P;
}

cv::Mat QuestCalibData::getFlipXMat() const
{
    cv::Mat mat = cv::Mat::eye(3,3,CV_64F);
    mat.at<double>(0,0) = -1;
    mat.at<double>(0,2) = image_width-1;
    return mat;
}

cv::Point2d QuestCalibData::projectToCam(cv::Point3d p) const
{
    cv::Mat P = getProjectionMat();
    double *ptr[3] = {P.ptr<double>(0), P.ptr<double>(1), P.ptr<double>(2)};
    double z = ptr[2][0] * p.x + ptr[2][1] * p.y + ptr[2][2] * p.z + ptr[2][3];
    cv::Point2d result;
    result.x = (ptr[0][0] * p.x + ptr[0][1] * p.y + ptr[0][2] * p.z + ptr[0][3]) / z;
    result.y = (ptr[1][0] * p.x + ptr[1][1] * p.y + ptr[1][2] * p.z + ptr[1][3]) / z;
    //result = P*p
    return result;
}

bool QuestCalibData::calibrateCamPose(const std::vector<cv::Point3d>& listPoint3d, const std::vector<cv::Point2d>& listPoint2d)
{
    std::vector<cv::Point2d> listPoint2d_flip(listPoint2d.size());
    for(size_t i = 0; i < listPoint2d.size(); i++)
    {
        listPoint2d_flip[i].x = image_width - listPoint2d[i].x - 1;
        listPoint2d_flip[i].y = listPoint2d[i].y;
    }
    cv::Mat K = getCameraMatrix();
    cv::Mat distCoeffs = getDistCoeffs();
    cv::Mat rvec, tvec;
    if(!cv::solvePnP(listPoint3d, listPoint2d_flip, K, distCoeffs, rvec, tvec))
        return false;
    cv::Mat Rt = vec2Rt(rvec, tvec);
    cv::Mat Rt_inv = Rt.inv();
    cv::Quat<double> rot = cv::Quat<double>::createFromRotMat(Rt_inv(cv::Rect(0,0,3,3)));
    translation = mat2vec<double>(Rt_inv(cv::Rect(3,0,1,3)));
    rotation = {rot.x, rot.y, rot.z, rot.w};
    return true;
}
#endif


}
