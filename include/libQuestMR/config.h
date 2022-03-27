#pragma once

#define LIBQUESTMR_USE_OPENCV
#define LIBQUESTMR_USE_FFMPEG

#define QUEST_CALIB_DEFAULT_PORT 25671

//Taken from OpenCV

#ifndef LQMR_EXPORTS
# if (defined _WIN32 || defined WINCE || defined __CYGWIN__) && defined(LIBQUESTMR_EXPORTS)
#   define LQMR_EXPORTS __declspec(dllexport)
# elif defined __GNUC__ && __GNUC__ >= 4 && (defined(LIBQUESTMR_EXPORTS) || defined(__APPLE__))
#   define LQMR_EXPORTS __attribute__ ((visibility ("default")))
# endif
#endif

#ifndef LQMR_EXPORTS
# define LQMR_EXPORTS
#endif
