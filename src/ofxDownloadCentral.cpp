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
#include "Poco/LocalDateTime.h"
#include "Poco/DateTimeFormatter.h"

ofxDownloadCentral::ofxDownloadCentral(){

	verbose = true;
	busy = false;
	onlySkipDownloadIfChecksumMatches = true;
	maxURLsToList = 10;
	idleTimeAfterDownload = 0.0f;
	avgSpeed = 0.0f;
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
		if(downloaders.size() > 0){
			busy = true;
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
				downloaders.erase(downloaders.begin());
				if(avgSpeed == 0.0f){
					avgSpeed = bd->getAverageSpeed();
				}else{
					avgSpeed = avgSpeed * 0.75 + 0.25 * bd->getAverageSpeed();
				}
				downloadedSoFar += bd->getDownloadedBytesSoFar();

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

	mutex.lock();
	int total = 0;
	for(int i = 0; i < downloaders.size(); i++){
		total += downloaders[i]->getNumSuppliedUrls();
	}

	string aux;
	int numQueuedJobs = downloaders.size();
	if(downloaders.size() > 0){
		vector<string> allURLs;

		ofxBatchDownloader * bd = downloaders[0];
		aux += bd->getDrawableString() + "\n";

		total -= bd->getNumSuppliedUrls() - bd->pendingDownloads();

		if(drawAllPending){
			int c = 0;
			aux += "//// Remaining Downloads /////////////////////////////////////////\n";
			vector<string> allPending;
			bool done = false;
			for(int i = 0; i < downloaders.size(); i++){
				vector<string> pending = downloaders[i]->pendingURLs();
				for(int j = 0; j < pending.size(); j++){
					allPending.push_back(pending[j]);
					if (c < maxURLsToList){
						aux += "//   " + pending[j] + "\n";
					}
					if (c == maxURLsToList ){
						aux += "//   ...\n";
						done = true;
					}
					c++;
				}
				if (done ) break;
			}
			aux += "//////////////////////////////////////////////////////////////////";
		}
	}
	mutex.unlock();

	float elapsedTime = ofGetElapsedTimef() - downloadStartTime;
	float timeLeft = 0.0f;
	int numProcessed = downloadStartJobsNumber - numQueuedJobs;
	string estFinish;
	if (numProcessed > 0){
		timeLeft = numQueuedJobs * elapsedTime / float(numProcessed);
		Poco::Timespan timeToAdd;
		timeToAdd.assign(timeLeft, 0);
		string timeFormat = "%H:%M on %w %e, %b %Y";
		Poco::LocalDateTime now;
		now += timeToAdd;
		estFinish = Poco::DateTimeFormatter::format(now, timeFormat);
	}

	string spa = "     ";
	string header = "//// ofxDownloadCentral queued Jobs: " + ofToString(numQueuedJobs) + " ///////////////////////////\n"
	"//// Jobs executed:        " + spa + ofToString(numProcessed) + "\n" +
	"//// Total Downloads Left: " + spa + ofToString(total) + "\n" +
	"//// Elapsed Time:         " + spa + ofxSimpleHttp::secondsToHumanReadable(elapsedTime, 1) + "\n" +
	"//// Estimated Time Left:  " + spa + ofxSimpleHttp::secondsToHumanReadable(timeLeft, 1) + "\n"
	"//// Estimated Completion: " + spa + estFinish + "\n" +
	"//// Avg Download Speed:   " + spa + ofxSimpleHttp::bytesToHumanReadable(avgSpeed, 1) + "/sec\n" +
	"//// Downloaded So Far:    " + spa + ofxSimpleHttp::bytesToHumanReadable(downloadedSoFar, 1) +
	"\n\n";
	return header + aux;
}


void ofxDownloadCentral::draw(float x, float y, bool drawAllPending){
	if (busy){
		string aux = getDrawableInfo(drawAllPending);
		ofDrawBitmapString(aux, x, y);
	}
}
