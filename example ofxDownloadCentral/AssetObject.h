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

#define OBJ_DRAW_SIZE	200

static int OBJECT_ID = 0;

class AssetObject{

public:

	AssetObject();
	void setup(ofxDownloadCentral *downloader);
	void loadRandomAsset();

	void update();
	void draw();

	void downloadFinished(ofxBatchDownloaderReport &report);
	void videoIsReadyCallback(ofxThreadedVideoPlayerStatus &status);

	int ID;
	ofxDownloadCentral *d;

	ofVec2f pos;
	float speed;

	ofxThreadedImage * image;
	ofxThreadedVideoPlayer* video;
	bool isVideo;
	bool failedToLoad;

	vector<ofxThreadedVideoPlayer*> videosPendingForDeletion;
	vector<ofxThreadedImage*> imgsPendingForDeletion;

	bool waitingForDownload;
};



#endif /* defined(__BaseApp__AssetObject__) */
