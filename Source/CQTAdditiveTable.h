#pragma once
#include <string>
#include <vector>

class CQTAdditiveTable
{
	friend class CQTTablePlayer; // ugly 
public:

	enum FileType 
	{
		Invalid,			// file is fucked up beyond repair
		CQTTable,			// file defines a CQT table
		IncludeFile			// file includes other files
	};

	enum FileVariable
	{
		NoVariable,			// used as an invalid-variable state
		Include,			// include to another file which is then being imported
		SampleRate,			// sample rate used in the analysis
		HopSize,			// or frame size, number of samples per envelope-sample
		BinsPerSemitone,	// frequency-resolution, number of bin's per semitones
		Frequency,			// followed by a float defines the frequency of the following bin
		Amplitudes			// data of a frequency-bin, envelope-samples
	};

	
public:
	// #################### CONSTRUCTOR ####################
	
	CQTAdditiveTable();


	static CQTAdditiveTable * createTableFromFile(std::string filename);
		

private:
	// #################### HELPER ####################

	// returns the variable specified by string 'input'
	static FileVariable getVariable(const std::string &input);

	// splits a string 'variable=value' in variable (FileVariable), changes input to only contain the value
	static FileVariable separateVariableAndValue(std::string &input);

	// functions try to convert a string ('str') into float / int (ref. 'val'), return false if not possible
	static bool			 convertString(const std::string &str, float &val);
	static bool			 convertString(const std::string &str, int   &val);

	// function tries to read a CSV line from 'str', writes the content in container
	static bool			readValueList(const std::string &str, std::vector<float> &container);
	static int			estimateValueListSize(const std::string &str);

	bool isValid();


private:
	// #################### MEMBER ####################

	std::vector<float>				frequencyTable; // list of frequencies
	std::vector<std::vector<float>> amplitudeTable; // list of amplitudes assigned (1. dim) to frequencyTable;

	bool  initialized;
	float sampleRate;
	float hopSize;
	float binsPerSemitone; // pls don't make that a fractional number

};


