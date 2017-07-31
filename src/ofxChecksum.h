
#pragma once

#include "ofMain.h"

class ofxChecksum{
public:
	static bool sha1(const string & filePath,
					 const string & sha1String,
					 bool verbose = false);

	static string calcSha1(const string & filePath);
};

