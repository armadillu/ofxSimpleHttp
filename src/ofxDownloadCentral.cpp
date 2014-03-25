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
				if (downloaders.size()){
					startQueue();
				}
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
			bd->cancelBatch(true);
		}
	}
	mutex.unlock();
}


void ofxDownloadCentral::cancelAllDownloads(){
	mutex.lock();
	if (busy){
		if(downloaders.size() > 0){
			ofxBatchDownloader * bd = downloaders[0];
			bd->cancelBatch(true);
			for(int i = 0; i < downloaders.size(); i++){
				delete downloaders[0];
				downloaders.erase(downloaders.begin());
			}
		}
		busy = false;
	}
	mutex.unlock();
}


void ofxDownloadCentral::draw(float x, float y, bool drawAllPending){

	if (busy){
		mutex.lock();
		if(downloaders.size() > 0){
			vector<string> allURLs;
			ofDrawBitmapString("ofxDownloadCentral queue: " + ofToString(downloaders.size()), x + 3, y + 12);
			ofxBatchDownloader * bd = downloaders[0];
			bd->draw(x, y + 16);
			if(drawAllPending){
				vector<string> allPending;
				for(int i = 0; i < downloaders.size(); i++){
					vector<string> pending = downloaders[i]->pendingURLs();
					for(int j = 0; j < pending.size(); j++){
						allPending.push_back(pending[j]);
					}
				}
				ofSetColor(0,255,100);
				for(int i = 0; i < allPending.size(); i++){
					ofDrawBitmapString( allPending[i], x + 3, y + 100 + 15 * i);
				}
			}
		}
		mutex.unlock();
	}
}

