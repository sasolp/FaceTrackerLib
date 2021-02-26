// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//



#include <opencv\cv.h>
#include <opencv2\opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2\tracking\tracking.hpp>
#include <opencv2\core\ocl.hpp>

#include <vector>
#include <math.h>
#include <map>
#include <complex>
#include <ctime>
#include <iostream>
#include <cstdlib>
#include<thread>
 
using namespace std;
#define _EXE0

#define USB 0
#define IDS 1
#define IP_URL 2
#define VIDEO_File 3

#ifdef _EXE
#define DISPLAY_FRAMES 
#define SAVE_FRAMES 
#define DISPLAY_FACES
#define SAVE_FACES 
#endif

extern int WAIT_DELAY  ;
extern int MAX_TRACKABLE_COUNT  ;

extern bool Save_Frames ;
extern bool Save_Faces ;
extern int SAVE_FRAME_RATE  ;
extern int PROCESS_FRAME_RATE ;
extern int MIN_FACE_SIZE_TO_RECOGNITION ; 
extern int MAX_FACE_SIZE_TO_DETECTION ; 
extern int MIN_FACE_COUNT_TO_DECLARE;
extern float MAX_MATCHED_FACES_DIST;
extern int MAX_FRAME_WIDTH;
extern int MAX_FRAME_HEIGHT;
extern int TRACK_QUEUE_SIZE;
extern int Frame_Steps;
// TODO: reference additional headers your program requires here
void logger(char* file_name, char * format, ...);


extern void read_from_data(cv::Mat &dst, uchar* image_data, int width, int height, int channels, int stride);
extern void write_to_data(cv::Mat &src, uchar* image_data, int width, int height, int channels, int stride);


