#pragma once

#include "ofMain.h"
#include "ofxBatchDownloader.h"

class testApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();

		void keyPressed(int key);

		float p1;
		bool timeSample;

		void downloadFinished(ofxBatchDownloaderReport &report);

		ofxBatchDownloader downloader;
		vector<string> downloadList;

};
