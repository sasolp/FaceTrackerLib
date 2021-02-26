// stdafx.cpp : source file that includes just the standard includes
// FaceTracker.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"
// TODO: reference any additional headers you need in STDAFX.H
// and not in this file
void logger(char* file_name, char * format, ...)
{
	ofstream stream_out(string("d:\\") + file_name, ofstream::app);
	char str[_MAX_PATH];
	va_list arg_list;
	va_start(arg_list, format);
	string str_format = format;
	vsprintf(str, (str_format + "\n").c_str(), arg_list);
	va_end(arg_list);
	stream_out << str;
	stream_out.close();
}