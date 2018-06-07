
#pragma once

#include "ofMain.h"

class ofxChecksum{
public:

	//checks if sha1 match between file & expected sha1
	static bool sha1(const std::string & filePath,
					 const std::string & sha1String,
					 bool verbose = false);

	//calc sha1 of a file on disk
	static std::string calcSha1(const std::string & filePath);

	//calc sha1 of a std::string
	static std::string calcSha1FromString(const std::string & data);


	static std::string xxHash(const std::string & filePath);

};

