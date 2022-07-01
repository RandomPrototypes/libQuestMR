# libQuestMR

Unofficial library that handles the communication with the Quest 2 for mixed reality and provides an easy bridge to OpenCV.  
Nearly ready for a first release. Already usable but the interface may change a little bit in future versions.  
A mixed reality capture software is on development [here](https://github.com/RandomPrototypes/RPMixedRealityCapture).

The master branch is hosted at https://github.com/RandomPrototypes/libQuestMR

[![IMAGE ALT TEXT](http://img.youtube.com/vi/5r5InOlSnu4/0.jpg)](https://www.youtube.com/watch?v=5r5InOlSnu4 "Custom Mixed Reality in C++ with libQuestMR")


### Features

Already implemented :  
* Communication with Oculus Mixed Reality Capture App (Headset and controller poses, trigger button, calibration load and upload)
* Calibration between external camera and headset
* Extraction and decoding of the frames from the headset
* Recording and playback of the frames from the headset
* Windows, Linux, Macos support
* Video matting without greenscreen
* Audio recording

Future plan :
* Moving camera
* Body tracking
* Android, iphone, maybe raspberry pi support (the code is portable but the makefiles probably need to be modified)

### Dependencies
Networking is managed by [BufferedSocket](https://github.com/RandomPrototypes/BufferedSocket), a lighweight portable library I made. 

XML loading/writing is managed by [tinyxml2](https://github.com/leethomason/tinyxml2). It's automatically downloaded by cmake.  

Video encoding/decoding is managed by [FFMPEG](https://github.com/FFmpeg/FFmpeg). You need to install it and let cmake find it.  

[OpenCV](https://github.com/opencv/opencv) is a recommended dependency. Some parts of the project can work without it but many functions including the calibration and visualization would be disabled if compiled without OpenCV.  

[RPMixedRealityCapture](https://github.com/RandomPrototypes/RPMixedRealityCapture) is a recommended but optional dependency. It's a portable camera library I made to support some functions that were not available in OpenCV : camera enumeration, camera formats and resolution enumeration, MJPEG streaming, network camera (android phone with custom app,...).

### Background subtraction without greenscreen

To use the deep learning background subtraction method, you need to download rvm_mobilenetv3_fp32.onnx from [RobustVideoMatting](https://github.com/PeterL1n/RobustVideoMatting/) project and specify the folder with the command setBackgroundSubtractorResourceFolder().  
Please check the license of this file before using it in your project.  


### Credits
A part of the code is based on the official [OBS plugin for Quest 2](https://github.com/facebookincubator/obs-plugins).  
Most of the rest of the code is based on wireshark captures and some reading of the code of [RealityMixer](https://github.com/fabio914/RealityMixer)
