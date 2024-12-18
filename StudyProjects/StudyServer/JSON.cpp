#include "pch.h"
#include "JSON.h"

JSON::JSON()
{
}

JSON::~JSON()
{
}

json JSON::ReadJsonFromFile(const std::string& filePath)
{
	std::ifstream file(filePath);
	json data = json::parse(file);

	return data;
}
