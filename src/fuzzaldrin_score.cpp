// Original ported from: string_score.js: String Scoring Algorithm 0.1.10
// https://github.com/joshaven/string_score
//
// Copyright (C) 2009-2011 Joshaven Potter <yourtech@gmail.com>
// Special thanks to all of the contributors listed here https://github.com/joshaven/string_score
// MIT license: http://www.opensource.org/licenses/mit-license.php
//
// Re-ported from Atom's Fuzzaldrin score.js file to C++

#include "fuzzaldrin_score.h"

const std::string PathSeparator = "/"; // Assuming Unix-like path separator

float fuzzaldrin_basename_score(const std::string& string, const std::string& query, float score) {
  long index = string.length() - 1;
  while (string[index] == PathSeparator[0]) { index--; } // Skip trailing slashes
  std::size_t slashCount = 0;
  const std::size_t lastCharacter = index;
  std::string base;

  while (index >= 0) {
    if (string[index] == PathSeparator[0]) {
      slashCount++;
      if (base.empty()) {
        base = string.substr(index + 1, lastCharacter - index);
      }
    } else if (index == 0) {
      if (lastCharacter < (string.length() - 1)) {
        if (base.empty()) {
          base = string.substr(0, lastCharacter + 1);
        }
      } else {
        if (base.empty()) {
          base = string;
        }
      }
    }
    index--;
  }

  // Basename matches count for more.
  if (base == string) {
      score *= 2;
  } else if (!base.empty()) {
      score += fuzzaldrin_score(base, query);
  }

  // Shallow files are scored higher
  const std::size_t segmentCount = slashCount + 1;
  const std::size_t depth = std::max<std::size_t>(1, 10 - segmentCount);
  score *= depth * 0.01;
  return score;
}

bool queryIsLastPathSegment(const std::string& string, const std::string& query) {
    auto res = string[string.length() - query.length() - 1] == PathSeparator[0];

    if (string[string.length() - query.length() - 1] == PathSeparator[0]) {
      auto res2 = string.rfind(query) == (string.length() - query.length());
      return string.rfind(query) == (string.length() - query.length());
    }
    return false;
}

float fuzzaldrin_score(const std::string& candidate, const std::string& query) {
    std::string string = candidate;

    if (string == query) {
      return 1.0;
    }
    if (queryIsLastPathSegment(string, query)) {
      return 1.0;
    }

    float totalCharacterScore = 0.0;
    const std::size_t queryLength = query.length();
    const std::size_t stringLength = string.length();

    std::size_t indexInQuery = 0;
    std::size_t indexInString = 0;

    while (indexInQuery < queryLength) {
        char character = query[indexInQuery++];
        std::size_t lowerCaseIndex = string.find(std::tolower(character));
        std::size_t upperCaseIndex = string.find(std::toupper(character));
        std::size_t minIndex = std::min(lowerCaseIndex, upperCaseIndex);
        if (minIndex == std::string::npos) { minIndex = std::max(lowerCaseIndex, upperCaseIndex); }
        indexInString = minIndex;
        if (indexInString == std::string::npos) { return 0.0; }

        float characterScore = 0.1;

        // Same case bonus.
        if (string[indexInString] == character) { characterScore += 0.1; }

        if ((indexInString == 0) || (string[indexInString - 1] == PathSeparator[0])) {
            // Start of string bonus
            characterScore += 0.8;
        } else if (string[indexInString - 1] == '-' || string[indexInString - 1] == '_' || string[indexInString - 1] == ' ') {
            // Start of word bonus
            characterScore += 0.7;
        }

        // Trim string to after the current abbreviation match
        string = string.substr(indexInString + 1);
        totalCharacterScore += characterScore;
    }

    const float queryScore = totalCharacterScore / queryLength;
    auto div = (queryLength / (float) stringLength);
    return ((queryScore * (queryLength / (float) stringLength)) + queryScore) / 2.0;
}
