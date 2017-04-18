#include "CQTTableManager.h"
#include <memory>
#include <fstream>
#include <regex>
#include <iostream>
#include <array>
#include <atomic>
#include <thread>

#include "Util/FilePath.h"

#include "cereal/archives/binary.hpp"
#include "cereal/types/array.hpp"
#include "cereal/types/memory.hpp"

CQTTableManager::CQTTableManager(bool debugMode) : debugMode(debugMode)
{
}

void CQTTableManager::storeBinaryCache(std::string filename)
{
	std::ofstream outfile(filename, std::ios::binary);
	cereal::BinaryOutputArchive archive(outfile);
	this->serialize(archive);
}

void CQTTableManager::loadBinaryCache(std::string filename)
{
	std::ifstream infile(filename, std::ios::binary);
	cereal::BinaryInputArchive archive(infile);
	this->serialize(archive);
}


bool CQTTableManager::importTextFile(std::string filename, bool useMT)
{
	auto fileType = getFileTypeFromFile(filename);

	if (fileType == FileType::CQTTable)
	{

		if(debugMode) std::printf("Importing CQT Table %s \n", filename.c_str());
		// include file, done
		bool res = importCQTTableFile(filename);

		if (debugMode) std::printf("Finished importing %s \n", filename.c_str());
		return res;
	}
	else if (fileType == FileType::IncludeFile)
	{
		if (debugMode) std::printf("Importing Include File %s, \n", filename.c_str());
		return importIncludeFile(filename, useMT);
	}

	return false;

}

bool CQTTableManager::importIncludeFile(std::string filename, bool useMT)
{
	unsigned int lineCounter = 0;
	std::ifstream infile(filename);

	auto prependPath = FilePath::getPathOfFile(filename);
	
	bool success = true;

	std::vector<std::string> includeList;

	std::string line;
	while (std::getline(infile, line))
	{
		lineCounter++;
		if (line.size() > 2 && (line[0] == '/') && (line[1] == '/')) continue; 
		
		// after separate, line contains value only
		FileVariable var = separateVariableAndValue(line);

		if (var == FileVariable::Include)
		{
			auto fullPath = prependPath + FilePath::delim() +  line;
			includeList.push_back(fullPath);
		}
	}
	
	if (includeList.size() > 0)
	{
		std::vector<bool> returnList(includeList.size(), true);

		auto callIncludeFunc = [&](unsigned int idx)
		{
			returnList[idx] = importTextFile(includeList[idx], false);
		};

		if (useMT)
		{
			std::vector<std::thread> threadList;
			
			// start threads
			for (int i = 0; i < includeList.size() - 1; i++) 
				threadList.push_back(std::thread(callIncludeFunc, i));

			// do work myself
			returnList.push_back(true);
			callIncludeFunc(includeList.size()-1);

			// join all the threads
			for (int i = 0; i < includeList.size() - 1; i++) 
				threadList[i].join();
		}
		else
		{
			for (int i = 0; i < includeList.size(); i++) 
				callIncludeFunc(i);
		}
		
		bool success = true;
		 
		for (int i = 0; i < includeList.size() - 1; i++) 
			success &= returnList[i];

	}
	return success;
}

bool CQTTableManager::importCQTTableFile(std::string fileName)
{
	// include file, done
	auto cqtTable = createTableFromFile(fileName);
	if (cqtTable == nullptr) return false;

	// get assigned midi note
	auto midiNote = cqtTable->getMidiNote();
	if (midiNote > 127) return false;

	// remove currently assigned table, if existing
	if (tables[midiNote] != nullptr) tables[midiNote] = nullptr;

	// insert new table
	tables[midiNote] = std::shared_ptr<CQTAdditiveTable>(cqtTable);
	return true;
}

CQTAdditiveTable * CQTTableManager::createTableFromFile(std::string filename, bool debugMode)
{
	if (getFileTypeFromFile(filename) != FileType::CQTTable) return nullptr;

	unsigned int lineCounter = 0;
	std::ifstream infile(filename);


	CQTAdditiveTable::Config cfg;
	std::vector<CQTAdditiveTable::Bin> bins;
	
	int assignedMidiNote = -1;

	float currentFrequency = -1;
	unsigned int freqBin = 0;

	int lastReported20Bins = 0;

	std::string line;
	while (std::getline(infile, line))
	{
		lineCounter++;


		// after separate, line contains value only
		FileVariable var = separateVariableAndValue(line);

		switch (var)
		{		
			// #################### MidiNote ####################
			case FileVariable::MidiNote:
			{
				if (!convertString(line, assignedMidiNote))
				{
					std::cout << "Invalid argument in line " << lineCounter << ", file: " << filename << std::endl;
					return nullptr;
				}
			} break;

			// #################### SampleRate ####################
			case FileVariable::SampleRate:
			{
				if (!convertString(line, cfg.sampleRate))
				{
					std::cout << "Invalid argument in line " << lineCounter << ", file: " << filename << std::endl;
					return nullptr;
				}
			} break;

			// #################### BinsPerSemitone ####################
			case FileVariable::BinsPerSemitone:
			{
				if (!convertString(line, cfg.binsPerSemitone))
				{
					std::cout << "Invalid argument in line " << lineCounter << ", file: " << filename << std::endl;
					return nullptr;
				}
			} break;

			// #################### HopSize ####################
			case FileVariable::HopSize:
			{
				if (!convertString(line, cfg.hopSize))
				{
					std::cout << "Invalid argument in line " << lineCounter << ", file: " << filename << std::endl;
					return nullptr;
				}
			} break;

			// #################### Frequency ####################
			case FileVariable::Frequency:
			{
				if (!convertString(line, currentFrequency))
				{
					std::cout << "Invalid argument in line " << lineCounter << ", file: " << filename << std::endl;
					return nullptr;
				}
			} break;

			// #################### Amplitudes ####################
			case FileVariable::Amplitudes:
			{
				if (currentFrequency < 0)
				{
					std::cout << "Invalid variable in line " << lineCounter << ", file: " << filename << std::endl;
					std::cout << "Frequency was expected to precede Amplitude" << std::endl;
					return nullptr;
				}

				// get size of list
				int listSize = estimateValueListSize(line);

				// prepare 
				bins.push_back(CQTAdditiveTable::Bin());
				auto & binData = bins.back();
				binData.frequency = currentFrequency;
				binData.envelope.reserve(listSize);

				// import csv list into std vector, 
				if (!readValueList(line, binData.envelope))
				{
					std::cout << "Invalid argument in line " << lineCounter << ", file: " << filename << std::endl;
					std::cout << "Amplitude list could not be paresd correctly" << std::endl;
					return NULL;
				}

				if (debugMode)
				{
					auto bin20 = bins.size() / 20;
					if (bin20 > lastReported20Bins)
					{
						std::printf("%d bins in to %s\n", bins.size(), filename.c_str());
						lastReported20Bins = bin20;
					}
				}

				currentFrequency = -1; // this is set back to -1 to ensure an 
			} break;
		}
	}

	CQTAdditiveTable * table = new CQTAdditiveTable();
	table->setConfig(cfg);
	table->setBins(std::move(bins));
	if (assignedMidiNote < 0) assignedMidiNote = table->estimateMidiNote();
	table->setMidiNote(assignedMidiNote);

	if (!table->isValid())
	{
		delete table;
		return nullptr;
	}

	return table;
}

bool CQTTableManager::prepareTables()
{
	// this version determines the prepared range simply by checking the lowest and highest supported midi notes
	int lowestTable = 128;
	int highestTable = -1;
	for (int i = 0; i < tables.size(); i++)
	{
		if (tables[i] != nullptr)
		{
			if (i < lowestTable)  lowestTable = i;
			if (i > highestTable) highestTable = i;
		}
	}

	if (lowestTable > highestTable) return false;
	return prepareTables({ lowestTable, highestTable});

}

bool CQTTableManager::prepareTables(std::pair<unsigned int, unsigned int> range)
{
	if (range.first > range.second) std::swap(range.first, range.second);

	if (range.second >= tables.size()) range.second = tables.size() - 1;

	// lambda helpers
	auto findNextTableFrom = [&](unsigned int idx) -> unsigned int
	{
		for (int k = idx; k < tables.size(); k++)
		{
			if (tables[k] != nullptr) return k;
		}
		return -1;
	};

	int lastTableIdx = -1;
	int nextTableIdx = -1;

	for (int i = range.first; i >= 0; i--)
	{
		if (tables[i] != nullptr)
		{
			lastTableIdx = i;
			break;
		}
	}

	nextTableIdx = findNextTableFrom(range.first);

	for (int i = range.first; i <= range.second; i++)
	{
		if (tables[i] == nullptr)
		{
			bool lastTableInRange = lastTableIdx != -1;
			bool nextTableInRange = nextTableIdx != -1;
			if (lastTableInRange && nextTableInRange)
			{
				auto newTable = std::shared_ptr<CQTAdditiveTable>(CQTAdditiveTable::interpolateTable(*tables[lastTableIdx], *tables[nextTableIdx], i));
				tables[i] = newTable;
				if (newTable->getData()[0].envelope.size() < 1)
				{
					return false;
				}
			}
			else if (lastTableInRange)
			{
				auto newTable = std::make_shared<CQTAdditiveTable>(*tables[lastTableIdx]);
				newTable->shiftFrequencyTo(i);
				tables[i] = newTable;
				if (newTable->getData()[0].envelope.size() < 1)
				{
					return false;
				}
			}
			else if (nextTableInRange)
			{
				auto newTable = std::make_shared<CQTAdditiveTable>(*tables[nextTableIdx]);
				newTable->shiftFrequencyTo(i);
				tables[i] = newTable;
				if (newTable->getData()[0].envelope.size() < 1)
				{
					return false;
				}
			}
		}
		else
		{
			lastTableIdx = i;
			nextTableIdx = findNextTableFrom(i + 1);			
		}
	}
	return true;
}

bool CQTTableManager::sanity()
{
	int numBins   = -1;
	int numFrames = -1;
	for (auto const & table  : tables)
	{
		if (table == nullptr) continue;
		if (!table->isValid()) 
			return false;

		if (numBins < 0)
			numBins = table->getData().size();
		else
			if (numBins != table->getData().size()) 
				return false;
		

		if (numFrames < 0)
			numFrames = table->getData()[0].envelope.size();
		else
			if (numFrames != table->getData()[0].envelope.size()) 
				return false;
	}
	return true;
}

const CQTAdditiveTable * CQTTableManager::getTable(unsigned int midiNote) const
{
	if (midiNote > tables.size()) return nullptr;

	return tables[midiNote].get();
}

CQTTableManager::FileType CQTTableManager::getFileTypeFromFile(std::string filename)
{
	std::ifstream infile(filename);
	// we expect something like variable=value, value is a int, float or list of both (separated by comma)

	static std::regex regexInclude("[\\s]*\\[IncludeFile\\][\\s]*", std::regex::icase);
	static std::regex regexCQTTable("[\\s]*\\[CQTTableFile\\][\\s]*", std::regex::icase);

	std::string line;
	while (std::getline(infile, line))
	{		
		if (std::regex_match(line, regexInclude))  return FileType::IncludeFile;
		if (std::regex_match(line, regexCQTTable)) return FileType::CQTTable;
	}

	return FileType::Invalid;
}

CQTTableManager::FileVariable CQTTableManager::getVariable(const std::string & input)
{
	// imput is expected to be some string with leading / trailing widecards
	// first thing to do is to strip those

	static std::regex regexInclude("[\\s]*Include[\\s]*", std::regex::icase);
	static std::regex regexSampleRate("[\\s]*SampleRate[\\s]*", std::regex::icase);
	static std::regex regexHopSize("[\\s]*HopSize[\\s]*", std::regex::icase);
	static std::regex regexBinsPerSemitone("[\\s]*BinsPerSemitone[\\s]*", std::regex::icase);
	static std::regex regexFrequency("[\\s]*Frequency[\\s]*", std::regex::icase);
	static std::regex regexAmplitudes("[\\s]*Amplitudes?[\\s]*", std::regex::icase);
	static std::regex regexMidiNote("[\\s]*MidiNote[\\s]*", std::regex::icase);

	if (std::regex_match(input, regexInclude))			return FileVariable::Include;
	if (std::regex_match(input, regexSampleRate))		return FileVariable::SampleRate;
	if (std::regex_match(input, regexHopSize))			return FileVariable::HopSize;
	if (std::regex_match(input, regexBinsPerSemitone))	return FileVariable::BinsPerSemitone;
	if (std::regex_match(input, regexFrequency))		return FileVariable::Frequency;
	if (std::regex_match(input, regexAmplitudes))		return FileVariable::Amplitudes;
	if (std::regex_match(input, regexMidiNote))			return FileVariable::MidiNote;

	return FileVariable::NoVariable;
}

CQTTableManager::FileVariable CQTTableManager::separateVariableAndValue(std::string & input)
{
	std::string variableString;
	size_t delimPos = input.find("=");
	if (delimPos == std::string::npos) return FileVariable::NoVariable;

	variableString = input.substr(0, delimPos);
	input.erase(input.begin(), input.begin() + delimPos + 1);

	return getVariable(variableString);
}

bool	CQTTableManager::readValueList(const std::string & str, std::vector<float>& container)
{
	if (str.size())
	{
		std::string tmp;
		tmp.reserve(32);

		std::string::const_iterator last = str.begin();
		for (std::string::const_iterator i = str.begin(); i != str.end(); ++i) {
			if (*i == ',') {
				tmp.assign(last, i);

				float val = 0;
				if (!convertString(tmp, val))
				{
					return false;
				}
				else
				{
					container.push_back(val);
				}
				last = i + 1;
			}
		}

		// check ther rest of the string
		if (last != str.end())
		{
			tmp.assign(last, str.end() - 1);
			float val = 0;
			if (convertString(tmp, val))
			{
				container.push_back(val); // last argument is optional, we don't abort when conversion was unsuccessful
			}
		}
	}
	return true;
}

bool	CQTTableManager::convertString(const std::string & str, float & val)
{
	try
	{
		float res = std::stof(str);
		val = res;
		return true;
	}
	catch (const std::exception &e)
	{
		return false;
	}
}

bool	CQTTableManager::convertString(const std::string & str, int & val)
{
	try
	{
		float res = std::stoi(str);
		val = res;
		return true;
	}
	catch (const std::exception &e)
	{
		return false;
	}
}

int		CQTTableManager::estimateValueListSize(const std::string & str)
{
	int count = 0;
	for (int i = 0; i < str.size(); i++)
	{
		count += (str[i] == ',');
	}
	return count;
}

