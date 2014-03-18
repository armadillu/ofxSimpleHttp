//
//  ofxDownloadCentral.cpp
//  emptyExample
//
//  Created by Oriol Ferrer Mesià on 14/03/14.
//
//


#include "ofxDownloadCentral.h"
#include "ofEvents.h"
#include "ofEventUtils.h"

ofxDownloadCentral::ofxDownloadCentral(){

	verbose = true;
	busy = false;

}

ofxDownloadCentral::~ofxDownloadCentral(){
	cancelAllDownloads();
}

void ofxDownloadCentral::startQueue(){

	if (!busy){
		busy = true;
		if(downloaders.size() > 0){
			ofxBatchDownloader * bd = downloaders[0];
			bd->startDownloading();
		}else{
			cout << "proper wtf! " << endl;
		}
	}
}


void ofxDownloadCentral::update(){

	mutex.lock();
	if (busy){
		if(downloaders.size() > 0){
			ofxBatchDownloader * bd = downloaders[0];
			bd->update();
			if (!bd->isBusy()){ //this one is over! start the next one!
				//ofRemoveEvent??? mmm... TODO!
				downloaders.erase(downloaders.begin());
				delete bd;
				busy = false;
				startQueue();
			}
		}
	}
	mutex.unlock();
}


void ofxDownloadCentral::cancelCurrentDownload(){
	mutex.lock();
	if (busy){
		if(downloaders.size() > 0){
			ofxBatchDownloader * bd = downloaders[0];
			bd->cancelBatch();
		}
	}
	mutex.unlock();
}


void ofxDownloadCentral::cancelAllDownloads(){
	mutex.lock();
	if (busy){
		if(downloaders.size() > 0){
			ofxBatchDownloader * bd = downloaders[0];
			bd->cancelBatch();
			for(int i = 0; i < downloaders.size(); i++){
				delete downloaders[0];
				downloaders.erase(downloaders.begin());
			}
		}
		busy = false;
	}
	mutex.unlock();
}


void ofxDownloadCentral::draw(float x, float y){

	if (busy){
		if(downloaders.size() > 0){
			ofDrawBitmapString("ofxDownloadCentral queue: " + ofToString(downloaders.size()), x + 3, y + 12);
			ofxBatchDownloader * bd = downloaders[0];
			bd->draw(x, y + 16);
		}
	}
}

