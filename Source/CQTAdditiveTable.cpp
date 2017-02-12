#include "CQTAdditiveTable.h"
#include <fstream>
#include <regex>
#include <iostream>

// #################### CONSTRUCTOR ####################

CQTAdditiveTable::CQTAdditiveTable()
	:
	initialized(false),
	binsPerSemitone(0),
	sampleRate(0),
	hopSize(0)
{
}

CQTAdditiveTable * CQTAdditiveTable::createTableFromFile(std::string filename)
{
	unsigned int lineCounter = 0;
	std::ifstream infile(filename);
	// we expect something like variable=value, value is a int, float or list of both (separated by comma)

	CQTAdditiveTable * table = new CQTAdditiveTable();

	// unescapable characters: 
	// '=' for variable/value splot
	// ',' for splits of a list

	float currentFrequency = -1; 
	unsigned int freqBin = 0;

	std::string line;
	while (std::getline(infile, line))
	{
		lineCounter++;


		// after separate, line contains value only
		FileVariable var = separateVariableAndValue(line);

		switch (var)
		{
			// #################### Include ####################
			case FileVariable::Include:
			{
				// don't bother for now	
			} break;

			// #################### Include ####################
			case FileVariable::SampleRate:
			{
				if (!convertString(line, table->sampleRate))
				{
					delete table; 
					std::cout << "Invalid argument in line " << lineCounter << ", file: " << filename << std::endl;
					return NULL;
				}
			} break;

			// #################### BinsPerSemitone ####################
			case FileVariable::BinsPerSemitone:
			{
				if (!convertString(line, table->binsPerSemitone))
				{
					delete table;
					std::cout << "Invalid argument in line " << lineCounter << ", file: " << filename << std::endl;
					return NULL;
				}
			} break;

			// #################### HopSize ####################
			case FileVariable::HopSize:
			{
				if (!convertString(line, table->hopSize))
				{
					delete table;
					std::cout << "Invalid argument in line " << lineCounter << ", file: " << filename << std::endl;
					return NULL;
				}
			} break;

			// #################### Frequency ####################
			case FileVariable::Frequency:
			{
				if (!convertString(line, currentFrequency))
				{
					delete table;
					std::cout << "Invalid argument in line " << lineCounter << ", file: " << filename << std::endl;
					return NULL;
				}
				std::cout << "Import: Frequency Bin " << ++freqBin << std::endl;
			} break;

			// #################### Amplitudes ####################
			case FileVariable::Amplitudes:
			{
				if (currentFrequency < 0)
				{
					delete table;
					std::cout << "Invalid variable in line " << lineCounter << ", file: " << filename << std::endl;
					std::cout << "Frequency was expected to precede Amplitude" << std::endl;
					return NULL;
				}
				
				// get size of list
				int listSize = estimateValueListSize(line);

				// prepare 
				table->frequencyTable.push_back(currentFrequency);
				table->amplitudeTable.push_back(std::vector<double>());
				table->amplitudeTable.back().reserve(listSize);

				// import csv list into std vector, 
				if (!readValueList(line, table->amplitudeTable.back()))
				{
					delete table;
					std::cout << "Invalid argument in line " << lineCounter << ", file: " << filename << std::endl;
					std::cout << "Amplitude list could not be paresd correctly" << std::endl;
					return NULL;
				}
				currentFrequency = -1; // this is set back to -1 to ensure an 
			} break;
		}
	}

	
	if (!table->isValid())
	{
		delete table;
		return NULL;
	}
	else
	{
		return table;
	}
}

CQTAdditiveTable::FileVariable CQTAdditiveTable::getVariable(const std::string & input)
{
	// imput is expected to be some string with leading / trailing widecards
	// first thing to do is to strip those

	static std::regex regexInclude			("[\\s]*Include[\\s]*", std::regex::flag_type::icase); 
	static std::regex regexSampleRate		("[\\s]*SampleRate[\\s]*", std::regex::flag_type::icase); 
	static std::regex regexHopSize			("[\\s]*HopSize[\\s]*", std::regex::flag_type::icase); 
	static std::regex regexBinsPerSemitone	("[\\s]*BinsPerSemitone[\\s]*", std::regex::flag_type::icase);
	static std::regex regexFrequency		("[\\s]*Frequency[\\s]*", std::regex::flag_type::icase);
	static std::regex regexAmplitudes		("[\\s]*Amplitudes?[\\s]*", std::regex::flag_type::icase); 

	if (std::regex_match(input, regexInclude))			return FileVariable::Include;
	if (std::regex_match(input, regexSampleRate))		return FileVariable::SampleRate;
	if (std::regex_match(input, regexHopSize))			return FileVariable::HopSize;
	if (std::regex_match(input, regexBinsPerSemitone))	return FileVariable::BinsPerSemitone;
	if (std::regex_match(input, regexFrequency))		return FileVariable::Frequency;
	if (std::regex_match(input, regexAmplitudes))		return FileVariable::Amplitudes;
		
	return FileVariable::NoVariable;
}

CQTAdditiveTable::FileVariable CQTAdditiveTable::separateVariableAndValue(std::string & input)
{
	std::string variableString;
	size_t delimPos = input.find("=");
	if (delimPos == std::string::npos) return FileVariable::NoVariable;

	variableString = input.substr(0, delimPos); 
	input.erase(input.begin(), input.begin() + delimPos+1);

	return getVariable(variableString);
}

bool CQTAdditiveTable::convertString(const std::string & str, float & val)
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

bool CQTAdditiveTable::convertString(const std::string & str, int & val)
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

int CQTAdditiveTable::estimateValueListSize(const std::string & str)
{
	int count = 0;
	for (int i = 0; i < str.size(); i++)
	{
		count += (str[i] == ',');
	}
	return count;
}

bool CQTAdditiveTable::isValid()
{
	// check for all arguments
	bool validTable = true;
	if (sampleRate <= 0)										validTable = false;
	if (hopSize <= 0)											validTable = false;
	if (binsPerSemitone <= 0)									validTable = false;
	if (frequencyTable.size() <= 0)								validTable = false;
	if (amplitudeTable.size() <= 0)								validTable = false;

	if (!validTable)
	{
		std::cout << "Invalid table: missing arguments or data." << std::endl;
	}

	return validTable;


	// check ratio amplitude arrays : frequencies
	if (frequencyTable.size() != amplitudeTable.size())			validTable = false;
	
	// check number of frames
	int numFrames = amplitudeTable[0].size();
	if (numFrames < 1)
	{
		validTable = false;
		std::cout << "Invalid table: missing arguments or data." << std::endl;
	}
	for (int i = 1; i < amplitudeTable.size(); i++)
	{
		if (amplitudeTable[i].size() != numFrames)
		{
			validTable = false;
			std::cout << "Invalid table: amplitude arrays don't have the same size." << std::endl;
			break;
		}
	}
	

	return false;
}

bool CQTAdditiveTable::readValueList(const std::string & str, std::vector<double>& container)
{
	if (str.size())
	{
		std::string::const_iterator last = str.begin();
		for (std::string::const_iterator i = str.begin(); i != str.end(); ++i) {
			if (*i == ',') {
				const std::string sub(last, i);
				float val = 0;
				if (!convertString(sub, val))
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
			const std::string sub(last, str.end() - 1);
			float val = 0;
			if (convertString(sub, val))
			{
				container.push_back(val); // last argument is optional, we don't abort when conversion was unsuccessful
			}
		}
	}
	return true;
}

// #################### HELPER ####################
