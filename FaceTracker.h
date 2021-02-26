// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the FACETRACKER_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// FACETRACKER_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#include <NewFaceDetectionLib.h>
#include "ids_camera.h"
typedef int (*FACE_STREAM_CREATE)(char* id);
typedef struct _FaceRect
{
	int x;
	int y;
	int width;
	int height;
}FaceRect;
typedef struct _FaceObj
{
	char face_id[100];
	FaceRect face_rect;
	float* features;
	int tag;
	int is_lost;
}FaceObj;
class FaceTracker
{
public:
	FaceTracker();
	FaceTracker(string face_detect_dep, string face_extract_fld, 
		float min_confidence, UserFaceSettings face_settings, int device_id);
	FaceTracker(uchar* image_data, int width, int height, int channels, 
		int stride, float min_confidence, UserFaceSettings face_settings);
	void DestroyAll();
	
	int create_face_detector();
	int create_face_extractor(int which);
	int extract_faces();
	int reset_all();
	void set_image(uchar* image_data, int width, int height, int channels, int stride);
	void set_image(cv::Mat &img);
	int set_capture(std::string path);
	int set_ids(IDSCamera * cap);
	int set_capture(cv::VideoCapture cap);
	void set_gpu_id(int device_id);
	void set_min_confidence(float cnf);
	void set_user_settings(UserFaceSettings user_settings);
	int createTracker(string trackerType);
	int track();
	int track0();
	void set_transposed(bool be_transposed);
	void set_extraction_mode(bool have_to_feature_extraction_);
	int set_repository_path(string pre_path);
	int set_cam_name(string cam_path, bool be_erase);
	int set_roi(cv::Rect roi);
	int refine_rect(cv::Rect &rect);
	int refine_rect(FaceRect &rect);
	int get_next_frame(string recognized_list, bool is_registeration_mode, int new_width, int new_height, int stride, uchar* buffer, FaceObj** out_faces, int *out_faces_count);
public:
	int cap_type;
	int FACE_FEATURE_SIZE;
	long long frame_counter;
	DetectedFace ** faces;
	int faces_count;
	string cam_path;
	cv::Mat frame_grabber;
	cv::VideoCapture cam_cap;
	IDSCamera * ids_camera;
	FACE_STREAM_CREATE face_stream_created;
	int worker_thread_id;
	thread grabber_thread;
private:
	vector<thread> arr_threads_to_save_images;
	mutex mutex_obj;
	bool have_to_feature_extraction;
	MultiFaceTracker multi_face_tracker;
	vector<int>jpg_high_params;
	vector<int>jpg_low_params;
	bool is_set_repository_path;
	string repository_path;
	string frames_path;
	string cam_name;
	string prifix_path;
	string face_detection_folder;
	string face_extraction_folder;
	string tracker_type;
	cv::Mat current_img;
	FaceDetector* face_detector;
	Classifier* face_extractor;
	UserFaceSettings user_settings;
	cv::Rect roi;
	int frame_height;
	int frame_width;
	float cnf;
	int gpu_id;
	int frame_rate;
	bool must_be_transposed;
};
