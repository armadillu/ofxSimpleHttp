//
//  AssetObject.cpp
//  BaseApp
//
//  Created by Oriol Ferrer Mesià on 24/03/14.
//
//

#include "AssetObject.h"

AssetObject::AssetObject(){
	ID = OBJECT_ID;
	OBJECT_ID++;
	isVideo = false;
	failedToLoad = false;
	waitingForDownload = false;
	video = new ofxThreadedVideoPlayer();
	image = new ofxThreadedImage();
}


void AssetObject::downloadFinished(ofxBatchDownloaderReport &report){

	cout << "#### Object with ID "<< ID << " got report back!!" << endl;
	cout << "#### " << report.attemptedDownloads.size() << " attempted; " ;
	cout << report.failedDownloads.size() << " failed, ";
	cout << report.successfulDownloads.size() << " ok!!" << endl;
	for(int i = 0; i < report.responses.size(); i++){
		ofxSimpleHttpResponse r = report.responses[i];
		r.print();
	}

	//load the 1st asset
	bool ok = false;
	int i = 0;
	while (!ok && i < report.responses.size()){

		if (report.responses[i].ok){

			if (report.responses[i].contentType == "image/jpeg"){

				isVideo = false;
				imgsPendingForDeletion.push_back(image);

				image = new ofxThreadedImage();
				image->loadImageThreaded(report.responses[i].absolutePath);

			}else{

				isVideo = true;
				if(video){ //if we had a video, delete it
					video->markForDeletion();
					videosPendingForDeletion.push_back(video);
				}
				video = new ofxThreadedVideoPlayer();
				video->setLoopMode(OF_LOOP_NONE);
				ofAddListener(video->videoIsReadyEvent, this, &AssetObject::videoIsReadyCallback);
				video->loadVideo(report.responses[i].absolutePath);
			}
			ok = true;
		}
		i++;
	}
	failedToLoad = !ok;
	waitingForDownload = false;
}

//we get notified here when the video is ready for playback
void AssetObject::videoIsReadyCallback(ofxThreadedVideoPlayerStatus &status){
	cout << "video at "<< status.path << " is ready for playback!" << endl;
	//start playback as soon as the video is ready!
	//status.player->play();
	video->play();
}


void AssetObject::loadRandomAsset(){

	if (waitingForDownload){
		cout << "already waiting for a download! ignoring request" << endl;
		return;
	}

	int assetID = floor(ofRandom(2));
	vector<string> downloadList;

//	switch (assetID) {
//		case 0:
//			//pic
//			downloadList.push_back("http://farm8.staticflickr.com/7420/10032530563_86ff701d19_o.jpg");
//			break;
//		case 1:
//			//video!
//			downloadList.push_back("http://uri.cat/dontlook/localProjects/CWRU/chaos.mov");
//			break;
////		case 2:
////			//will fail! wrong url
////			downloadList.push_back("http://farm8.staticflickr.com/7438/9481688475_e83f92e8b5_o.jpg");
////			break;
//		default:
//			break;
//	}

	downloadList.push_back("http://uri.cat/dontlook/localProjects/CWRU/chaos_mini.mov");

	waitingForDownload = true;
	d->downloadResources(downloadList,						//list of urls to download
						 this,								//who will be notified
						 &AssetObject::downloadFinished,	//callback method
						 "downloads_" + ofToString(ID)		//destination folder
						 );

}



void AssetObject::setup(ofxDownloadCentral *downloader){

	d = downloader;

	pos.x = ofRandom(ofGetWidth() - OBJ_DRAW_SIZE) ;
	pos.y = ofRandom(OBJ_DRAW_SIZE, ofGetHeight() - OBJ_DRAW_SIZE);
	speed = ofRandom(0.5, 1.2);

	loadRandomAsset();
};


void AssetObject::update(){

	pos.x += speed;
	if(pos.x > ofGetWidth() ){
		pos.x = - OBJ_DRAW_SIZE;
	}
	if(video) video->update();

	//clean up old videos that might be pending for deletion
	for(int i = videosPendingForDeletion.size() - 1; i >= 0; i--){
        if (videosPendingForDeletion[i]->isReadyToBeDeleted()){
            ofxThreadedVideoPlayer * v = videosPendingForDeletion[i];
            videosPendingForDeletion.erase(videosPendingForDeletion.begin() + i);
            delete v;
        }
    }

	//clean up old videos that might be pending for deletion
//	for(int i = imgsPendingForDeletion.size() - 1; i >= 0; i--){
//        if (imgsPendingForDeletion[i]->isReadyToBeDeleted()){
//            ofxThreadedVideoPlayer * img = imgsPendingForDeletion[i];
//            imgsPendingForDeletion.erase(imgsPendingForDeletion.begin() + i);
//            delete img;
//        }
//    }

}


void AssetObject::draw(){

	ofSetColor(255);
	if(isVideo){
		video->draw(pos.x, pos.y, OBJ_DRAW_SIZE, OBJ_DRAW_SIZE * 0.6);
	}else{
		image->draw(pos.x, pos.y, OBJ_DRAW_SIZE, OBJ_DRAW_SIZE * 0.6);
	}

	ofNoFill();
	ofSetColor(255);
	if(failedToLoad){
		ofDrawBitmapString( "Failed to load!", pos.x, pos.y + 16);
	}
	ofSetColor(255,0,0, 128);
	ofRect(pos, OBJ_DRAW_SIZE, OBJ_DRAW_SIZE * 0.6);
	ofFill();


}

