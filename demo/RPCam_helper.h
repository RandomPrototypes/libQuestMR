#pragma once

#include <RPCameraInterface/CameraInterface.h>

bool configureCamera(std::shared_ptr<RPCameraInterface::CameraInterface>& cam, RPCameraInterface::ImageFormat *resultFormat);
