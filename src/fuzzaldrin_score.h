#pragma once
#include <string>

float fuzzaldrin_score(const std::string& string, const std::string& query);
float fuzzaldrin_basename_score(const std::string& string, const std::string& query, float score);
