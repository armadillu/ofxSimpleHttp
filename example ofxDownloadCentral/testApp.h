#pragma once

#include "ofMain.h"
#include "ofxBatchDownloader.h"
#include "ofxDownloadCentral.h"
#include "AssetObject.h"

#define NUM_OBJECTS 5


class testApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();

		void keyPressed(int key);

	vector<AssetObject*> objects;
	ofxDownloadCentral downloader;

	ofEasyCam cam;

};
