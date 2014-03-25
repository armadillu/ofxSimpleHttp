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
	video->setLoopMode(OF_LOOP_NONE);
	ofAddListener(video->videoIsReadyEvent, this, &AssetObject::videoIsReadyCallback);

	image = new ofxThreadedImage();
}


void AssetObject::setup(ofxDownloadCentral *downloader){

	d = downloader;

	pos.x = ofRandom(-ofGetWidth(), ofGetWidth()) ;
	pos.y = 0.8 * ofRandom(-ofGetHeight(), ofGetHeight());
	pos.z = ofRandom(-ofGetWidth(), 0);
	speed = ofRandom(0.5, 2.2);

	loadRandomAsset();
};


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

			name = report.responses[i].fileName;
			if (report.responses[i].contentType == "image/jpeg"){

				isVideo = false;
				delete image;
				image = new ofxThreadedImage();
				image->loadImageThreaded(report.responses[i].absolutePath);

			}else{

				isVideo = true;
				if(video){ //if we had a video, delete it
					delete video;
				}
				video = new ofxThreadedVideoPlayer();
				video->setLoopMode(OF_LOOP_NONE);
				ofAddListener(video->videoIsReadyEvent, this, &AssetObject::videoIsReadyCallback);
				video->loadVideo(report.responses[i].absolutePath);
				//video->loadVideo("downloads_0/chaos_mini_mute.mov");
			}
			ok = true;
		}
		i++;
	}
	if (!ok){
		cout << "failed to download!" << endl;
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

	vector<string> allURLS;

	allURLS.push_back("http://uri.cat/dontlook/localProjects/CWRU/chaos.mov");
	allURLS.push_back("http://uri.cat/dontlook/localProjects/CWRU/bill.mov");
	allURLS.push_back("http://uri.cat/dontlook/localProjects/CWRU/walle.mov");
	allURLS.push_back("http://uri.cat/dontlook/localProjects/CWRU/walle2.mov");
	allURLS.push_back("http://uri.cat/dontlook/localProjects/CWRU/success.mov");
	allURLS.push_back("http://uri.cat/dontlook/localProjects/CWRU/cat.mov");
	allURLS.push_back("http://uri.cat/dontlook/localProjects/CWRU/blackboard.mov");
	allURLS.push_back("http://uri.cat/dontlook/localProjects/CWRU/after.mov");
	allURLS.push_back("http://uri.cat/dontlook/localProjects/CWRU/allpowerful.mov");
	allURLS.push_back("http://uri.cat/dontlook/localProjects/CWRU/beep.mov");
	allURLS.push_back("http://uri.cat/dontlook/localProjects/CWRU/walle3.mov");

//	allURLS.push_back("http://farm8.staticflickr.com/7420/10032530563_86ff701d19_o.jpg");
//	allURLS.push_back("http://farm4.staticflickr.com/3686/9225463176_d0bf83a992_o.jpg");
//	allURLS.push_back("http://farm8.staticflickr.com/7255/6888724266_158ce261a2_o.jpg");
//	allURLS.push_back("http://farm8.staticflickr.com/7047/7034809565_5f80871bff_o.jpg");
//	allURLS.push_back("http://farm8.staticflickr.com/7438/9481688475_e83f92e8b5_o.jpg");
//	allURLS.push_back("http://farm8.staticflickr.com/7321/9481647489_e73bed28e1_o.jpg");
//	allURLS.push_back("http://farm8.staticflickr.com/7367/9484432454_9701453c66_o.jpg");
//	allURLS.push_back("http://farm6.staticflickr.com/5537/9481654243_7b73b87ceb_o.jpg");
//	allURLS.push_back("http://farm4.staticflickr.com/3740/9481659071_3159d318dc_o.jpg");
//	allURLS.push_back("http://farm6.staticflickr.com/5346/9484309488_11ee39298e_o.jpg");
//	allURLS.push_back("http://farm4.staticflickr.com/3802/9484323300_6d3a6a78b5_o.jpg");

	int which = floor(ofRandom(allURLS.size()));

	vector<string> downloadList;
	downloadList.push_back(allURLS[which]);

	waitingForDownload = true;
	d->downloadResources(downloadList,						//list of urls to download
						 this,								//who will be notified
						 &AssetObject::downloadFinished,	//callback method
						 "downloads_" + ofToString(ID)		//destination folder
						 );
}


void AssetObject::update(){

	pos.x += speed;
	if(pos.x > ofGetWidth() - OBJ_DRAW_SIZE){
		pos.x = - ofGetWidth() ;
	}
	if(video) video->update();
}


void AssetObject::draw(){

	ofSetColor(255);
	ofPushMatrix();
		ofTranslate(0,0,pos.z);
		if(isVideo){
			video->draw(pos.x, pos.y, OBJ_DRAW_SIZE, OBJ_DRAW_SIZE * 0.6);
		}else{
			image->draw(pos.x, pos.y, OBJ_DRAW_SIZE, OBJ_DRAW_SIZE * 0.6);
		}

		ofNoFill();
		ofSetColor(255);

		//draw filename
		ofDrawBitmapString( name, pos.x, pos.y - 16);

		//draw progress
		if(waitingForDownload){
			string down;
			switch ( (int)(0.2 * ofGetFrameNum() )%4) {
				case 0: down = "|"; break;
				case 1: down = "/"; break;
				case 2: down = "-"; break;
				case 3: down = "\\"; break;
			}
			ofDrawBitmapString(down , pos.x, pos.y - 32);
		}

		//draw errors
		if(failedToLoad){
			ofDrawBitmapString( "Failed to load!", pos.x, pos.y - 48);
		}

		//red frame around asset
		ofSetColor(255,0,0, 128);
		ofRect(pos.x, pos.y, OBJ_DRAW_SIZE, OBJ_DRAW_SIZE * 0.6);
		ofFill();
	ofPopMatrix();
}
