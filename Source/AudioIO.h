
#pragma once

#include "portaudio.h"
#include <vector>
#include "Util/RingBuffer.h"
#include "Util/StackBuffer.h"


class AudioCallbackProvider;

class AudioIO
{
public:

	static const unsigned int MaxFrameSize = 8192;

	struct StreamData
	{
		AudioIO * ioObject;
	};


	struct CallbackConfig
	{
		unsigned int inChannels;
		unsigned int outChannels;
		unsigned int frameSize;
		float		 sampleRate;

		int			 inDevice;	// if < 0, the default device is used
		int			 outDevice;
	};

	static const int DefaultDevice = -1;
	static const int NoDevice = -2;

	class CallbackData
	{
	public:
		CallbackData(unsigned int inChannels, unsigned int outChannels);
		~CallbackData();

		float ** read;
		float ** write;
	};

public:
	AudioIO();
	~AudioIO();

	bool initStream(CallbackConfig cfg);
	bool startStream();
	bool closeStream();

	CallbackConfig getConfiguration();

	void setCallback(AudioCallbackProvider* callbackProvider) { this->callbackProvider = callbackProvider;  };

private:
	static int portAudioCallback(const void *					 inputBuffer,
								 void *							 outputBuffer,
								 unsigned long					 framesPerBuffer,
								 const PaStreamCallbackTimeInfo* timeInfo,
								 PaStreamCallbackFlags			 statusFlags,
								 void *							 userData);


	void audioCallback(	const float *	inputBuffer,
						float *			outputBuffer,
						unsigned long	framesPerBuffer);

	void processFrame();
	bool evaluatePAError(PaError& err);

	int queryNumber(int min, int max, std::string label);
	int queryNumber(std::vector<int> aviable, std::string label);

	int   askForDevice(std::string query);
	void  printDevice(int deviceID);
private:
	PaStream *stream;

	
	unsigned int bufferSize;
	std::vector<StackBuffer<float>>		inputBuffer;
	std::vector<StackBuffer<float>>		outputBuffer;
	std::vector<RingBuffer<float>>		latencyBuffer; 

	StreamData   streamData;

	CallbackData		  * data;
	CallbackConfig		  * cfg;
	AudioCallbackProvider * callbackProvider;

	
	static bool			portAudioInitialized;
	static unsigned int audioIOInstances;

};

class AudioCallbackProvider
{
public:
	virtual void process(AudioIO::CallbackConfig &cfg, AudioIO::CallbackData &data) = 0;
};
