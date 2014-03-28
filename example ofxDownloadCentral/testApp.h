#pragma once

#include "ofMain.h"
#include "ofxBatchDownloader.h"
#include "ofxDownloadCentral.h"
#include "AssetObject.h"
#include "ofxTimeMeasurements.h"
#include "ofxHistoryPlot.h"

#define NUM_OBJECTS 10


class testApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();

		void drawClock();

		void keyPressed(int key);

		vector<AssetObject*> objects;


		ofxDownloadCentral downloader;


		ofEasyCam cam;
		ofxHistoryPlot * frameTimePlot;
		int numAssetLoads;

		bool autoAlloc;
};
