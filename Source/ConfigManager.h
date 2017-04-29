#pragma once

#include <iostream>
#include <vector>
#include "tinyxml2.h"

class ConfigManager
{
public:

	class ID
	{
	public:
		ID(std::string group, std::string key) : group(group), key(key) {};
		
		bool operator==(const ID &other)
		{
			return ((key.compare(other.key) == 0) && (group.compare(other.group) == 0));
		}

		std::string key;
		std::string group;
	};

	ConfigManager(std::string pathToFile);
	~ConfigManager();

	void			setValue(ID id, std::string value);
	bool			getValue(ID id, std::string &value);

	void writeChanges();

private:

	void readConfigXML();

private:

	std::vector<std::pair<ID, std::string>> keys;
	
	std::string fileString;


};
