#pragma once

#define LIBQUESTMR_USE_OPENCV
#define LIBQUESTMR_USE_FFMPEG

#define QUEST_CALIB_DEFAULT_PORT 25671

//Taken from OpenCV

#ifndef RP_EXPORTS
# if (defined _WIN32 || defined WINCE || defined __CYGWIN__) && defined(RPCAMERAINTERFACE_EXPORTS)
#   define RP_EXPORTS __declspec(dllexport)
# elif defined __GNUC__ && __GNUC__ >= 4 && (defined(RPCAMERAINTERFACE_EXPORTS) || defined(__APPLE__))
#   define RP_EXPORTS __attribute__ ((visibility ("default")))
# endif
#endif

#ifndef RP_EXPORTS
# define RP_EXPORTS
#endif
