#pragma once

#include <iostream>
#include <map>


class ConfigManager
{
	ConfigManager *initConfigManager(std::string file);
public:


	void		setValue(std::string key, std::string value);
	std::string getValue(std::string key);

private:

	std::map<std::string, std::string> keys;



};