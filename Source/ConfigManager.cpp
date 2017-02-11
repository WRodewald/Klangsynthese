#include "ConfigManager.h"
#include <fstream>

using namespace tinyxml2;

ConfigManager::ConfigManager(std::string fileString)
	: fileString(fileString)
{
	readConfigXML();
}

ConfigManager::~ConfigManager()
{
}

void ConfigManager::setValue(ID id, std::string value)
{
	for (int i = 0; i < keys.size(); i++)
	{
		// id already exists, update value
		if (keys[i].first == id)
		{
			keys[i].second = value;
			return;
		}
	}

	// new entry, append to list
	keys.push_back(std::pair<ID, std::string>(id, value));
}

bool ConfigManager::getValue(ID id, std::string &value)
{
	for (int i = 0; i < keys.size(); i++)
	{
		// id already exists, update value
		if (keys[i].first == id)
		{
			value = keys[i].second;
			return true;
		}
	}

	return false;
}

void ConfigManager::writeChanges()
{
	tinyxml2::XMLDocument doc;
	doc.SetBOM(false);
	doc.NewDeclaration("");
	
	auto it = keys.begin();
	while (it != keys.end())
	{
		ID id = it->first;
		auto group = doc.FirstChildElement(id.group.c_str());
		if (!group)
		{
			group = doc.NewElement(id.group.c_str());
			doc.InsertEndChild(group);
		}

		group->SetAttribute(id.key.c_str(), it->second.c_str());

		it++;
	}

	FILE *f = fopen(fileString.c_str(), "w+");
	doc.SaveFile(f);


	if (f) fclose(f);
}

void ConfigManager::readConfigXML()
{
	XMLDocument doc;
	
	std::ifstream fileStream(fileString);
	std::string fileInput((std::istreambuf_iterator<char>(fileStream)),std::istreambuf_iterator<char>());

	doc.Parse(fileInput.c_str());
	XMLElement * group = doc.FirstChildElement();
	while (group != NULL)
	{
		const XMLAttribute * key = group->FirstAttribute();
		while (key != NULL)
		{
			ID id = ID(group->Name(),  key->Name());
			std::string val = key->Value();

			keys.push_back(std::pair<ID, std::string>(id, val));
			key = key->Next();
		}

		group = group->NextSiblingElement();
	}
}
