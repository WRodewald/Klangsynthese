#pragma once

#include "HarmonicTable.h"
#include "CQTTable.h"
#include "ATable.h"

#include <memory>
#include <vector>
#include <array>


class TableManager
{
	friend class cereal::access;
public:
	enum class FileType
	{
		Invalid,			// file is fucked up beyond repair
		CQTTable,			// file defines a CQT table
		HarmonicTable,		// file defines a Harmonic table
		IncludeFile			// file includes other files
	};

	enum class ErrorCode
	{
		NoError			  = 0,
		InvalidTables	  = 1 << 1,
		InvalidNumBins	  = 1<<2
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
		Harmonic,			// followed by a float defines the harmonic index of the following bin
		Amplitudes			// data of a frequency-bin, envelope-samples
	};

public:
	TableManager(bool debugMode = false);

	void loadBinaryCache(std::string fileName);
	void storeBinaryCache(std::string fileName);
	
	bool importTextFile(std::string fileName, bool useMT);
	
	bool prepareTablesAutoRange();
	bool prepareTables(std::pair<unsigned int, unsigned int> range = { 0,128 });

	void applyThreshold(float val);
	void limitNumActiveBins(unsigned int num);
	void unlimitNumActiveBins();

	ErrorCode sanity();

	const ATable * getTable(unsigned int midiNote) const;

public:

	static CQTTable * createCQTTableFromFile(std::string filename, bool debugMode = false);
	static HarmonicTable * createHarmonicTableFromFile(std::string filename, bool debugMode = false);

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
	bool importTableFile(ATable *table);

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
	std::array<std::shared_ptr<ATable>, 128> tables;

	bool debugMode;
};

inline TableManager::ErrorCode operator|(const TableManager::ErrorCode &lhs, const TableManager::ErrorCode & rhs)
{
	return static_cast<TableManager::ErrorCode>(static_cast<int>(lhs) | static_cast<int>(rhs));
}

inline bool operator&(const TableManager::ErrorCode &lhs, const TableManager::ErrorCode & rhs)
{
	return static_cast<bool>(static_cast<int>(lhs) & static_cast<int>(rhs));
}
