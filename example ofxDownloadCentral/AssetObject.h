//
//  AssetObject.h
//  BaseApp
//
//  Created by Oriol Ferrer Mesià on 24/03/14.
//
//

#ifndef __BaseApp__AssetObject__
#define __BaseApp__AssetObject__

#include <iostream>
#include "ofMain.h"
#include "ofxDownloadCentral.h"
#include "ofxThreadedImage.h"
#include "ofxThreadedVideoPlayer.h"

#define OBJ_DRAW_SIZE	300

static int OBJECT_ID = 0;

class AssetObject{

public:

	AssetObject();
	void setup(ofxDownloadCentral *downloader);
	void loadRandomAsset();
	void unloadAssets();

	void update();
	void draw();

	void downloadFinished(ofxBatchDownloaderReport &report);
	void videoIsReadyCallback(ofxThreadedVideoPlayerStatus &status);
	void imageIsReadyCallback(ofxThreadedImageEvent &e);

	int ID;
	ofxDownloadCentral *d;

	ofVec3f pos;
	float speed;

	ofxThreadedImage * image;
	ofxThreadedVideoPlayer* video;

	bool isVideo;
	bool failedToLoad;

	bool waitingForDownload;
	string name;

};



#endif /* defined(__BaseApp__AssetObject__) */
