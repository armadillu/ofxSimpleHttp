#pragma once

#include "ofMain.h"
#include "ofxSimpleHttp.h"

class testApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();

		void keyPressed(int key);

		float p1;
		bool timeSample;

		void newResponse(ofxSimpleHttpResponse &response);

		ofxSimpleHttp http;
		vector<string> downloadList;

};
