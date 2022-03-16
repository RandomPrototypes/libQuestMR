#include "RPCam_helper.h"

using namespace RPCameraInterface;

bool configureCamera(std::shared_ptr<CameraInterface>& cam, ImageFormat *resultFormat)
{
	//Obtain a camera enumerator using default backend
	std::shared_ptr<CameraEnumerator> camEnum = getCameraEnumerator();
    camEnum->detectCameras();
    printf("%d cameras detected\n", camEnum->count());
    for(int i = 0; i < camEnum->count(); i++) {
        printf("%d: %s, %s\n", i, camEnum->getCameraId(i).c_str(), camEnum->getCameraName(i).c_str());
    }
	
	int cameraId = -1;
	while(cameraId < 0 || cameraId >= camEnum->count())
	{
		printf("Which camera? ");
		scanf("%d", &cameraId);
	}
	
	//Obtain a camera interface using the same backend as the enumerator
    cam = getCameraInterface(camEnum->backend);
    //Open the camera using the id from the enumerator
    cam->open(camEnum->getCameraId(cameraId).c_str());
    //Get the list of available formats
    std::vector<ImageFormat> listFormats = cam->getAvailableFormats();
    for(size_t i = 0; i < listFormats.size(); i++) {
    	ImageFormat& format = listFormats[i];
        printf("%d: %dx%d (%s)\n", (int)i, format.width, format.height, toString(format.type).c_str());
    }
    
    int formatId = -1;
	while(formatId < 0 || formatId >= listFormats.size())
	{
		printf("Which format? ");
		scanf("%d", &formatId);
	}
	
	//Select the format
    cam->selectFormat(listFormats[formatId]);
    if(!cam->startCapturing()) {
        printf("error in startCapturing()\n");
    	return false;
    }
    *resultFormat = listFormats[formatId];
    return true;
}

