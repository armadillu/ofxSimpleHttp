#pragma once

#include "ofMain.h"
#include "ofxSimpleHttp.h"

#define OUTPUT_DIRECTORY "tempDownloads"

class ofApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();
		void exit();

		void keyPressed(int key);

		float p1;
		bool timeSample;

		void newResponse(ofxSimpleHttpResponse &response);

		ofxSimpleHttp http;
		vector<string> downloadList;

};
