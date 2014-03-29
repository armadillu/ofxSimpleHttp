#pragma once

#include "ofMain.h"
#include "ofxDownloadCentral.h"

class testApp : public ofBaseApp{

public:

	void setup();
	void update();
	void draw();
	void keyPressed(int key);
	void drawClock();

	//download ready callback
	void downloadFinished(ofxBatchDownloaderReport &report);

	ofxDownloadCentral downloader;

};
