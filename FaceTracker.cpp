// FaceTracker.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include<direct.h>

#include "MultiFaceTracker.h"
#include "FaceTracker.h"
#include <Windows.h>
#include<ShlObj.h>
#include <Shlwapi.h>
#include <sysinfoapi.h>
// Convert to string
#define SSTR( x ) static_cast< std::ostringstream & >( \
( std::ostringstream() << std::dec << x ) ).str()
void get_time_string(time_t tr, char *str_time)
{
	SYSTEMTIME s;
	GetSystemTime(&s);
	/*tm * tm = localtime(&tr);
	clock_t c = clock();
	int miliseconds = c % 1000;
	int seconds = tm->tm_sec;
	int minutes = tm->tm_min;
	int hours = tm->tm_hour;
	int days = tm->tm_mday;
	int year = tm->tm_year + 1900;
	int month = tm->tm_mon + 1;*/
	
	tr *= 1000;
	tr += s.wMilliseconds;
	//tr = (LONGLONG)s.dwLowDateTime + ((LONGLONG)(s.dwLowDateTime) << 32LL);//s.wSecond * 1000 + s.wMilliseconds;
	
	//sprintf(str_time, "_%d_%d_%d_%d_%d_%d_%d", year, month, days, hours, minutes, seconds, miliseconds);
	sprintf(str_time, "%lld", tr);
}
void read_from_data(cv::Mat &dst, uchar* image_data, int width, int height, int channels, int stride)
{

	if (channels > 1)
	{
		dst = cv::Mat(height, width, CV_8UC3);
		uchar* dst_data = dst.data;
		for (int i = 0; i < height; i++)
		{
			memcpy(dst_data, image_data, width * channels);
			image_data += stride;
			dst_data += width * channels;
		}
	}
	else
	{
		dst = cv::Mat(height, width, CV_8UC1);
		uchar* dst_data = dst.data;
		for (int i = 0; i < height; i++)
		{
			memcpy(dst_data, image_data, width * channels);
			image_data += stride;
			dst_data += width * channels;
		}
		cv::cvtColor(dst, dst, CV_GRAY2BGR);
	}
}
FaceTracker::FaceTracker() 
{
	throw "Please Pass Parameters";
};
FaceTracker::FaceTracker(string face_detect_fld, string face_extract_fld, float min_confidence, UserFaceSettings face_settings, int device_id)
{
	face_detection_folder= face_detect_fld;
	face_extraction_folder= face_extract_fld;
	cnf = min_confidence;
	user_settings = face_settings;
	gpu_id = device_id;
	must_be_transposed = true;
	jpg_low_params.push_back(20);
	jpg_high_params.push_back(100);
	roi.x = -1;
	roi.y = -1;
	roi.width = -1;
	roi.height = -1;
	faces = 0;
	multi_face_tracker.create();
	
}
FaceTracker::FaceTracker(uchar* image_data, int width, int height, int channels, int stride, float min_confidence, UserFaceSettings face_settings)
{
	if (image_data == 0)
	{
		throw "image data is not correct";
	}
	if (width * channels != stride)
	{
		throw "image parameters are not correct";
	}
	read_from_data(this->current_img, image_data, width, height, channels, stride);
	cnf = min_confidence;
	this->user_settings = face_settings;
	must_be_transposed = true;
}
void FaceTracker::set_transposed(bool be_transposed)
{
	must_be_transposed = be_transposed;
	if (be_transposed)
	{
		int tmp = frame_height;
		frame_height = frame_width;
		frame_width = tmp;
	}
}
void FaceTracker::set_extraction_mode(bool have_to_feature_extraction_)
{
	have_to_feature_extraction = have_to_feature_extraction_;
}
void FaceTracker::set_gpu_id(int device_id)
{
	gpu_id = device_id;
}
int FaceTracker::create_face_extractor(int which)
{
	int ret_val = 0;
	if (face_extraction_folder.size() != 0 )
	{
		switch (which)
		{
		case 0:
			face_extractor = CreateClassifier((char*)(face_extraction_folder + "\\senet50_128.prototxt").c_str(),
			(char*)(face_extraction_folder + "\\senet50_128.caffemodel").c_str(), "feat_extract", gpu_id);
			FACE_FEATURE_SIZE = 128;
			break;
		case 1:
		face_extractor = CreateClassifier((char*)(face_extraction_folder + "\\senet50_256.prototxt").c_str(),
			(char*)(face_extraction_folder + "\\senet50_256.caffemodel").c_str(), "feat_extract", gpu_id);
		FACE_FEATURE_SIZE = 256;
			break;
		case 2:
			face_extractor = CreateClassifier((char*)(face_extraction_folder + "\\senet50_ft.prototxt").c_str(),
			(char*)(face_extraction_folder + "\\senet50_ft.caffemodel").c_str(), "pool5/7x7_s1", gpu_id);
			FACE_FEATURE_SIZE = 2048;
			break; 
		default:
			throw ("Incorrect Model Name");
				break;
		}
		/**/
		/**/
	}
	else
	{
		ret_val = -1;
	}
	return ret_val;
}
int FaceTracker::create_face_detector()
{
	int ret_val = 0;
	if (face_detection_folder.size() != 0 )
	{
		face_detector = CreateFaceDetector((char*)face_detection_folder.c_str(), gpu_id);
	}
	else
	{
		ret_val = -1;
	}
	return ret_val;
}
int FaceTracker::set_repository_path(string pre_path)
{
	repository_path = pre_path;
	
	int ret_val = _mkdir(repository_path.c_str());
	
	if (!ret_val || errno == EEXIST)
	{
		is_set_repository_path = true;
	}
	return ret_val;
}
void FaceTracker::DestroyAll()
{
	FreeClassifier(face_extractor);
	FreeFaceDetector(face_detector);
}
int FaceTracker::set_cam_name(string cam_path, bool be_erase)
{
	logger("logger__.txt", "there 0.0");
	if (!is_set_repository_path)
		return -1;
	logger("logger__.txt", "there 0.1");
	cam_name = cam_path;
	int ret_val = 0;
	logger("logger__.txt", "there 1");
	if (be_erase)
	{
		
		char curr_dir[MAX_PATH];
		ret_val = _rmdir((repository_path + "\\" + cam_name).c_str());
		if (ret_val != 0)
		{
			SHELLEXECUTEINFOA ExecuteInfo;

			memset(&ExecuteInfo, 0, sizeof(ExecuteInfo));

			ExecuteInfo.cbSize = sizeof(ExecuteInfo);
			ExecuteInfo.fMask = 0;
			ExecuteInfo.hwnd = 0;
			ExecuteInfo.lpVerb = "open";                
			ExecuteInfo.lpFile = "cmd.exe";  
			ExecuteInfo.lpParameters = ("/c rd /s /q " + repository_path + "\\" + cam_name).c_str(); 
			ExecuteInfo.lpDirectory = 0;     
			ExecuteInfo.nShow = SW_HIDE;
			ExecuteInfo.hInstApp = 0;

			ret_val = ShellExecuteExA(&ExecuteInfo);
			if (ret_val != 1)
			{
				cout << std::strerror(errno);
			}
		}
	}
	logger("logger__.txt", "there 2");
	logger("logger__.txt", "path is : %s", (repository_path + "\\" + cam_name + "\\").c_str());
	ret_val = _mkdir((repository_path + "\\" + cam_name + "\\").c_str());
	logger("logger__.txt", "retval is : %d\t lasterr = %d", ret_val, GetLastError());

	if (!ret_val || errno == EEXIST)
	{
		frames_path = repository_path + "\\" + cam_name + "\\frames";
		ret_val = _mkdir(frames_path.c_str()) * 1000;
		frames_path += "\\frame_";
		prifix_path = repository_path + "\\" + cam_name + "\\";
	}
	else
	{
		ret_val *= 10;
	}
	return ret_val;
}
int FaceTracker::extract_faces()
{
	faces = new DetectedFace*[1];
	faces_count = 0;
	double *outputs;
	DetectFaces_Mat(face_detector, current_img, &outputs, &faces_count, 0, user_settings, faces);
	return faces_count;
}
int FaceTracker::set_roi(cv::Rect roi_)
{
	int ret_val = 0;
	if (cap_type != IDS && !cam_cap.isOpened())
	{
		ret_val = -1;
	}
	else 
	{
		roi_.x = roi_.x < 0 ? 0 : roi_.x;
		roi_.y = roi_.y < 0 ? 0 : roi_.y;
		roi_.height = roi_.height <= 0 ? frame_height : roi_.height;
		roi_.width = roi_.width <= 0 ? frame_width : roi_.width;
		roi_.width = roi_.x + roi_.width > frame_width ? frame_width  - roi_.x: roi_.width;
		roi_.height = roi_.y + roi_.height > frame_height ? frame_height - roi_.y : roi_.height;
		roi = roi_;
	}
	return ret_val;
}
int FaceTracker::set_ids(IDSCamera * cap)
{
	int ret_val = 0;
	ids_camera = cap;
	if (ids_camera)
	{ 
		frame_height = ids_camera->m_nSizeY;
		frame_width = ids_camera->m_nSizeX;
		frame_rate = 30;
		cap_type = IDS;
	}
	return ret_val;
} 
int FaceTracker::set_capture(cv::VideoCapture cap)
{
	int ret_val = 0;
	cam_cap = cap;
	if (!cam_cap.isOpened())
	{
		ret_val = -1;
	}
	else
	{ 
		cap_type = USB;
		frame_height = cam_cap.get(cv::CAP_PROP_FRAME_HEIGHT);
		frame_width = cam_cap.get(cv::CAP_PROP_FRAME_WIDTH);
		frame_rate = cam_cap.get(cv::CAP_PROP_FPS);
	}
	return ret_val;
}
int FaceTracker::set_capture(std::string path)
{
	int ret_val = 0;
	cam_cap = cv::VideoCapture(path.c_str(), cv::CAP_FFMPEG);
	cam_cap.set(cv::CAP_PROP_FPS, 15);
	double h = cam_cap.get(cv::CAP_PROP_FRAME_HEIGHT);
	// Exit if video is not opened
	if (!cam_cap.isOpened())
	{
		ret_val = -1;
	}
	else
	{ 
		cap_type = IP_URL;
		frame_height = cam_cap.get(cv::CAP_PROP_FRAME_HEIGHT);
		frame_width = cam_cap.get(cv::CAP_PROP_FRAME_WIDTH);
		frame_rate = cam_cap.get(cv::CAP_PROP_FPS);
	}
	return ret_val;
}
int FaceTracker::reset_all()
{
	int ret_val = 0;
	try
	{
		logger("logger.txt", "\nResetAll\n");
		if(cam_cap.isOpened())
			frame_counter = cam_cap.get(cv::CAP_PROP_POS_FRAMES);
		
		faces_count = 0;
		multi_face_tracker.reset_all();
	}
	catch (const std::exception&)
	{
		ret_val = -3;
	}
	catch (cv::Exception &ex)
	{
		ret_val = -4;
	}
	logger("logger.txt", "\nDone\n");
	return ret_val;
}

void worker_image_saver(cv::Mat img, cv::String path)
{
	cv::Mat resized_frame;
	vector<int>jpg_low_params;
	jpg_low_params.push_back(10);
	cv::resize(img, resized_frame, cv::Size(300, 300));
	//logger("image_Logger.txt", "worker_image_saver...1 %s", path.c_str());
	cv::imwrite(path, resized_frame, jpg_low_params);
	//logger("image_Logger.txt", "worker_image_saver...2 %s", path.c_str());
}
int FaceTracker::get_next_frame(string recognized_list, bool is_registeration_mode, int new_width, int new_height, int stride, uchar* buffer, FaceObj** out_faces, int *out_faces_count)
{
	//logger("image_Logger.txt", "get_next_frame...1");
	int ret_val = 0;
	cv::Mat frame;
	bool ok = true;
	const float MIN_TRACKING_INTERSECTION_RATIO = 0.4;
	char str_tmp[_MAX_PATH];
	unsigned char * str_uuid = nullptr;
	cv::Mat faces_img;
	cv::Mat frame_mask;
	switch (cap_type)
	{
	case IDS:
	{
		int index = 0;
		frame = cv::Mat (ids_camera->m_nSizeY, ids_camera->m_nSizeX, CV_8UC4, ids_camera->m_vecMemory[index].pcImageMemory);
		cv::cvtColor(frame, frame, CV_BGRA2BGR); 
		break;
	}
	case USB:
		cam_cap.grab();
		ret_val = !cam_cap.retrieve(frame); 
		break;
	case VIDEO_File:
		cam_cap.grab();
		ret_val = !cam_cap.retrieve(frame);
		while(frame_counter % Frame_Steps != 0)
		{
			cam_cap.grab();
			ret_val = !cam_cap.retrieve(frame);
			frame_counter++;
		}
		break; 
	case IP_URL:
		
		mutex_obj.lock();
		frame = frame_grabber;
		mutex_obj.unlock();
		break; 
	}
	if (ret_val == 1 || frame.data == 0 || frame.cols == 0)
	{
		return  1;
	}
	if (frame.cols > MAX_FRAME_WIDTH || frame.rows > MAX_FRAME_HEIGHT)
	{
		cv::resize(frame, frame, cv::Size(MAX_FRAME_WIDTH, MAX_FRAME_HEIGHT),0.0,.0, cv::INTER_AREA);
	}
	//cv::flip(frame, frame, 1);
	if (must_be_transposed)
	{
		faces_img = cv::Mat(frame.t()).clone();
	}
	else
	{
		faces_img = frame.clone();
	}
	multi_face_tracker.set_max_queue_size(TRACK_QUEUE_SIZE);
	multi_face_tracker.set_faces_img(faces_img);
	multi_face_tracker.prifix_path = prifix_path;
	//logger("image_Logger.txt", "get_next_frame...2");
	if (roi.width > 0)
	{
		frame_mask = cv::Mat(frame.rows, frame.cols, frame.type());
		frame_mask = 0;
		frame_mask(roi) = cv::Scalar(255, 255, 255);
	}
	vector<cv::Scalar> colors;
	vector<cv::Rect2d>detected_faces;
	vector<cv::Rect2d>new_faces;
	FaceStream face_stream;
	cv::Mat resized_frame;
	int frames_per_seconds_to_save = cvRound(float(frame_rate) / SAVE_FRAME_RATE);
	//logger("image_Logger.txt", "get_next_frame...2.0, %d, %d", frame_rate, SAVE_FRAME_RATE);
	if (frame.data && frame.cols > 0)
	{
		if (must_be_transposed)
		{
			frame = frame.t();
		}

		write_to_data(frame, buffer, new_width, new_height, stride / new_width, stride);

		time_t current_time = time(nullptr);
		get_time_string(current_time, str_tmp);
		if (Save_Frames)
		if (frame_counter % frames_per_seconds_to_save == 0)
		{
			try
			{
				worker_image_saver( frame.clone(), (frames_path + str_tmp + ".jpg").c_str());
				/*arr_threads_to_save_images.push_back(thread(worker_image_saver, frame.clone(), (frames_path + str_tmp + ".jpg").c_str()));
				if (arr_threads_to_save_images.size() > 1000)
				{
					arr_threads_to_save_images.erase(arr_threads_to_save_images.begin());
				}*/
			}
			catch(std::exception &ex)
			{
				//logger("exception_Logger.txt", "%s", ex.what());
			}
			catch (cv::Exception &ex)
			{
				//logger("exception_Logger.txt", "%s", ex.what());
			}
			catch (...)
			{
				//logger("exception_Logger.txt", "error");
			}
		}


		frame_counter++;
		if (cap_type == VIDEO_File )
		{
			cam_cap.set(cv::CAP_PROP_POS_MSEC, (float(frame_counter) / PROCESS_FRAME_RATE)* CLOCKS_PER_SEC);
		}
#ifdef _EXE
		cam_cap.set(cv::CAP_PROP_POS_MSEC, (float(frame_counter) / PROCESS_FRAME_RATE)* CLOCKS_PER_SEC);
#endif

		if (roi.width > 0)
		{
			frame = frame_mask & frame;
		}
		/*cv::imshow("Tracking", frame);

		// Exit if ESC pressed.
		int k = cv::waitKey();*/
		//logger("image_Logger.txt", "get_next_frame...3");
		set_image(frame);
		extract_faces();
		//logger("image_Logger.txt", "get_next_frame...4");
		if (!is_registeration_mode)
		{
			multi_face_tracker.set_image(frame);
			multi_face_tracker.set_faces(faces, faces_count);
			multi_face_tracker.update(current_time, jpg_high_params, frame_counter);

			//logger("image_Logger.txt", "get_next_frame...5");
			int new_faces_count = multi_face_tracker.new_faces.size();
			for (int i = 0; i < new_faces_count; i++)
			{

				multi_face_tracker.add_new_face(i);
				FaceStream new_face_stream;
				FaceCase new_face_case;
				new_face_case.index = frame_counter;
				new_face_case.bounding_rect = multi_face_tracker.new_faces[i];
				refine_rect(new_face_case.bounding_rect);
				GUID guid;
				CoCreateGuid(&guid);
				
				UuidToStringA(&guid, (RPC_CSTR*)&str_uuid);
				new_face_case.id = (LPSTR)str_uuid;
				RpcStringFreeA(&str_uuid);
				get_time_string(time(nullptr), str_tmp);
				new_face_case.face_img_path = prifix_path + new_face_case.id + "\\face_" + str_tmp + ".jpg";
				if (Save_Faces)
				{
					_mkdir((prifix_path + new_face_case.id).c_str());
					cv::imwrite(new_face_case.face_img_path, frame(new_face_case.bounding_rect), jpg_high_params);
				}

				new_face_case.color = cv::Scalar(rand() % 255, rand() % 255, rand() % 255);
				new_face_stream.push_back(new_face_case);
				multi_face_tracker.tracked_faces.push_back(new_face_stream);
			}
			faces_count = multi_face_tracker.tracked_faces.size();
			int final_faces_count = 0;
			for (int i = 0; i < faces_count; i++)
			{
				if (multi_face_tracker.tracked_faces[i].size() >= MIN_FACE_COUNT_TO_DECLARE)
				{
					final_faces_count++;
				}
			}
			//logger("image_Logger.txt", "get_next_frame...6=>\t%d", final_faces_count);
			if (final_faces_count > 0)
			{
				*out_faces = new FaceObj[final_faces_count];
				*out_faces_count = final_faces_count;
				bool must_do_extraction_by_gpu = false;
				vector<cv::Mat> images_vec;
				vector<int>indexes;
				vector<float*> features_vec;
				int face_counter = 0;
				for (int i = 0; i < faces_count; i++)
				{
					if (multi_face_tracker.tracked_faces[i].size() < MIN_FACE_COUNT_TO_DECLARE)
					{
						continue;
					}
					FaceObj face_obj;
					FaceCase face_case = multi_face_tracker.tracked_faces[i][multi_face_tracker.tracked_faces[i].size() - 1];
					strcpy(face_obj.face_id, face_case.id.c_str());
					face_obj.face_rect.x = face_case.bounding_rect.x;
					face_obj.face_rect.y = face_case.bounding_rect.y;
					face_obj.face_rect.width = face_case.bounding_rect.width;
					face_obj.face_rect.height = face_case.bounding_rect.height;
					face_obj.tag = 0;
					face_obj.is_lost = face_case.is_lost;

					if (have_to_feature_extraction&& (frame_counter % Frame_Steps == 0) && (recognized_list.find(face_obj.face_id) == -1) && (face_case.bounding_rect.width >= MIN_FACE_SIZE_TO_RECOGNITION || face_case.bounding_rect.height >= MIN_FACE_SIZE_TO_RECOGNITION))
					{
						face_obj.features = new float[FACE_FEATURE_SIZE];
						if (gpu_id < 0)
						{
							ExtractFeature_Mat(face_extractor, frame(face_case.bounding_rect).clone(), face_obj.features);
						}
						else
						{
							must_do_extraction_by_gpu = true;
							features_vec.push_back(face_obj.features);
							indexes.push_back(i);
							images_vec.push_back(frame(face_case.bounding_rect).clone());
						}
					}
					else
					{
						face_obj.features = 0;
					}
					(*out_faces)[face_counter++] = face_obj;
				}
				if (must_do_extraction_by_gpu)
				{
					ExtractFeature_Flip_Batch_Mat(face_extractor, images_vec, features_vec);
				}
			}
			else
			{

				*out_faces = new FaceObj[final_faces_count];
				*out_faces_count = final_faces_count;
			}
		}
		else
		{
			
			*out_faces = new FaceObj[faces_count];
			*out_faces_count = faces_count;
			for (int i = 0; i < faces_count; i++)
			{
				FaceObj face_obj;
				strcpy(face_obj.face_id, "0000000000");
				face_obj.face_rect.x = (*faces)[i].x; face_obj.face_rect.y = (*faces)[i].y;
				face_obj.face_rect.height = (*faces)[i].height; face_obj.face_rect.width = (*faces)[i].width;
				/*if (face_obj.face_rect.height >= MAX_FACE_SIZE_TO_DETECTION || face_obj.face_rect.width >= MAX_FACE_SIZE_TO_DETECTION)
				{
					continue;
				}*/
				refine_rect(face_obj.face_rect);
				face_obj.features = 0;
				face_obj.tag = 0;
				face_obj.is_lost = false;
				(*out_faces)[i] = face_obj;
			}

		}
		if (faces && *faces)
		{
			delete[](*faces);
			delete faces;
		}
#ifdef DISPLAY_FRAMES
		for (int i = 0; i < faces_count; i++)
		{

			FaceCase face_case = multi_face_tracker.tracked_faces[i][multi_face_tracker.tracked_faces[i].size() - 1];

			cv::rectangle(frame, face_case.bounding_rect, face_case.color, 2);

		}

		// Display frame.
		cv::imshow("Tracking", frame);

		// Exit if ESC pressed.
		int k = cv::waitKey(1);
		if (k == 27)
		{
			return ret_val;
		}
#endif
	}
	else
	{
		ret_val = -100;
	}
	return ret_val;
}
int FaceTracker::refine_rect(cv::Rect &rect)
{
	int ret_val = 0;
	rect.x = rect.x < 0 ? 0 : rect.x;
	rect.y = rect.y < 0 ? 0 : rect.y;
	rect.width = rect.x + rect.width > frame_width ? frame_width - rect.x : rect.width;
	rect.height = rect.y + rect.height > frame_height ? frame_height - rect.y : rect.height;
	return ret_val;
}
int FaceTracker::refine_rect(FaceRect &rect)
{
	int ret_val = 0;
	rect.x = rect.x < 0 ? 0 : rect.x;
	rect.y = rect.y < 0 ? 0 : rect.y;
	rect.width = rect.x + rect.width > frame_width ? frame_width - rect.x : rect.width;
	rect.height = rect.y + rect.height > frame_height ? frame_height - rect.y : rect.height;
	return ret_val;
}
int FaceTracker::track()
{
	int ret_val = 0;
	// Read first frame 
	cv::Mat frame;
	bool ok = true;
	const float MIN_TRACKING_INTERSECTION_RATIO = 0.4;
	cv::namedWindow("Faces", cv::WINDOW_KEEPRATIO);
	cv::namedWindow("Tracking", cv::WINDOW_KEEPRATIO);
	cv::resizeWindow("Faces", 700, 800 );
	cv::resizeWindow("Tracking", 700, 800);
	cv::moveWindow("Faces", 50, 0);
	cv::moveWindow("Tracking", 750, 0);
	char str_tmp[_MAX_PATH];
	unsigned char * str_uuid = nullptr;
	cv::Mat faces_img;
	cv::Mat frame_mask;
	cam_cap.read(frame);
	if (must_be_transposed)
	{
		faces_img = cv::Mat(frame.t()).clone();
	}
	else
	{
		faces_img = frame.clone();
	}
	multi_face_tracker.set_max_queue_size(35);
	multi_face_tracker.set_faces_img(faces_img);
	multi_face_tracker.prifix_path = prifix_path;
	
	if (roi.width > 0)
	{
		frame_mask = cv::Mat(frame.rows, frame.cols, frame.type());
		frame_mask = 0;
		frame_mask(roi) = cv::Scalar(255,255,255);
	}
	vector<cv::Scalar> colors;
	vector<cv::Rect2d>detected_faces;
	vector<cv::Rect2d>new_faces;
	FaceStream face_stream;
	cv::Mat resized_frame;
	
	int frames_per_seconds_to_save = cvRound(frame_rate / SAVE_FRAME_RATE);
	//cv::Ptr<cv::ORB> orb = cv::ORB::create(20, 1.2, 8, 31, 0, 2, 0, 15, 20);
	FaceObj* out_faces;
	int out_faces_count;
	while (!get_next_frame("", false, frame.cols, frame.rows, frame.cols * frame.channels(), faces_img.data, &out_faces, &out_faces_count));
	return ret_val;
}
int FaceTracker::track0()
{
	int ret_val = 0;
	// Read first frame 
	cv::Mat frame;
	bool ok = true;
	const float MIN_TRACKING_INTERSECTION_RATIO = 0.4;
	cv::namedWindow("Faces", cv::WINDOW_KEEPRATIO);
	cv::namedWindow("Tracking", cv::WINDOW_KEEPRATIO);
	cv::resizeWindow("Faces", 700, 800 );
	cv::resizeWindow("Tracking", 700, 800);
	cv::moveWindow("Faces", 50, 0);
	cv::moveWindow("Tracking", 750, 0);
	char str_tmp[_MAX_PATH];
	unsigned char * str_uuid = nullptr;
	cv::Mat faces_img;
	cv::Mat frame_mask;
	cam_cap.read(frame);
	if (must_be_transposed)
	{
		faces_img = cv::Mat(frame.t()).clone();
	}
	else
	{
		faces_img = frame.clone();
	}
	multi_face_tracker.set_max_queue_size(35);
	multi_face_tracker.set_faces_img(faces_img);
	multi_face_tracker.prifix_path = prifix_path;
	
	if (roi.width > 0)
	{
		frame_mask = cv::Mat(frame.rows, frame.cols, frame.type());
		frame_mask = 0;
		frame_mask(roi) = cv::Scalar(255,255,255);
	}
	vector<cv::Scalar> colors;
	vector<cv::Rect2d>detected_faces;
	vector<cv::Rect2d>new_faces;
	FaceStream face_stream;
	cv::Mat resized_frame;
	
	int frames_per_seconds_to_save = cvRound(frame_rate / SAVE_FRAME_RATE);
	//cv::Ptr<cv::ORB> orb = cv::ORB::create(20, 1.2, 8, 31, 0, 2, 0, 15, 20);

	while (cam_cap.read(frame))
	{
		if (must_be_transposed)
		{
			frame = frame.t();
		}

		time_t current_time = time(nullptr);
		get_time_string(current_time, str_tmp);
#ifdef SAVE_FRAMES
		if (frame_counter % frames_per_seconds_to_save == 0)
		{
			cv::resize(frame, resized_frame, cv::Size(300, 300));
			cv::imwrite((frames_path + str_tmp + ".jpg").c_str(), resized_frame, jpg_low_params);
		}
#endif
		
		frame_counter++;
		cam_cap.set(cv::CAP_PROP_POS_MSEC, (float(frame_counter)  / PROCESS_FRAME_RATE)* CLOCKS_PER_SEC);

		if (roi.width > 0)
		{
			frame = frame_mask & frame;
		}
		set_image(frame);
		extract_faces();
		multi_face_tracker.set_image(frame);
		multi_face_tracker.set_faces(faces, faces_count);
		multi_face_tracker.update(current_time, jpg_high_params, frame_counter);
		

		int new_faces_count = multi_face_tracker.new_faces.size();
		for (int i = 0; i < new_faces_count; i++)
		{
			
			multi_face_tracker.add_new_face(i);
			FaceStream new_face_stream;
			FaceCase new_face_case;
			new_face_case.index = frame_counter;
			new_face_case.bounding_rect = multi_face_tracker.new_faces[i];
			refine_rect(new_face_case.bounding_rect);
			GUID guid;
			CoCreateGuid(&guid);

			UuidToStringA(&guid, (RPC_CSTR*)&str_uuid);
			new_face_case.id = (LPSTR)str_uuid;
			RpcStringFreeA(&str_uuid);
			_mkdir((prifix_path + new_face_case.id).c_str());
			get_time_string(time(nullptr), str_tmp);
			new_face_case.face_img_path = prifix_path + new_face_case.id +"\\face_" + str_tmp + ".jpg";
#ifdef SAVE_FACES			
			cv::imwrite(new_face_case.face_img_path, frame(new_face_case.bounding_rect), jpg_high_params);
#endif			
			
			new_face_case.color = cv::Scalar(rand() % 255, rand() % 255, rand() % 255);
			new_face_stream.push_back(new_face_case);
			multi_face_tracker.tracked_faces.push_back(new_face_stream);
		}
		faces_count = multi_face_tracker.tracked_faces.size();
		for (int i = 0; i < faces_count; i++)
		{
			
			FaceCase face_case = multi_face_tracker.tracked_faces[i][multi_face_tracker.tracked_faces[i].size() - 1];
#ifdef DISPLAY_FRAMES
			cv::rectangle(frame, face_case.bounding_rect, face_case.color, 2);
#endif
		}
		// Display frame.
		cv::imshow("Tracking", frame);

		// Exit if ESC pressed.
		int k = cv::waitKey(1);
		if (k == 27)
		{
			break;
		}

	}
	return ret_val;
}
int FaceTracker::createTracker(string trackerType )
{
	int ret_val = 0;
	tracker_type = trackerType;
	
	return ret_val;
}
void FaceTracker::set_image(uchar* image_data, int width, int height, int channels, int stride)
{
	if (image_data == 0)
	{
		throw "image data is not correct";
	}
	if (width * channels != stride)
	{
		throw "image parameters are not correct";
	}
	read_from_data(this->current_img, image_data, width, height, channels, stride);
}
void FaceTracker::set_image(cv::Mat &img)
{
	current_img = img;
}
void FaceTracker::set_min_confidence(float min_confidence)
{
	cnf = min_confidence;
}
void FaceTracker::set_user_settings(UserFaceSettings face_settings)
{
	user_settings = face_settings;
}


