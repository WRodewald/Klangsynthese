
#pragma once

#include "portaudio.h"
#include <vector>
#include <string>
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

	enum ConfigKeys
	{
		Conf_InputDevice		= 1,
		Conf_InputChannels		= 2,
		Conf_OutputDevice		= 4,
		Conf_OutputChannels		= 8,
		Conf_SampleRate			= 16,
		Conf_FrameSize			= 32
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

	struct DebugData
	{
		bool  activeStream; // is stream active at all?
		float cpuLoad;
		float processedSamples;
		float externFrameSize;
		float internFrameSize;
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
	AudioIO(bool debugMode);
	~AudioIO();

	bool initStream(CallbackConfig cfg);
	bool startStream();
	bool closeStream();

	void getConfiguration(CallbackConfig &cfg, ConfigKeys keysToConfig);

	void		setCallback(AudioCallbackProvider* callbackProvider) { this->callbackProvider = callbackProvider;  };

	const DebugData	* getDebugData();

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

	void  printDevice(int deviceID);
private:
	PaStream *stream;

	
	unsigned int bufferSize;
	std::vector<StackBuffer<float> >	inputBuffer;
	std::vector<StackBuffer<float> >	outputBuffer;
	std::vector<RingBuffer<float> >		latencyBuffer; 

	StreamData   streamData;

	CallbackData		  * data;
	CallbackConfig		  * cfg;
	AudioCallbackProvider * callbackProvider;

	// debug information
	bool debugMode;
	
	DebugData debugData;
	
	static const float debugUpdateIntervall;
	unsigned int	   samplesPerDebugUpdate;
	unsigned int	   debugUpdateTimer;

	
	// static variables

	static bool			portAudioInitialized;
	static unsigned int audioIOInstances;

};

class AudioCallbackProvider
{
public:
	virtual void process(AudioIO::CallbackConfig &cfg, AudioIO::CallbackData &data) = 0;
};
