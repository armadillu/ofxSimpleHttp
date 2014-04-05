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
	onlySkipDownloadIfChecksumMatches = true;
	maxURLsToList = 2;
}

ofxDownloadCentral::~ofxDownloadCentral(){
	cancelAllDownloads();
}


void ofxDownloadCentral::setMaxURLsToList(int max){
	maxURLsToList = max;
}


void ofxDownloadCentral::setVerbose(bool b){
	verbose = b;
}

void ofxDownloadCentral::setIdleTimeAfterEachDownload(float seconds){
	idleTimeAfterDownload = seconds;
}



void ofxDownloadCentral::setNeedsChecksumMatchToSkipDownload(bool needs){
	onlySkipDownloadIfChecksumMatches = needs;
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


bool ofxDownloadCentral::isBusy(){
	return busy;
}

int ofxDownloadCentral::getNumPendingDownloads(){

	mutex.lock();
	int c = 0;
	for(int i = 0; i < downloaders.size(); i++){
		vector<string> pending = downloaders[i]->pendingURLs();
		c+= pending.size();
	}
	mutex.unlock();
	return c;
}

string ofxDownloadCentral::getDrawableInfo(bool drawAllPending){

	string aux = "## ofxDownloadCentral queued Jobs: " + ofToString(downloaders.size()) + " ###########################\n";
	mutex.lock();
	if(downloaders.size() > 0){
		vector<string> allURLs;

		ofxBatchDownloader * bd = downloaders[0];
		aux += bd->getDrawableString() + "\n";

		if(drawAllPending){
			int c = 0;
			aux += "## List of Queued Downloads ####################################\n";
			vector<string> allPending;

			for(int i = 0; i < downloaders.size(); i++){
				vector<string> pending = downloaders[i]->pendingURLs();
				for(int j = 0; j < pending.size(); j++){
					allPending.push_back(pending[j]);
					if (c <= maxURLsToList){
						aux += "#  " + pending[j] + "\n";
					}
					c++;
					if (c == maxURLsToList +1){
						aux += "#  (...)\n";
					}
				}
			}
			aux += "################################################################";
		}
	}
	mutex.unlock();
	return aux;
}


void ofxDownloadCentral::draw(float x, float y, bool drawAllPending){
	if (busy){
		string aux = getDrawableInfo(drawAllPending);
		ofDrawBitmapString(aux, x, y);
	}
}
