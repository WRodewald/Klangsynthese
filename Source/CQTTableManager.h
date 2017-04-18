#pragma once

#include "CQTAdditiveTable.h"

#include <memory>
#include <vector>
#include <array>


class CQTTableManager
{
	friend class cereal::access;
public:
	enum class FileType
	{
		Invalid,			// file is fucked up beyond repair
		CQTTable,			// file defines a CQT table
		IncludeFile			// file includes other files
	};

	enum class FileVariable
	{
		NoVariable,			// used as an invalid-variable state
		Include,			// include to another file which is then being imported
		SampleRate,			// sample rate used in the analysis
		HopSize,			// or frame size, number of samples per envelope-sample
		BinsPerSemitone,	// frequency-resolution, number of bin's per semitones
		MidiNote,			// midi note this talbe is supposed to be used for
		Frequency,			// followed by a float defines the frequency of the following bin
		Amplitudes			// data of a frequency-bin, envelope-samples
	};

public:
	CQTTableManager(bool debugMode = false);

	void loadBinaryCache(std::string fileName);
	void storeBinaryCache(std::string fileName);
	
	bool importTextFile(std::string fileName, bool useMT);
	
	bool prepareTables();
	bool prepareTables(std::pair<unsigned int, unsigned int> range = { 0,128 });

	bool sanity();

	const CQTAdditiveTable * getTable(unsigned int midiNote) const;

public:

	static CQTAdditiveTable * createTableFromFile(std::string filename, bool debugMode = false);

	// #################### SERIALIZATION ####################
private:

	template<class Archive>
	void serialize(Archive & archive)
	{
		archive(tables, debugMode);
	}

	// #################### HELPER ####################
private:

	bool importIncludeFile(std::string fileName, bool useMT);
	bool importCQTTableFile(std::string fileName);

	static FileType		getFileTypeFromFile(std::string filename);

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
		
private:
	std::array<std::shared_ptr<CQTAdditiveTable>, 128> tables;

	bool debugMode;
};