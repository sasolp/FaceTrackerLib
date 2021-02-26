/// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include "MultiFaceTracker.h"
#include "FaceTracker.h"


extern bool Save_Frames = false;
extern bool Save_Faces = false;
extern int WAIT_DELAY = 600;
extern int MAX_TRACKABLE_COUNT = 200;
extern int SAVE_FRAME_RATE = 1;
extern int PROCESS_FRAME_RATE = 10;
extern int MIN_FACE_SIZE_TO_RECOGNITION = 75;
extern int MAX_FACE_SIZE_TO_DETECTION = 800;
extern int MAX_FRAME_HEIGHT = 1080;
extern int MAX_FRAME_WIDTH = 1920;
extern float MAX_MATCHED_FACES_DIST = 0.15;
extern int MIN_FACE_COUNT_TO_DECLARE = 2;
extern int TRACK_QUEUE_SIZE = 18;
extern int Frame_Steps = 1;
extern void get_time_string(time_t tr, char *str_time);
void GetTimeInLongFormat(char** out_time)
{
	char str_tmp[_MAX_PATH];
	time_t current_time = time(nullptr);
	get_time_string(current_time, str_tmp);
	strcpy(*out_time, str_tmp);
}
void write_to_data(cv::Mat &src, uchar* image_data, int width, int height, int channels, int stride)
{

	if (image_data != 0 && width * channels <= stride && channels > 1)
	{
		uchar* src_data = src.data;
		for (int i = 0; i < height; i++)
		{
			memcpy(image_data, src_data, width * channels);
			image_data += stride;
			src_data += width * channels;
		}
	}
}

void worker_frame_grabber(cv::VideoCapture *cap, cv::Mat &frame_grabber)
{
	bool be_continue = true;
	while (be_continue)
	{
		cap->grab();

		be_continue = cap->retrieve(frame_grabber);
	}
}
int SetParams(
	bool xSaveFrames = false,
	bool xSaveFaces = false,
	int xSAVE_FRAME_RATE = 1,
	int xPROCESS_FRAME_RATE = 10,
	int xMIN_FACE_SIZE_TO_RECOGNITION = 75,
	float xMAX_MATCHED_FACES_DIST = 0.85f,
	int xDisplayFrameRate = 10,
	int xMAX_FACE_SIZE_TO_DETECTION = 800,
	int xMAX_FRAME_HEIGHT = 1080,
	int xMAX_FRAME_WIDTH = 1920,
	int xMIN_FACE_COUNT_TO_DECLARE = 2,
	int xWAIT_DELAY = 600,
	int xMAX_TRACKABLE_COUNT = 200,
	int xTRACK_QUEUE_SIZE = 18)
{
	int ret_val = 0;

	Save_Frames = xSaveFrames;
	Save_Faces = xSaveFaces;
	WAIT_DELAY = xWAIT_DELAY;
	MAX_TRACKABLE_COUNT = xMAX_TRACKABLE_COUNT;
	SAVE_FRAME_RATE = xSAVE_FRAME_RATE;
	PROCESS_FRAME_RATE = xPROCESS_FRAME_RATE;
	MIN_FACE_SIZE_TO_RECOGNITION = xMIN_FACE_SIZE_TO_RECOGNITION;
	MAX_FACE_SIZE_TO_DETECTION = xMAX_FACE_SIZE_TO_DETECTION;
	MAX_FRAME_HEIGHT = xMAX_FRAME_HEIGHT;
	MAX_FRAME_WIDTH = xMAX_FRAME_WIDTH;
	MIN_FACE_COUNT_TO_DECLARE = xMIN_FACE_COUNT_TO_DECLARE;
	MAX_MATCHED_FACES_DIST = (1 - xMAX_MATCHED_FACES_DIST);
	TRACK_QUEUE_SIZE = xTRACK_QUEUE_SIZE;
	Frame_Steps = xDisplayFrameRate / PROCESS_FRAME_RATE;
	if (!Frame_Steps)
	{
		Frame_Steps = 1;
	}
	return ret_val;

}
int GetUSBCamCount()
{
	int counter = 0;
	while (counter < 100)
	{
		try
		{
			cv::VideoCapture cap = cv::VideoCapture(counter);
			if (cap.isOpened())
			{
				cap.release();
				cap.~VideoCapture();
				counter++;
			}
			else
			{
				break;
			}
		}
		catch (const std::exception&)
		{
			counter++;
		}
	}
	return counter + 1;
}
int ReleaseCaptureCam(cv::VideoCapture* cap, int type = 0)
{
	int ret_val = 0;
	if (cap != 0)
	{
		try
		{
			switch (type)
			{
			case IDS:
			{
				IDSCamera *ptr_ids_cam = (IDSCamera *)cap;
				ptr_ids_cam->Stop();
				ptr_ids_cam->Exit();
				delete ptr_ids_cam;
				break;
			}
			case USB:
			case VIDEO_File:
			case IP_URL:
				cap->release();
				cap->~VideoCapture();
				delete cap;
				break;
			}
		}
		catch (cv::Exception &ex)
		{
			ret_val = -20;
		}

	}
	return ret_val;
}

int CreateCapture(FaceTracker *faceTracker, cv::VideoCapture** cap, char* str, int* frame_width, int* frame_height, double* frame_rate, int* frames_count, int cam_id = -1, int type = 0)
{
	int ret_val = 0;
	try
	{
		faceTracker->frame_counter = 0;

		if (faceTracker->grabber_thread.native_handle() != 0)
		{
			TerminateThread(faceTracker->grabber_thread.native_handle(), 0);
			faceTracker->grabber_thread.detach();
			faceTracker->grabber_thread.~thread();
			;

		}
		switch (type)
		{
		case IDS:
		{
			IDSCamera *ptr_ids_cam = new IDSCamera();
			cam_id = 50;
			if (ptr_ids_cam->Init(cam_id) == 0)
			{

				/*ptr_ids_cam->set_prop(3, 640);
				ptr_ids_cam->set_prop(4, 480);
				ptr_ids_cam->m_nSizeY = 640;
				ptr_ids_cam->m_nSizeY = 480;*/
				ptr_ids_cam->Start(cam_id);

				*cap = (cv::VideoCapture*)ptr_ids_cam;
				*frame_height = min(ptr_ids_cam->m_nSizeY, MAX_FRAME_HEIGHT);
				*frame_width = min(ptr_ids_cam->m_nSizeX, MAX_FRAME_WIDTH);
				*frame_rate = 30;
				*frames_count = -1;
			}
			else
			{
				ret_val = -2;
			}
			logger("logger.txt", "\nretVal = %d", ret_val);
			break;
		}
		case USB:
		{
			if (cam_id >= 0)
			{

				logger("logger.txt", "\nRead From Cam Device %d\n", cam_id);
				*cap = new cv::VideoCapture(cam_id);
			}
			else
			{
				*cap = 0;
				ret_val = -1;
			}

			logger("logger.txt", "\nCheck Cam Device : if ((*cap) && !(*cap)->isOpened()) => (*cap)-> %d\n", (int)(*cap));
			if ((*cap) && !(*cap)->isOpened())
			{
				ret_val = -2;
			}
			else
			{

				*frame_height = (*cap)->get(cv::CAP_PROP_FRAME_HEIGHT);
				*frame_width = (*cap)->get(cv::CAP_PROP_FRAME_WIDTH);
				*frame_width = min(*frame_width, MAX_FRAME_WIDTH);
				*frame_height = min(*frame_height, MAX_FRAME_HEIGHT);
				*frame_rate = (*cap)->get(cv::CAP_PROP_FPS);
				*frames_count = (*cap)->get(cv::CAP_PROP_FRAME_COUNT);
			}
			logger("logger.txt", "\nretVal = %d", ret_val);
			break;
		}
		case VIDEO_File:
		{
			faceTracker->cam_path = str;
			string path(str);
			if (path != "")
			{
				*cap = new cv::VideoCapture(path, cv::CAP_FFMPEG);
			}
			logger("logger.txt", "\nRead From Cam Device %s\n", str);
			logger("logger.txt", "\nCheck Cam Device : if ((*cap) && !(*cap)->isOpened()) => (*cap)-> %d\n", (int)(*cap));
			if ((*cap) && !(*cap)->isOpened())
			{
				ret_val = -2;
			}
			else
			{

				*frame_height = (*cap)->get(cv::CAP_PROP_FRAME_HEIGHT);
				*frame_width = (*cap)->get(cv::CAP_PROP_FRAME_WIDTH);
				*frame_width = min(*frame_width, MAX_FRAME_WIDTH);
				*frame_height = min(*frame_height, MAX_FRAME_HEIGHT);
				*frame_rate = (*cap)->get(cv::CAP_PROP_FPS);

				*frames_count = (*cap)->get(cv::CAP_PROP_FRAME_COUNT);
				faceTracker->set_capture(**cap);
			}
			logger("logger.txt", "\nretVal = %d", ret_val);
			break;
		}
		case IP_URL:
		{
			faceTracker->cam_path = str;
			//string path("rtsp://admin:Aa123456@192.168.2.64/mpeg4/ch00/sub/av_stream");

			string path(str);
			if (path != "")
			{
				*cap = new cv::VideoCapture(path, cv::CAP_FFMPEG);
			}
			logger("logger.txt", "\nRead From Cam Device %s\n", str);
			logger("logger.txt", "\nCheck Cam Device : if ((*cap) && !(*cap)->isOpened()) => (*cap)-> %d\n", (int)(*cap));
			if ((*cap) && !(*cap)->isOpened())
			{
				ret_val = -2;
			}
			else
			{

				*frame_height = (*cap)->get(cv::CAP_PROP_FRAME_HEIGHT);
				*frame_width = (*cap)->get(cv::CAP_PROP_FRAME_WIDTH);
				*frame_rate = (*cap)->get(cv::CAP_PROP_FPS);

				*frames_count = (*cap)->get(cv::CAP_PROP_FRAME_COUNT);
				faceTracker->set_capture(**cap);
				faceTracker->frame_grabber = cv::Mat(*frame_height, *frame_width, CV_8UC3);
				bool bb = (*cap)->set(cv::CAP_PROP_POS_FRAMES, 100);
				faceTracker->grabber_thread = thread(worker_frame_grabber, &(faceTracker->cam_cap), faceTracker->frame_grabber);

				*frame_width = min(*frame_width, MAX_FRAME_WIDTH);
				*frame_height = min(*frame_height, MAX_FRAME_HEIGHT);
			}
			logger("logger.txt", "\nretVal = %d", ret_val);
			break;
		}
		}
	}
	catch (cv::Exception &ex)
	{
		ret_val = -1;
	}
	return ret_val;
}
int SetCapProperty(cv::VideoCapture* cap, int prop_id, double value, int type = 0)
{
	int ret_val = 0;
	try
	{
		switch (type)
		{
		case USB:
			cap->set(prop_id, value);
			break;
		case IDS:
		{
			IDSCamera *ptr_ids_cam = (IDSCamera *)cap;
			ptr_ids_cam->set_prop(prop_id, value);
			break;
		}
		case IP_URL:
			cap->set(prop_id, value);
		case VIDEO_File:
			cap->set(prop_id, value);
			break;
		}
	}
	catch (cv::Exception &ex)
	{
		ret_val = -1;
	}
	return ret_val;
}
int GetCapProperty(cv::VideoCapture* cap, int prop_id, double *value, int type = 0)
{
	int ret_val = 0;
	try
	{
		switch (type)
		{
		case USB:
		{
			*value = cap->get(prop_id);
			break;
		}
		case IDS:
		{
			IDSCamera *ptr_ids_cam = (IDSCamera *)cap;
			switch (prop_id)
			{
			case cv::CAP_PROP_FRAME_WIDTH:
				*value = ptr_ids_cam->m_nSizeX;
				break;
			case cv::CAP_PROP_FRAME_HEIGHT:
				*value = ptr_ids_cam->m_nSizeY;
				break;
			default:
				break;
			}
			break;
		}
		case IP_URL:
		{
			*value = cap->get(prop_id);
			break;
		}
		case VIDEO_File:
		{
			*value = cap->get(prop_id);
			break;
		}
		}

	}
	catch (cv::Exception &ex)
	{
		ret_val = -1;
	}
	return ret_val;
}
int GetJpegFromRawData(uchar* buffer, int width, int height, int stride, FaceRect roi, uchar **out_buffer, int* out_len)
{
	int ret_val = 0;

	try
	{
		vector<int>jpg_high_params;
		jpg_high_params.push_back(100);
		cv::Mat img(height, width, CV_8UC3);
		read_from_data(img, buffer, width, height, stride / width, stride);
		cv::Rect _roi;
		_roi.x = roi.x;
		_roi.y = roi.y;
		_roi.width = roi.width;
		_roi.height = roi.height;
		img = img(_roi).clone();
		vector<uchar>buf;
		cv::imencode(".jpg", img, buf);
		out_len[0] = buf.size();
		*out_buffer = new uchar[out_len[0]];
		memcpy(*out_buffer, buf.data(), out_len[0]);
		/*ofstream of("d:\\1.jpg", ofstream::binary);
		of.write((const char*)(*out_buffer), out_len[0]);
		of.close();*/
	}
	catch (const std::exception&)
	{
		ret_val = -3;
	}
	catch (cv::Exception &ex)
	{
		ret_val = -4;
	}
	return ret_val;
}
int GetNextFrame(void * obj, cv::VideoCapture* cap, char* recognized_list, bool is_registeration_mode, int new_width, int new_height, int stride, uchar* buffer, FaceObj** out_faces, int *out_faces_count, bool is_transposed = true, int type = 0, bool have_to_feature_extraction = true)
{
	int ret_val = 0;
	if (cap)
	{
		try
		{
			FaceTracker *face_tracker = ((FaceTracker*)obj);
			switch (type)
			{
			case USB:
				face_tracker->set_capture(*cap);
				face_tracker->cap_type = USB;
				break;
			case IDS:
			{
				IDSCamera *ptr_ids_cam = (IDSCamera *)cap;
				face_tracker->set_ids(ptr_ids_cam);
				face_tracker->cap_type = IDS;
				break;
			}
			case VIDEO_File:
				face_tracker->set_capture(*cap);
				face_tracker->cap_type = VIDEO_File;
				break;
			case IP_URL:
				face_tracker->set_capture(*cap);
				face_tracker->cap_type = IP_URL;
				break;
			}
			face_tracker->set_extraction_mode(have_to_feature_extraction);
			//face_tracker.set_roi(cv::Rect(350, 0, 570, 0));
			face_tracker->createTracker("");
			face_tracker->set_transposed(is_transposed);
			face_tracker->get_next_frame(recognized_list, is_registeration_mode, new_width, new_height, stride, buffer, out_faces, out_faces_count);
		}
		catch (const std::exception&)
		{
			ret_val = -3;
		}
		catch (cv::Exception &ex)
		{
			ret_val = -4;
		}
	}
	else
	{
		ret_val = -2;
	}
	return ret_val;
}


int Initialize(void** obj, int which, int gpuid, char* base_dir, char* camrepositoryPath, char* cam_name)
{
	int ret_val = 0;
	logger("logger2.txt", "here 0");
	if (obj != 0)
	{
		try
		{
			logger("logger2.txt", "here 1");
			string face_detect_fld = "FaceDetectionModels";
			string face_extract_fld = "FaceFtrExtModels";
			string repository_path = "CamsRepository";
			UserFaceSettings face_settings;

			face_settings.pThresh = 0.6f;
			face_settings.rThresh = 0.6f;
			face_settings.nmsThresh = 0.8f;
			face_settings.sizeThresh = 50;
			face_settings.minConf = 0.1;
			face_settings.facialKeisDetection = 1;
			FaceTracker* face_tracker = new FaceTracker(base_dir + face_detect_fld, base_dir + face_extract_fld, 0.25, face_settings, -1);
			face_tracker->create_face_detector();
			face_tracker->set_gpu_id(gpuid);
			face_tracker->create_face_extractor(which);
			face_tracker->worker_thread_id = 0;

			face_tracker->set_repository_path(camrepositoryPath + repository_path);
			face_tracker->set_cam_name(cam_name, false);
			*obj = face_tracker;
			logger("logger2.txt", "here 2");
		}
		catch (cv::Exception &ex)
		{
			ret_val = -20;
		}

	}
	else
	{
		logger("logger2.txt", "here -1");
	}
	return ret_val;
}
int ResetAll(void* obj)
{
	FaceTracker *face_tracker = ((FaceTracker*)obj);
	int ret_val = face_tracker->reset_all();
	return ret_val;
}
int SetFaceTrackerRoi(void* obj, FaceRect roi)
{
	int ret_val = 0;
	if (obj != 0)
	{
		try
		{
			FaceTracker *face_tracker = ((FaceTracker*)obj);
			face_tracker->set_roi(cv::Rect(roi.x, roi.y, roi.width, roi.height));
		}
		catch (cv::Exception &ex)
		{
			ret_val = -20;
		}

	}
	return ret_val;
}
int GetFeatureSize(FaceTracker *tracker, int * feature_size)
{
	int ret_val = 0;
	*feature_size = tracker->FACE_FEATURE_SIZE;
	return ret_val;
}
int ResetFaceTrackerRoi(void* obj)
{
	int ret_val = 0;
	if (obj != 0)
	{
		try
		{
			FaceTracker *face_tracker = ((FaceTracker*)obj);
			face_tracker->set_roi(cv::Rect(-1, -1, -1, -1));
		}
		catch (cv::Exception &ex)
		{
			ret_val = -20;
		}

	}
	return ret_val;
}
int ReleaseFaces(FaceObj** faces)
{
	int ret_val = 0;
	if ((*faces) != 0)
	{
		try
		{
			delete[](*faces);
			(*faces) = 0;
		}
		catch (cv::Exception &ex)
		{
			ret_val = -20;
		}

	}
	return ret_val;
}
int ReleaseBuffer(void* buffer)
{
	int ret_val = 0;
	if (buffer != 0)
	{
		try
		{
			delete[]buffer;
			buffer = 0;
		}
		catch (cv::Exception &ex)
		{
			ret_val = -20;
		}

	}
	return ret_val;
}

void worker_face_grabber(int id)
{
	string path = "D:\\Sasol\\Projects\\dataset\\1.mp4";
	string face_detect_fld = "FaceDetectionModels";
	string face_extract_fld = "FaceFtrExtModels";
	/*if (argc <= 1)
	{
		path = "D:\\Sasol\\DataSets\\Face_Tracking\\face (1).mp4";
	}
	else
	{
		path = argv[1];
	}*/
	UserFaceSettings face_settings;
	/*
		face_settings.pThresh = 0.9f;
		face_settings.rThresh = 0.8f;
		face_settings.nmsThresh = 0.9f;
		face_settings.sizeThresh = 40;
		face_settings.minConf = 0.2;
		face_settings.facialKeisDetection = 1;*/

	face_settings.pThresh = 0.8f;
	face_settings.rThresh = 0.8f;
	face_settings.nmsThresh = 0.8f;
	face_settings.sizeThresh = 64;
	face_settings.minConf = 0.25;
	face_settings.facialKeisDetection = 1;
	FaceTracker face_tracker(face_detect_fld, face_extract_fld, 0.25, face_settings, -1);
	face_tracker.create_face_detector();
	face_tracker.create_face_extractor(1);
	face_tracker.worker_thread_id = id;
	face_tracker.set_capture(path);
	//face_tracker.set_roi(cv::Rect(350, 0, 570,0));
	face_tracker.set_repository_path("D:\\Sasol\\Projects\\dataset\\CamsRepository");
	face_tracker.set_cam_name("Video2", true);
	string trackerTypes[8] = { "BOOSTING", "MIL", "KCF", "TLD","MEDIANFLOW", "GOTURN", "MOSSE", "CSRT" };
	face_tracker.createTracker(trackerTypes[7]);
	face_tracker.set_transposed(false);
	//	SetCapProperty(&face_tracker.cam_cap, cv::CAP_PROP_POS_FRAMES, 100);
	face_tracker.frame_counter = 15;
	face_tracker.track();
	//cv::VideoCapture cap("rtsp://10.10.32.232:554/h264_2", cv::CAP_FFMPEG);

}
void main(int argc, char*argv[])
{
	thread th(worker_face_grabber, 1);
	th.join();
}

