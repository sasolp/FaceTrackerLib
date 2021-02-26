#pragma once
#include <NewFaceDetectionLib.h>
#include <WrapperFunctions.h>
typedef int(*FACE_STREAM_CREATE)(char* id);
typedef vector<FaceCase> FaceStream;

class MultiFaceTracker
{
public:
	MultiFaceTracker();
	int create(int max_matching_point_count = 300, int norm_type = cv::NORM_L2SQR);
	void set_image(cv::Mat &img);
	void set_faces_img(cv::Mat &img);
	int update(time_t current_time,vector<int>jpg_high_params, int);
	int add_new_face(int new_face_index);
	int remove(int case_index, int face_index);
	int remove(int case_index);
	void set_max_queue_size(int max_queue_size_);
	void set_faces(DetectedFace**faces_, int faces_count_);
	bool is_got_out_face(cv::Rect2d face_rect);
	void reset_all();
public:
	vector<thread>arr_threads_to_save_images;
	vector<cv::Rect2d> new_faces;
	int new_faces_count;
	vector<cv::Rect2d> tracked_faces_box;
	vector<FaceStream> tracked_faces;
	string prifix_path;
	vector<cv::Mat> curr_frame_face_descripts;
	vector<cv::Mat> curr_frame_face_mats;
	
private:

	int refine_rect(cv::Rect2d &rect);
	void add_to_face_stream(int index, cv::Mat& face_img, cv::Mat& descripts);
	int find_face(cv::Rect2d detected_face_rect, int);
private:
	int max_matching_point_count;
	int max_queue_size;
	DetectedFace ** faces;
	int faces_count;
	vector<vector<cv::Mat>> prev_face_descripts;
	vector<vector<cv::Mat>> prev_face_mats;
	vector<int> prev_losses_cunt;
	cv::Mat frame;
	cv::Mat faces_img;
	cv::Ptr<cv::GFTTDetector> keypoint_detector;
	cv::Ptr<cv::AKAZE> feature_ext;
	cv::Ptr<cv::BFMatcher> bf_matcher;
	int frame_height;
	int frame_width;
};
