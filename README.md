# libQuestMR

Unofficial library that handles the communication with the Quest 2 for mixed reality and provides an easy bridge to OpenCV.  
Still a work in progress. Already usable but the interface may change a lot in future versions.  
A mixed reality capture software is on development [here](https://github.com/RandomPrototypes/RPMixedRealityCapture).

The master branch is hosted at https://github.com/RandomPrototypes/libQuestMR

### Dependencies
Networking is managed by [BufferedSocket](https://github.com/RandomPrototypes/BufferedSocket), a lighweight portable library I made. It's automatically downloaded by cmake.  

XML loading/writing is managed by [tinyxml2](https://github.com/leethomason/tinyxml2). It's automatically downloaded by cmake.  

Video encoding/decoding is managed by [FFMPEG](https://github.com/FFmpeg/FFmpeg). You need to install it and let cmake find it.  

[OpenCV](https://github.com/opencv/opencv) is a recommended dependency. Some parts of the project can work without it but many functions including the calibration and visualization would be disabled if compiled without OpenCV.  

[RPMixedRealityCapture](https://github.com/RandomPrototypes/RPMixedRealityCapture) is a recommended but optional dependency. It's a portable camera library I made to support some functions that were not available in OpenCV : camera enumeration, camera formats and resolution enumeration, MJPEG streaming, network camera (android phone with custom app,...).


### Credits
A part of the code is based on the official [OBS plugin for Quest 2](https://github.com/facebookincubator/obs-plugins).  
Most of the rest of the code is based on wireshark captures and a little bit of reading of the code of [RealityMixer](https://github.com/fabio914/RealityMixer)
