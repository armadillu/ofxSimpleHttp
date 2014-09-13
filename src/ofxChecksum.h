
#pragma once

#include "ofMain.h"

class ofxChecksum{
public:
	static bool sha1(string filePath, string sha1String, bool verbose = true);	
};

