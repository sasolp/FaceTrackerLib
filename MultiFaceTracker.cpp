//

#include "stdafx.h"
#include <ppl.h>
#include<direct.h>
#include "MultiFaceTracker.h"
#include <Windows.h>
#include<omp.h>
void worker_face_saver(cv::Mat img, cv::String path)
{
	cv::Mat resized_frame;
	vector<int>jpg_low_params;
	jpg_low_params.push_back(10);
	cv::resize(img, resized_frame, cv::Size(120, 120));
	cv::imwrite(path, resized_frame, jpg_low_params);
}
extern void get_time_string(time_t tr, char *str_time);
#define ShowFaces(prifix_str, offset)\
{\
	cv::namedWindow((string("Faces____") + prifix_str).c_str(), cv::WINDOW_KEEPRATIO);\
	cv::namedWindow((string("MatchedFace") + prifix_str).c_str(), cv::WINDOW_KEEPRATIO);\
	cv::namedWindow((string("MatchedFace2") + prifix_str).c_str(), cv::WINDOW_KEEPRATIO);\
	cv::resizeWindow((string("Faces____") + prifix_str).c_str(), 250, 250);\
	cv::resizeWindow((string("MatchedFace") + prifix_str).c_str(), 250, 250);\
	cv::resizeWindow((string("MatchedFace2") + prifix_str).c_str(), 250, 250);\
	cv::moveWindow((string("Faces____") + prifix_str).c_str(), 50 + offset, 50);\
	cv::moveWindow((string("MatchedFace") + prifix_str).c_str(), 350 + offset, 150);\
	cv::moveWindow((string("MatchedFace2") + prifix_str).c_str(), 50 + offset, 550);\
}
MultiFaceTracker::MultiFaceTracker()
{
	max_queue_size = 0;
}

void MultiFaceTracker::set_image(cv::Mat &img)
{
	frame = img;
	frame_height = img.rows;
	frame_width = img.cols;
}
void MultiFaceTracker::set_faces_img(cv::Mat &img)
{
	faces_img = img;
}
void MultiFaceTracker::set_max_queue_size(int max_queue_size_)
{
	max_queue_size = max_queue_size_;
}
void MultiFaceTracker::set_faces(DetectedFace**faces_, int faces_count_)
{
	faces = faces_;
	faces_count = faces_count_;
}
int MultiFaceTracker::create(int max_matching_point_count_, int norm_type)
{
	int ret_val = 0;
	max_matching_point_count = max_matching_point_count_;
	keypoint_detector = cv::GFTTDetector::create(max_matching_point_count, 0.03);
	if (keypoint_detector.empty())
	{
		ret_val = -1;
	}
	else
	{
		feature_ext = cv::AKAZE::create(cv::AKAZE::DESCRIPTOR_KAZE_UPRIGHT, 0, 3, 0.000051);
		if (feature_ext.empty())
		{
			ret_val = -2;
		}
		else
		{
			bf_matcher = cv::BFMatcher::create(norm_type, true);
			if (bf_matcher.empty())
			{
				ret_val = -3;
			}
		}
	}
	return ret_val;
}
int MultiFaceTracker::remove(int case_index)
{
	int ret_val = 0;

	prev_face_mats.erase(prev_face_mats.begin() + case_index);
	prev_face_descripts.erase(prev_face_descripts.begin() + case_index);
	tracked_faces.erase(tracked_faces.begin() + case_index);
	prev_losses_cunt.erase(prev_losses_cunt.begin() + case_index);
	return ret_val;
}
int MultiFaceTracker::remove(int case_index, int face_index)
{
	int ret_val = 0;

	prev_face_mats[case_index].erase(prev_face_mats[case_index].begin() + face_index);
	prev_face_descripts[case_index].erase(prev_face_descripts[case_index].begin() + face_index);
	tracked_faces[case_index].erase(tracked_faces[case_index].begin() + face_index);
	prev_losses_cunt.erase(prev_losses_cunt.begin() + face_index);
	return ret_val;
}
int MultiFaceTracker::add_new_face(int new_face_index)
{
	int ret_val = 0;
	vector<cv::Mat> new_face_vector;
	vector<cv::Mat> new_descript_vector;
	new_face_vector.push_back(curr_frame_face_mats[new_face_index]);
	new_descript_vector.push_back(curr_frame_face_descripts[new_face_index]);
	prev_losses_cunt.push_back(0);
	prev_face_descripts.push_back(new_descript_vector);
	prev_face_mats.push_back(new_face_vector);
	return ret_val;
}
void MultiFaceTracker::add_to_face_stream(int index, cv::Mat& face_img, cv::Mat& descripts)
{
	if (prev_face_mats[index].size() < max_queue_size)
	{
		prev_face_mats[index].push_back(face_img);
		prev_face_descripts[index].push_back(descripts);
	}
	else
	{
		prev_face_mats[index].erase(prev_face_mats[index].begin());
		prev_face_descripts[index].erase(prev_face_descripts[index].begin());
		prev_face_mats[index].push_back(face_img);
		prev_face_descripts[index].push_back(descripts);
	}
}
int MultiFaceTracker::find_face(cv::Rect2d detected_face_rect, int frame_counter)
{
	const float MIN_TRACKING_INTERSECTION_RATIO = 0.5;
	const float MAX_DIST_RATIO = 0.2;
	int found_index = -1;
	cv::Point2d center_detected = { detected_face_rect.x + detected_face_rect.width * 0.5,
		detected_face_rect.y + detected_face_rect.height * 0.5 };
	float detected_box_area = detected_face_rect.area();
	int tracked_faces_count = tracked_faces.size();
	int max_intersection_index = -1;
	float max_intersection_ratio = 0.0;
	int dx = 0;
	int dy = 0;
	int dist = 0;
	float dist_ratio = 0;
	for (int j = 0; j < tracked_faces_count; j++)
	{
		FaceCase face_case = tracked_faces[j][tracked_faces[j].size() - 1];
		if (face_case.index < frame_counter - 2)
		{
			continue;
		}
		cv::Rect2d box = face_case.bounding_rect;

		cv::Point2d center_box = { box.x + box.width * 0.5,
			box.y + box.height * 0.5 };
		dx = (center_box.x - center_detected.x);
		dy = (center_box.y - center_detected.y);
		dist = sqrt(float(dx*dx + dy*dy));
		dist_ratio = dist / max(detected_face_rect.width, box.width);
		float intersection_ratio0 = (detected_face_rect & box).area() / box.area();
		float intersection_ratio1 = (detected_face_rect & box).area() / detected_box_area;
		if (
			center_detected.inside(box) &&
			(dist_ratio < MAX_DIST_RATIO || intersection_ratio0 > MIN_TRACKING_INTERSECTION_RATIO || intersection_ratio1 > MIN_TRACKING_INTERSECTION_RATIO)
			)
		{
			if (intersection_ratio0 > max_intersection_ratio)
			{
				max_intersection_index = j;
				max_intersection_ratio = intersection_ratio0;
			}
			if (intersection_ratio1 > max_intersection_ratio)
			{
				max_intersection_index = j;
				max_intersection_ratio = intersection_ratio1;
			}
		}
	}
	found_index = max_intersection_index;
	return found_index;
}
int MultiFaceTracker::refine_rect(cv::Rect2d &rect)
{
	int ret_val = 0;
	rect.x = rect.x < 0 ? 0 : rect.x;
	rect.y = rect.y < 0 ? 0 : rect.y;
	rect.width = rect.x + rect.width > frame_width ? frame_width - rect.x : rect.width;
	rect.height = rect.y + rect.height > frame_height ? frame_height - rect.y : rect.height;
	return ret_val;
}
bool MultiFaceTracker::is_got_out_face(cv::Rect2d face_rect)
{
	bool ret_val = false;
	//const int edge_Len = 10;
	//if (face_rect.x <= edge_Len || face_rect.x + face_rect.width >= (frame_width - edge_Len) ||
	//	face_rect.y <= edge_Len || face_rect.y + face_rect.height >= (frame_height - edge_Len))
	//{
	//	ret_val = 0;
	//}
	return ret_val;
}
void MultiFaceTracker::reset_all()
{
	prev_face_descripts.clear();
	prev_face_mats.clear();;
	prev_losses_cunt.clear();;

	new_faces_count = 0;;
	new_faces.clear();;
	tracked_faces_box.clear();;
	tracked_faces.clear();;
	curr_frame_face_descripts.clear();;
	curr_frame_face_mats.clear();;
}

int MultiFaceTracker::update(time_t current_time, vector<int>jpg_high_params, int frame_counter)
{
	int ret_val = 0;
	double MAX_MATCHED_NEAR_FACES_DIST = 0.3;

	const int MIN_LOSS_COUNT = 10;
	const int MIN_MATCHED_COUNT = 2;
	vector<cv::KeyPoint> key_points;
	char str_tmp[_MAX_PATH];
	faces_img = 0;

	int last_detected_faces_count = prev_face_mats.size();
	curr_frame_face_descripts.clear();
	curr_frame_face_mats.clear();
	new_faces.clear();
	vector<bool> founded_tracked_faces(tracked_faces.size(), false);
	//#define USE_PARALLEL_FOR
#ifndef USE_PARALLEL_FOR
	for (int i = 0; i < faces_count; i++)
#else
	Concurrency::parallel_for(0, faces_count, [&](int i)
#endif
	{
		cv::Rect2d bound_rect;
		int MAX_NOSE_EYES_DISTANCE = max(4, abs((*faces)[i].pnts[0].x - (*faces)[i].pnts[1].x) / 5);
		int left_eye_nose_distance = ((*faces)[i].pnts[0].x - (*faces)[i].pnts[2].x);
		int right_eye_nose_distance = ((*faces)[i].pnts[2].x - (*faces)[i].pnts[1].x);
		if ((left_eye_nose_distance > -MAX_NOSE_EYES_DISTANCE && left_eye_nose_distance < MAX_NOSE_EYES_DISTANCE) ||
			(right_eye_nose_distance > -MAX_NOSE_EYES_DISTANCE && right_eye_nose_distance < MAX_NOSE_EYES_DISTANCE)
			)
		{
#ifdef DISPLAY_FACES
			cv::circle(frame, cv::Point((*faces)[i].pnts[0].x, (*faces)[i].pnts[0].y), 5, cv::Scalar(255, 255, 0), 5);

			cv::circle(frame, cv::Point((*faces)[i].pnts[1].x, (*faces)[i].pnts[1].y), 5, cv::Scalar(255, 0, 0), 5);
			cv::imshow("eyes", frame);cv::waitKey();
#endif

			continue;
		}
		bound_rect.x = (*faces)[i].x;
		bound_rect.y = (*faces)[i].y;
		bound_rect.width = (*faces)[i].width;
		bound_rect.height = (*faces)[i].height;
		if (bound_rect.height >= MAX_FACE_SIZE_TO_DETECTION || bound_rect.width >= MAX_FACE_SIZE_TO_DETECTION)
		{
			continue;
		}
		refine_rect(bound_rect);
		frame(bound_rect).copyTo(faces_img(bound_rect));
		cv::Mat curr_face_img;
		cv::Rect crop_bound_rect = bound_rect;
		int crop_size = crop_bound_rect.width * 0.15;
		crop_bound_rect.x += crop_size / 2;
		crop_bound_rect.y += crop_size / 2;
		crop_bound_rect.width -= crop_size;
		crop_bound_rect.height -= crop_size;
		resize(frame(crop_bound_rect), curr_face_img, cv::Size(150, 150));
		if (0)
		{
			cv::imshow("Faces____", curr_face_img); cv::waitKey();
		}
		int t1 = clock();
		//keypoint_detector->detect(curr_face_img, key_points);
		feature_ext->detect(curr_face_img, key_points);
		if (key_points.size() < 1)
#ifndef USE_PARALLEL_FOR
			continue;
#else
			return;
#endif

		cv::Mat descripts;
		try
		{

			for (size_t mm = 0; mm < key_points.size(); mm++)
			{
				key_points[mm].class_id = 1;
			}
			feature_ext->compute(curr_face_img, key_points, descripts);
			curr_frame_face_descripts.push_back(descripts);
			curr_frame_face_mats.push_back(curr_face_img);
		}
		catch (const std::exception&ex)
		{
			cout << ex.what();
		}
		int founded_index = -1;
		founded_index = find_face(bound_rect, frame_counter);
		if (founded_index >= 0 && prev_face_descripts.size() >= founded_index)
		{
			float min_dist = 100000000;
			int min_index = -1;
			int matched_count = 0;
			int tracked_faces_match_count = 0;
			auto prev_descripts = prev_face_descripts[founded_index];
			last_detected_faces_count = prev_descripts.size();
			for (size_t mm = last_detected_faces_count > 4 ? last_detected_faces_count - 4 : 0; mm < last_detected_faces_count; mm++)
			{
				vector<cv::DMatch>matches;
				float dist = 0;
				if (prev_descripts[mm].rows > 3)
				{
					bf_matcher->match(descripts, prev_descripts[mm], matches);
					if (matches.size() >= MIN_MATCHED_COUNT)
					{
						for (size_t nn = 0; nn < matches.size(); nn++)
						{
							dist += matches[nn].distance;
						}
						dist /= matches.size();

						if (dist < min_dist)
						{
							min_dist = dist;
							min_index = mm;
						}
						if (dist < MAX_MATCHED_NEAR_FACES_DIST)
						{
							matched_count += matches.size();
							tracked_faces_match_count++;
						}
					}
				}
				matched_count /= last_detected_faces_count;
			}
			if (min_index < 0 || tracked_faces_match_count < 1)
			{
				founded_index = -1;
			}
		}
		if (founded_index < 0 && prev_face_mats.size() >= 1)
		{
			float min_dist = 100000000;
			int min_index = 0;
			int second_min_index = 0;
			int tracked_faces_size = prev_face_mats.size();

			int min_case_index = -1;
			int second_min_case_index = 0;
			int matched_count = 0;
			vector<int> tracked_faces_match_count(tracked_faces_size, 0);
			for (size_t tracked_faces_index = 0; tracked_faces_index < tracked_faces_size; tracked_faces_index++)
			{


				last_detected_faces_count = prev_face_mats[tracked_faces_index].size();
				auto prev_descripts = prev_face_descripts[tracked_faces_index];
				for (size_t mm = last_detected_faces_count > 10 ? last_detected_faces_count / 2 : 0; mm < last_detected_faces_count; mm++)
				{
					vector<cv::DMatch>matches;
					float dist = 0;
					if (prev_descripts[mm].rows > 3)
					{
						bf_matcher->match(descripts, prev_descripts[mm], matches);
						if (matches.size() >= MIN_MATCHED_COUNT)
						{
							for (size_t nn = 0; nn < matches.size(); nn++)
							{
								dist += matches[nn].distance;
							}
							dist /= matches.size();

							if (dist < min_dist)
							{
								second_min_index = min_index;
								second_min_case_index = min_case_index;
								min_dist = dist;
								min_index = mm;
								min_case_index = tracked_faces_index;
							}
							if (dist < MAX_MATCHED_FACES_DIST)
							{
								matched_count += matches.size();
								tracked_faces_match_count[tracked_faces_index]++;
							}
						}
					}

				}

			}
			/*if (min_dist < 0.6 && (min_case_index == second_min_case_index) ||
				tracked_faces_match_count[min_case_index] >= 2)*/
			if (min_case_index >= 0 /*&& matched_count > 20*/ &&
				(tracked_faces_match_count[min_case_index] >= 1 ||
					tracked_faces_match_count[second_min_case_index] >= 1))
			{
				founded_index = tracked_faces_match_count[min_case_index] >= tracked_faces_match_count[second_min_case_index] ? min_case_index : second_min_case_index;
				int founded_face_index = tracked_faces_match_count[min_case_index] >= tracked_faces_match_count[second_min_case_index] ? min_index : second_min_index;
#ifdef DISPLAY_FACES
				ShowFaces("_MatchFace", 0);
				cv::imshow("Faces_____MatchFace", curr_face_img);
				if (founded_index >= 0 && min_index >= 0)
					cv::imshow("MatchedFace_MatchFace", prev_face_mats[founded_index][min_index]);

				int xx = cv::waitKey(WAIT_DELAY * 10);
				if (xx == 27)
				{
					xx = 50;
				}
#endif
				add_to_face_stream(founded_index, curr_face_img, descripts);
				FaceCase new_face_case;
				new_face_case.index = frame_counter;
				new_face_case.bounding_rect = bound_rect;
				new_face_case.pnts_1 = (*faces)[i].pnts[0];
				new_face_case.pnts_2 = (*faces)[i].pnts[1];
				new_face_case.pnts_3 = (*faces)[i].pnts[2];
				new_face_case.pnts_4 = (*faces)[i].pnts[3];
				new_face_case.pnts_5 = (*faces)[i].pnts[4];
				new_face_case.id = tracked_faces[founded_index][tracked_faces[founded_index].size() - 1].id;
				get_time_string(current_time, str_tmp);
				new_face_case.face_img_path = prifix_path + new_face_case.id + "\\face_" + str_tmp + ".jpg";
				new_face_case.color = tracked_faces[founded_index][tracked_faces[founded_index].size() - 1].color;
				tracked_faces[founded_index].push_back(new_face_case);
				if (Save_Faces)
				{
					worker_face_saver(frame(new_face_case.bounding_rect).clone(), new_face_case.face_img_path);
					/*arr_threads_to_save_images.push_back(thread(worker_image_saver, frame(new_face_case.bounding_rect).clone(), new_face_case.face_img_path));
					if (arr_threads_to_save_images.size() > 1000)
					{
						arr_threads_to_save_images.erase(arr_threads_to_save_images.begin());
					}*/
					//cv::imwrite(new_face_case.face_img_path, frame(new_face_case.bounding_rect), jpg_high_params);
				}

			}
			else
			{
				new_faces.push_back(bound_rect);
			}
			t1 = clock() - t1;
			t1++;
		}
		else if (founded_index >= 0)
		{
#ifdef DISPLAY_FACES
			ShowFaces("_FoundedFace", 500);
			cv::imshow("Faces_____FoundedFace", curr_face_img);
			cv::imshow("MatchedFace_FoundedFace", prev_face_mats[founded_index][prev_face_mats[founded_index].size() - 1]);
			int xx = cv::waitKey(WAIT_DELAY);
			if (xx == 27)
			{
				xx = 50;
			}
#endif
			add_to_face_stream(founded_index, curr_face_img, descripts);
			FaceCase new_face_case;
			new_face_case.index = frame_counter;
			new_face_case.bounding_rect = bound_rect;
			new_face_case.pnts_1 = (*faces)[i].pnts[0];
			new_face_case.pnts_2 = (*faces)[i].pnts[1];
			new_face_case.pnts_3 = (*faces)[i].pnts[2];
			new_face_case.pnts_4 = (*faces)[i].pnts[3];
			new_face_case.pnts_5 = (*faces)[i].pnts[4];
			new_face_case.id = tracked_faces[founded_index][tracked_faces[founded_index].size() - 1].id;
			get_time_string(current_time, str_tmp);
			new_face_case.face_img_path = prifix_path + new_face_case.id + "\\face_" + str_tmp + ".jpg";
			new_face_case.color = tracked_faces[founded_index][tracked_faces[founded_index].size() - 1].color;
			tracked_faces[founded_index].push_back(new_face_case);
			if (Save_Faces)
			{
				worker_face_saver(frame(new_face_case.bounding_rect).clone(), new_face_case.face_img_path);
				/*arr_threads_to_save_images.push_back(thread(worker_image_saver, frame(new_face_case.bounding_rect).clone(), new_face_case.face_img_path));
				if (arr_threads_to_save_images.size() > 1000)
				{
					arr_threads_to_save_images.erase(arr_threads_to_save_images.begin());
				}*/
			}
			/*if(Save_Faces)
				cv::imwrite(new_face_case.face_img_path, frame(new_face_case.bounding_rect), jpg_high_params);
			*/

		}
		else
		{
			new_faces.push_back(bound_rect);
		}
		if (founded_index >= 0)
		{
			founded_tracked_faces[founded_index] = true;
		}
	}
#ifdef USE_PARALLEL_FOR
	);
#endif

	for (size_t i = 0; i < founded_tracked_faces.size(); i++)
	{
		if (!founded_tracked_faces[i])
		{
			prev_losses_cunt[i]++;
			tracked_faces[i][tracked_faces[i].size() - 1].is_lost = true;
		}
		else
		{
			prev_losses_cunt[i] = 0;
			tracked_faces[i][tracked_faces[i].size() - 1].is_lost = false;
		}
	}
	for (size_t i = 0; i < prev_losses_cunt.size(); i++)
	{
		if (prev_losses_cunt[i] > MIN_LOSS_COUNT || (tracked_faces.size() > i && is_got_out_face(tracked_faces[i][tracked_faces[i].size() - 1].bounding_rect)))
		{
			remove(i);
		}
	}
#ifdef SAVE_FRAMES
	cv::imwrite("D:\\1.jpg", faces_img);
	cv::imshow("Faces", faces_img);
	int xx = cv::waitKey(1);
	if (xx == 27)
	{
		xx = 0;
	}
#endif
	new_faces_count = new_faces.size();


	return ret_val;
}