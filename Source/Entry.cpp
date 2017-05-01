#include "AudioIO.h"
#include "MidiIO.h"
#include "ConfigManager.h"
#include "CQTTable.h"
#include "CQTTable.h"
#include "TablePlayer.h"
#include "TableManager.h"
#include "Processor.h"
#include "Util/FilePath.h"
#include <iostream>
#include <fstream>
#include <regex>
#include <future>
#include <cmath>
// function blocks process by waiting for enter key
void waitForStdIn()
{
	std::cin.sync();
	std::cin.get();
}

#define  returnFail    { waitForStdIn(); return -1;}
#define  returnSuccess { waitForStdIn(); return -0;}


int main(int argc, char **argv)
{

	bool autoMode       = false;
	bool multiThreading = false;
	bool configMode		= false;
	bool debugMode		= false;
	float cqtBinThreshold = 0;
	int numVoices = 1;
	std::string fileName;

	std::regex configRegex("[\\\\\\/-]?C(onfig)?", std::regex::icase);
	std::regex debugRegex("[\\\\\\/-]?D(ebug)?", std::regex::icase);
	std::regex thresRegex("[\\\\\\/-]?T(hreshold)?", std::regex::icase);
	std::regex mtRegex("[\\\\\\/-]?MT", std::regex::icase);
	std::regex voicesRegex("[\\\\\\/-]?v(oices)", std::regex::icase);
	std::regex autoRegex("[\\\\\\/-]?a(uto)", std::regex::icase);



	// #################### read arguments ####################
	int argIdx = 1;
	while (argIdx < argc)
	{
		std::string argument = std::string(argv[argIdx]);
		
		if(std::regex_match(argument,		autoRegex))		autoMode	   = true;
		else if(std::regex_match(argument,		configRegex))	configMode	   = true;
		else if (std::regex_match(argument, debugRegex))	debugMode	   = true;
		else if (std::regex_match(argument, debugRegex))	debugMode	   = true;
		else if (std::regex_match(argument, mtRegex))		multiThreading = true;
		else if (std::regex_match(argument, thresRegex))
		{
			argIdx++;
			if (argIdx >= argc)
			{
				std::cout << "Unexpected Argument (Threshold)" << std::endl;
				returnFail;
			}
			std::string thresholdString = std::string(argv[argIdx]);

			try
			{
				cqtBinThreshold = std::stof(thresholdString);
			}
			catch (const std::exception &e)
			{
				std::cout << "Unexpected Argument (Threshold)" << std::endl;
				returnFail;
			}

		}
		else if (std::regex_match(argument, voicesRegex))
		{
			argIdx++;
			if (argIdx >= argc)
			{
				std::cout << "Unexpected Argument (Num Voices)" << std::endl;
				returnFail;
			}

			std::string voicesStr = std::string(argv[argIdx]);

			try
			{
				numVoices = std::stoi(voicesStr);
				if (numVoices < 1) numVoices = 1;
			}
			catch (const std::exception &e)
			{
				std::cout << "Unexpected Argument (Num Voices)" << std::endl;
				returnFail;
			}

		}
		else
		{
			if (fileName.size() != 0) // fileName already set
			{
				std::cout << "Unexpected Argument" << std::endl;
				returnFail;
			}
			fileName = argument;
		}
		argIdx++;
	}
	
	// #################### create Table manager ####################

	std::cout << "Create Table Manager" << std::endl;

	TableManager tableManager(debugMode);

	// find out if we include a cache file or a txt file 

	auto suffixStart = fileName.find_last_of('.');
	auto suffix = fileName.substr(suffixStart, fileName.size());	
	
	if (suffix != ".table")
	{
		std::cout << "Loading Table File " << fileName << ":" << std::endl;;
		bool success = tableManager.importTextFile(fileName, debugMode);
		if (success)
		{
			std::cout << "success" << std::endl;
		}
		else
		{
			std::cout << "failed" << std::endl;
			returnFail;
		}

		std::cout << "Interpolating Tables" << std::endl;
		tableManager.prepareTablesAutoRange();

		try
		{
			auto fileNameWOSuffix = fileName.substr(0, suffixStart);
			auto fileNameCache    = fileNameWOSuffix.append(".table");
			tableManager.storeBinaryCache(fileNameCache);
		}
		catch (const std::exception & e)
		{
			std::cout << "Error caching file" << std::endl;
			returnFail;
		}
	}
	else
	{
		std::cout << "Loading CQT table cache: ";
		try
		{
			tableManager.loadBinaryCache(fileName);
			std::cout << "success" << std::endl;
		}
		catch (const std::exception & e)
		{
			std::cout << "failed" << std::endl;
			returnFail;
		}
	}

	std::cout << "Table Manager Initialized" << std::endl;

	auto errCode = tableManager.sanity();
	if (errCode != TableManager::ErrorCode::NoError)
	{
		if (errCode & TableManager::ErrorCode::InvalidNumBins)
		{
			std::cout << "Table Manger contains tables with various number of bins" << std::endl;
		}
		if (errCode & TableManager::ErrorCode::InvalidTables)
		{
			std::cout << "Table Manger contains invalid tables" << std::endl;
		}
		returnFail;
	}

	// #################### Audio IO ####################

	// create audio context, start in debug mode if debugMode==true
	AudioIO audio(debugMode);

	// prepare audiocontext configuration
	AudioIO::CallbackConfig cfg;
	cfg.inChannels = 0;
	cfg.outChannels = 2;
	cfg.frameSize = 64;
	cfg.sampleRate = 44100;
	cfg.iSampleRate = 1. / cfg.sampleRate;
	cfg.inDevice = AudioIO::NoDevice;
	cfg.outDevice = AudioIO::DefaultDevice;



	// #################### config stuff ####################

	std::cout << "Reading Config.xml" << std::endl;

	// load config manager, read config.xml if existing
	ConfigManager config("config.xml");

	// if config is true update current variables with getConfiguration dialog
	if (configMode)
	{

		std::cout << std::endl << "Audio Configuration" << std::endl;

		AudioIO::ConfigKeys keys =(AudioIO::ConfigKeys)( AudioIO::ConfigKeys::Conf_OutputDevice | AudioIO::ConfigKeys::Conf_FrameSize | AudioIO::ConfigKeys::Conf_SampleRate);
		audio.getConfiguration(cfg, keys);

		config.setValue(ConfigManager::ID("Audio", "SampleRate"),	std::to_string(cfg.sampleRate));
		config.setValue(ConfigManager::ID("Audio", "FrameSize"),	std::to_string(cfg.frameSize));
		config.setValue(ConfigManager::ID("Audio", "OutputDevice"), std::to_string(cfg.outDevice));
	}

	// get audio configuration from config.xml
	std::string cfgValue;
	if (config.getValue(ConfigManager::ID("Audio", "SampleRate"), cfgValue))
	{
		cfg.sampleRate = std::atof(cfgValue.c_str());
	}
	if (config.getValue(ConfigManager::ID("Audio", "FrameSize"), cfgValue))
	{
		cfg.frameSize = std::atof(cfgValue.c_str());
	}
	if (config.getValue(ConfigManager::ID("Audio", "OutputDevice"), cfgValue))
	{
		cfg.outDevice = std::atoi(cfgValue.c_str());
	}


	// store changes to configuration
	config.writeChanges();




	// #################### create Processor ####################


    Processor processor(numVoices, &tableManager, autoMode);
	processor.prepare(cfg);
	audio.setCallback(&processor);

	// #################### MIDI Part 2 ####################

	MidiCaster midiCaster;

	midiCaster.addListener(&processor);
	midiCaster.open();

	// #################### start stream ####################


	std::cout << "Starting Audio Callback" << std::endl;

	bool state = true;
	state &= audio.initStream(cfg);
	state &= audio.startStream();

	if (!state)
	{
		std::cout << "Faild to Start Audio Callback" << std::endl;
		returnFail;
	}


	// #################### debug loop ####################

	std::cout << "Audio Callback Running" << std::endl;
	auto blocker = std::async(waitForStdIn);
	while (std::future_status::timeout == blocker.wait_for(std::chrono::seconds(1))) // wait_for waits for a second and repeats following loop
	{
		if(debugMode)
		{
			std::this_thread::sleep_for(std::chrono::seconds(1));
			std::cout << "## Stream State ##" << std::endl;
			auto state = audio.getDebugData();
			std::cout << "CPU Load: \t\t" <<  (int) std::round(state->cpuLoad * 100) << "%" << std::endl;
			std::cout << "Frame Size (Int/Ext): \t" << state->internFrameSize << "/" << state->externFrameSize << std::endl;
			std::cout << "Frames Processed:\t" << state->processedSamples << std::endl;
			std::cout << std::endl;
		}
	}



	// #################### deconstruction ####################
	audio.closeStream();

	std::cout << "Audio Callback Stopped" << std::endl;

	returnSuccess;

}
