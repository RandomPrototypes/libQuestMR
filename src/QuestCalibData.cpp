#include <libQuestMR/QuestCalibData.h>
#include <libQuestMR/QuestStringUtil.h>
#include <sstream>

namespace libQuestMR
{

std::string getString(tinyxml2::XMLElement *rootElem, const char *nodeName)
{
	auto *elem = rootElem->FirstChildElement(nodeName);
	if(elem == NULL)
		return "";
	const char *str = elem->GetText();
	if(str == NULL)
		return "";
    return str;
}

int getInt(tinyxml2::XMLElement *rootElem, const char *nodeName)
{
    int val = 0;
    auto *elem = rootElem->FirstChildElement(nodeName);
    if(elem == NULL)
		return 0;
    elem->QueryIntText(&val);
    return val;
}

double getDouble(tinyxml2::XMLElement *rootElem, const char *nodeName)
{
    double val = 0;
    auto *elem = rootElem->FirstChildElement(nodeName);
    if(elem == NULL)
		return 0;
    elem->QueryDoubleText(&val);
    return val;
}

std::vector<std::string> splitStringByWhitespace(const std::string& str)
{
    std::vector<std::string> list;
    int currentPos = 0;
    skipEmpty(str.c_str(), (int)str.length(), &currentPos);
    while(currentPos < str.length())
    {
        int start = currentPos;
        skipNonEmpty(str.c_str(), (int)str.length(), &currentPos);
        std::string val = str.substr(start, currentPos - start);
        list.push_back(val);
        skipEmpty(str.c_str(), (int)str.length(), &currentPos);
    }
    return list;
}

bool getDoubleList(tinyxml2::XMLElement *rootElem, const char *nodeName, double *buf, int maxLength)
{
    std::string str = getString(rootElem, nodeName);
    std::vector<std::string> list = splitStringByWhitespace(str);
    if(list.size() > maxLength)
        return false;
    for(int i = 0; i < list.size(); i++)
    {
        std::istringstream os(list[i]);
        os >> buf[i];
    }
    return true;
}

bool getMatrix(tinyxml2::XMLElement *rootElem, const char *nodeName, double *buf, int maxLength, int *rows = NULL, int *cols = NULL, std::string *type = NULL)
{
    tinyxml2::XMLElement *elem = rootElem->FirstChildElement(nodeName);
    int R = getInt(elem, "rows");
    int C = getInt(elem, "cols");
    if(rows != NULL)
        *rows = R;
    if(cols != NULL)
        *cols = C;
    if(C*R > maxLength)
        return false;
    if(type != NULL)
        *type = getString(elem, "dt");
    std::string data = getString(elem, "data");
    return getDoubleList(elem, "data", buf, maxLength);
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
std::string makeXmlStringFromMatrix(std::string tag, const T* data, int rows, int cols, std::string dt = "d")
{
    std::stringstream ss;
    ss << "<" << tag << " type_id=\"opencv-matrix\"><rows>" << rows << "</rows><cols>" << cols << "</cols><dt>" << dt << "</dt><data>";
    ss << std::scientific << std::setprecision(16);
    for(int i = 0; i < rows * cols; i++)
        ss << data[i] << " ";
    ss << "</data></"+tag+">";
    return ss.str();
}




#ifdef LIBQUESTMR_USE_OPENCV
cv::Mat vec2mat(const double* vec, int rows, int cols)
{
    cv::Mat mat(rows, cols, CV_64F);
    for(int i = 0; i < rows; i++)
        for(int j = 0; j < cols; j++)
            mat.at<double>(i, j) = vec[i*cols+j];
    return mat;
}

template<typename T>
void mat2vec(const cv::Mat& mat, T *vec, int length)
{
    if(mat.cols * mat.rows != length) {
        throw std::runtime_error("mat.cols * mat.rows != length");
    }
    if(mat.type() == CV_32F) {
		for(int i = 0; i < mat.rows; i++)
		{
            T *dst = vec + i * mat.cols;
		    const float *data = mat.ptr<float>(i);
		    for(int j = 0; j < mat.cols; j++)
		        dst[j] = (T)data[j];
		}
	} else if(mat.type() == CV_64F) {
		for(int i = 0; i < mat.rows; i++)
		{
            T *dst = vec + i * mat.cols;
		    const double *data = mat.ptr<double>(i);
		    for(int j = 0; j < mat.cols; j++)
		        dst[j] = (T)data[j];
		}
	} else if(!mat.empty()) {
		throw std::runtime_error("wrong mat type");
	}
}

cv::Mat quaternion2rotationMat(const cv::Mat& quaternion)
{
    double qx = quaternion.at<double>(0,0);
    double qy = quaternion.at<double>(1,0);
    double qz = quaternion.at<double>(2,0);
    double qw = quaternion.at<double>(3,0);

	//Quat is only available in OpenCV 4, so we use the formula directly for retro-compatibility. 
    //cv::Quat<double> quat(qw, qx, qy, qz);
    //return cv::Mat(quat.toRotMat3x3());


    double n = qw*qw + qx*qx + qy*qy + qz*qz;
	double s =  n == 0?  0 : 2 / n;
	double wx = s * qw * qx, wy = s * qw * qy, wz = s * qw * qz;
	double xx = s * qx * qx, xy = s * qx * qy, xz = s * qx * qz;
	double yy = s * qy * qy, yz = s * qy * qz, zz = s * qz * qz;
	
	cv::Mat R(3,3,CV_64F);
	R.at<double>(0,0) = 1 - (yy + zz);   R.at<double>(0,1) = xy + wz;   R.at<double>(0,2) = xz - wy;
	R.at<double>(1,0) = xy - wz;   R.at<double>(1,1) = 1 - (xx + zz);   R.at<double>(1,2) = yz + wx;
	R.at<double>(2,0) = xz + wy;   R.at<double>(2,1) = yz - wx;   R.at<double>(2,2) = 1 - (xx + yy);

	return R.t();
}

cv::Mat rotationMat2quaternion(const cv::Mat& R)
{
	cv::Mat rvec;
	cv::Rodrigues(R(cv::Rect(0,0,3,3)), rvec);
	double angle = sqrt(rvec.dot(rvec));
	double qw, qx, qy, qz;
	if (angle < 1.e-6) {
	    qw = 1;
	    qx = qy = qz = 0;
	} else {
		qw = cos(angle/2);
		double S = sin(angle/2) / angle;
		qx = S * rvec.at<double>(0,0);
		qy = S * rvec.at<double>(1,0);
		qz = S * rvec.at<double>(2,0);
	}
	cv::Mat quaternion(4,1,CV_64F);
	quaternion.at<double>(0,0) = qx;
    quaternion.at<double>(1,0) = qy;
    quaternion.at<double>(2,0) = qz;
    quaternion.at<double>(3,0) = qw;
	return quaternion;
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
cv::Mat vec2RtMat(const cv::Mat rvec, const cv::Mat& tvec)
{
    cv::Mat Rt = cv::Mat::eye(4,4,CV_64F);
    cv::Rodrigues(rvec, Rt(cv::Rect(0,0,3,3)));
    tvec.copyTo(Rt(cv::Rect(3,0,1,3)));
    return Rt;
}
cv::Mat RtMat2rvec(const cv::Mat Rt)
{
    cv::Mat rvec;
    cv::Rodrigues(Rt(cv::Rect(0,0,3,3)), rvec);
    return rvec;
}
cv::Mat RtMat2tvec(const cv::Mat Rt)
{
    return Rt(cv::Rect(3,0,1,3)).clone();
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
    for(int i = 0; i < 9; i++)
        camera_matrix[i] = 0;
    for(int i = 0; i < 8; i++)
        distortion_coefficients[i] = 0;
    for(int i = 0; i < 3; i++) {
        translation[i] = 0;
        raw_translation[i] = 0;
    }
    for(int i = 0; i < 4; i++) {
        rotation[i] = 0;
        raw_rotation[i] = 0;
    }
    rotation[0] = 1;
    raw_rotation[0] = 1;
}

QuestCalibData::QuestCalibData(const QuestCalibData& other)
{
    copyFrom(other);
}

QuestCalibData& QuestCalibData::operator=(const QuestCalibData& other)
{
    copyFrom(other);
    return *this;
}

void QuestCalibData::copyFrom(const QuestCalibData& other)
{
    camera_id = other.camera_id;
    camera_name = other.camera_name;
    image_width = other.image_width;
    image_height = other.image_height;
    memcpy(camera_matrix, other.camera_matrix, sizeof(camera_matrix));
    memcpy(distortion_coefficients, other.distortion_coefficients, sizeof(distortion_coefficients));
    memcpy(translation, other.translation, sizeof(translation));
    memcpy(rotation, other.rotation, sizeof(rotation));
    attachedDevice = other.attachedDevice;
    camDelayMs = other.camDelayMs;
    chromaKeyColorRed = other.chromaKeyColorRed;
    chromaKeyColorGreen = other.chromaKeyColorGreen;
    chromaKeyColorBlue = other.chromaKeyColorBlue;
    chromaKeySimilarity = other.chromaKeySimilarity;
    chromaKeySmoothRange = other.chromaKeySmoothRange;
    chromaKeySpillRange = other.chromaKeySpillRange;
    memcpy(raw_translation, other.raw_translation, sizeof(raw_translation));
    memcpy(raw_rotation, other.raw_rotation, sizeof(raw_rotation));

}

QuestCalibData::~QuestCalibData()
{
}

const char *QuestCalibData::getCameraId() const
{
    return camera_id.c_str();
}

void QuestCalibData::setCameraId(const char *id)
{
    camera_id = id;
}

const char *QuestCalibData::getCameraName() const
{
    return camera_name.c_str();
}

void QuestCalibData::setCameraName(const char *name)
{
    camera_name = name;
}

void QuestCalibData::loadXML(tinyxml2::XMLDocument& doc)
{
    tinyxml2::XMLElement *rootElem = doc.FirstChildElement("opencv_storage");
    if(rootElem == NULL) {
    	printf("can not load calib XML\n");
    	return ;
    }
    setCameraId(getString(rootElem, "camera_id").c_str());
    setCameraName(getString(rootElem, "camera_name").c_str());
    image_width = getInt(rootElem, "image_width");
    image_height = getInt(rootElem, "image_height");
    getMatrix(rootElem, "camera_matrix", camera_matrix, 9);
    getMatrix(rootElem, "distortion_coefficients", distortion_coefficients, 8);
    getMatrix(rootElem, "translation", translation, 3);
    getMatrix(rootElem, "rotation", rotation, 4);
    attachedDevice = getInt(rootElem, "attachedDevice");
    camDelayMs = getInt(rootElem, "camDelayMs");
    chromaKeyColorRed = getInt(rootElem, "chromaKeyColorRed");
    chromaKeyColorGreen = getInt(rootElem, "chromaKeyColorGreen");
    chromaKeyColorBlue = getInt(rootElem, "chromaKeyColorBlue");
    chromaKeySimilarity = getDouble(rootElem, "chromaKeySimilarity");
    chromaKeySmoothRange = getDouble(rootElem, "chromaKeySmoothRange");
    chromaKeySpillRange = getDouble(rootElem, "chromaKeySpillRange");
    getMatrix(rootElem, "raw_translation", raw_translation, 3);
    getMatrix(rootElem, "raw_rotation", raw_rotation, 4);
}

PortableString QuestCalibData::generateXMLString() const
{
    std::stringstream ss;
    ss << "<?xml version=\"1.0\"?><opencv_storage>";
    ss << makeXmlString("camera_id", std::string(getCameraId()));
    ss << makeXmlString("camera_name", std::string(getCameraName()));
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
    return toPortableString(ss.str());
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

void QuestCalibData::setCameraFromSizeAndFOV(double fov_x, int width, int height)
{
    double f = static_cast<double>(width) / (2*tan(fov_x / 2));
    camera_matrix[0] = f;   camera_matrix[1] = 0.0; camera_matrix[2] = width / 2.0;
    camera_matrix[3] = 0.0; camera_matrix[4] = f;   camera_matrix[5] = height / 2.0;
    camera_matrix[6] = 0.0; camera_matrix[7] = 0.0; camera_matrix[8] = 1.0;
    for(int i = 0; i < 8; i++)
        distortion_coefficients[i] = 0;
    image_width = width;
    image_height = height;
}

#ifdef LIBQUESTMR_USE_OPENCV
cv::Mat QuestCalibData::getCameraMatrix() const { return vec2mat(camera_matrix, 3, 3); }
void    QuestCalibData::setCameraMatrix(const cv::Mat& K) { mat2vec<double>(K, camera_matrix, 9); }
cv::Mat QuestCalibData::getDistCoeffs() const { return vec2mat(distortion_coefficients, 8, 1); }
void    QuestCalibData::setDistCoeffs(const cv::Mat& distCoeffs) { mat2vec<double>(distCoeffs, distortion_coefficients, 8); }
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
    cv::Mat Rt = vec2RtMat(rvec, tvec);
    cv::Mat Rt_inv = Rt.inv();
    //Quat is only available in OpenCV 4, so we use the formula directly for retro-compatibility. 
    //cv::Quat<double> rot = cv::Quat<double>::createFromRotMat(Rt_inv(cv::Rect(0,0,3,3)));
    //rotation = {rot.x, rot.y, rot.z, rot.w};
    cv::Mat rot = rotationMat2quaternion(Rt_inv(cv::Rect(0,0,3,3)));
    for(int i = 0; i < 4; i++)
        rotation[i] = rot.at<double>(i,0);
    mat2vec<double>(Rt_inv(cv::Rect(3,0,1,3)), translation, 3);
    return true;
}
#endif


}
