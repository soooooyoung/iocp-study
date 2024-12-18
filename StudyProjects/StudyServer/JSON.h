#pragma once
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class JSON
{
public:
	JSON();
	~JSON();

	static json ReadJsonFromFile(const std::string& filePath);
};