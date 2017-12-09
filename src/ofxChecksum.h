
#pragma once

#include "ofMain.h"

class ofxChecksum{
public:
	static bool sha1(const std::string & filePath,
					 const std::string & sha1String,
					 bool verbose = false);

	static std::string calcSha1(const std::string & filePath);
};

