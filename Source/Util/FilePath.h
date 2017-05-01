#pragma once
#include <string>
#include <memory>
#include <iostream>
#include <cstdlib>
#include <regex>

#if _WIN32
	#define WIN
#elif __APPLE__
	#define Apple
#elif __linux__
	#define LINUX
#elif __unix__ 
	#define LINUX // a bit of a stretch
#endif

namespace FilePath
{

	inline std::string getPathOfFile(std::string file);

	inline char delim();

	inline std::string clipWhiteSpaces(std::string str);


};

std::string FilePath::getPathOfFile(std::string file)
{
#ifdef WIN
	char delim = '\\';
#else
	char delim = '/';
#endif


	auto lastOcc = file.find_last_of(delim);
	if (lastOcc != std::string::npos)
	{
		return file.substr(0, lastOcc);
	}


	return file;
}

char FilePath::delim()
{
#ifdef WIN
	return '\\';
#else
	return '/';
#endif
}

std::string FilePath::clipWhiteSpaces(std::string str)
{
	static const std::regex regexLeadingWS("^[\\s]+", std::regex::icase);
	static const std::regex regexTrailingWS("[\\s]+$", std::regex::icase);

	str = std::regex_replace(str, regexLeadingWS, "");
	return std::regex_replace(str, regexTrailingWS, "");
	 
}

