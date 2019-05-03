//
//  ofxBatchDownloader.cpp
//  emptyExample
//
//  Created by Oriol Ferrer Mesià on 14/03/14.
//
//


#include "ofxBatchDownloader.h"
#include "ofLog.h"

ofxBatchDownloader::ofxBatchDownloader(){
	busy = false;
	downloadFolder = "_downloads_";
	downloadedSoFar = 0;
	//add download listener
	ofAddListener(http.httpResponse, this, &ofxBatchDownloader::httpResult);
}


void ofxBatchDownloader::setChecksumType(ofxChecksum::Type t){
	checksumType = t;
	http.setChecksumType(t);
}

void ofxBatchDownloader::setNeedsChecksumMatchToSkipDownload(bool needsChecksum){
	http.setNeedsChecksumMatchToSkipDownload(needsChecksum); 
}


void ofxBatchDownloader::setIdleTimeAfterEachDownload(float seconds){
	http.setIdleTimeAfterEachDownload( seconds );
}


void ofxBatchDownloader::setProxyConfiguration(const ofxSimpleHttp::ProxyConfig & c){
	http.setProxyConfiguration(c);
}


void ofxBatchDownloader::setCredentials(const std::string& user, const std::string& password){
	http.setCredentials(user, password);
}


void ofxBatchDownloader::setVerbose(bool b){}


ofxBatchDownloader::~ofxBatchDownloader(){
	if (busy) {
		cancelBatch(false /*notify*/);
	}
}


void ofxBatchDownloader::update(){
	http.update();
}


void ofxBatchDownloader::setTimeOut(float timeOut){
	http.setTimeOut(timeOut);
}

void ofxBatchDownloader::setSpeedLimit(float KB_per_sec){
	http.setSpeedLimit(KB_per_sec);
}


void ofxBatchDownloader::cancelBatch(bool notify){
	http.stopCurrentDownload(true);
	http.waitForThread(true);
	if(notify){
		ofxBatchDownloaderReport report;
		report.owner = this;
		report.downloadPath = downloadFolder;
		report.attemptedDownloads = originalUrlList;
		report.successfulDownloads = okList;
		report.failedDownloads = failedList;
		report.responses = responses;
		report.wasCanceled = true;
		ofNotifyEvent( resourcesDownloadFinished, report, this );
	}
	busy = false;
}


void ofxBatchDownloader::setDownloadFolder(std::string f){
	downloadFolder = f;
}


void ofxBatchDownloader::draw(float x, float y){
	http.draw(x, y);
}

std::string ofxBatchDownloader::getDrawableString(){
	return http.drawableString();
}

std::string ofxBatchDownloader::getMinimalDrawableString(){
	return http.minimalDrawableString();
}

void ofxBatchDownloader::addResourcesToDownloadList( std::vector<std::string> _urlList ){
	std::vector<std::string>_checksumList;
	addResourcesToDownloadList(_urlList, _checksumList);
}

void ofxBatchDownloader::addResourcesToDownloadList( std::vector<std::string> _urlList, std::vector<std::string>_checksumList ){

	if ( _checksumList.size() > 0 && (_urlList.size() != _checksumList.size()) ){
		ofLogWarning("ofxBatchDownloader") << "addResourcesToDownloadList >> urlList & shaList element num missmatch!";
		return;
	}

	if(!busy){

		for(int i = 0; i < _urlList.size(); i++){
			originalUrlList.push_back(_urlList[i]);
			if (_checksumList.size()){
				originalChecksumList.push_back(_checksumList[i]);
			}
			ofLogVerbose("ofxBatchDownloader") << "queueing " << _urlList[i] << " for download";
		}

	}else{
		ofLogWarning("ofxBatchDownloader") << "already working, wait for it to finish!";
	}
}


int ofxBatchDownloader::pendingDownloads(){
	return http.getPendingDownloads();
}


bool ofxBatchDownloader::isBusy(){
	return busy;
};

int ofxBatchDownloader::getNumSuppliedUrls(){
	return originalUrlList.size();
}

int ofxBatchDownloader::getNumFailedUrls(){
	return failedList.size();
}

void ofxBatchDownloader::startDownloading(){

	if (!busy){
		busy = true;
		reset();

		ofLogVerbose("ofxBatchDownloader") << "starting downloads! ";

		http.setMaxQueueLength(originalUrlList.size() * 2); //just in case

		for(int i = 0; i < originalUrlList.size(); i++){
			std::string sha = "";
			if (originalChecksumList.size()){
				sha = originalChecksumList[i];
			}
			http.fetchURLToDisk(originalUrlList[i], sha, true, downloadFolder);
		}
	}
}


//this might or might not be called from the main thread! depending on the ofxSimpleHttp config
void ofxBatchDownloader::httpResult(ofxSimpleHttpResponse &r){

	//int index = okList.size() + failedList.size();

	responses.push_back(r);
	bool checkedOK = r.checksumOK || (!r.checksumOK && r.expectedChecksum.size() == 0);
	if(r.ok && checkedOK){
		okList.push_back( r.url );
		ofLogNotice("ofxBatchDownloader") << "downloaded OK [" << r.url << "]";
	}else{
		failedList.push_back( r.url );
		if (!r.checksumOK){
			ofLogError("ofxBatchDownloader") << "checksum missmatch! [" << r.url << "] expectedChecksum: " << r.expectedChecksum << ")";
		}else{
			ofLogError("ofxBatchDownloader") << "FAILED TO download [" << r.url << "]";
		}
	}

	downloadedSoFar += r.downloadedBytes;

	if (originalUrlList.size() == failedList.size() + okList.size()){
		//we are done!
		ofxBatchDownloaderReport report;
		report.owner = this;
		report.downloadPath = downloadFolder;
		report.attemptedDownloads = originalUrlList;
		report.successfulDownloads = okList;
		report.failedDownloads = failedList;
		report.responses = responses;
		ofNotifyEvent( resourcesDownloadFinished, report, this );
		busy = false;
	}
}


float ofxBatchDownloader::getAverageSpeed(){
	return http.getAvgDownloadSpeed();
}


std::vector<std::string> ofxBatchDownloader::pendingURLs(){

	std::vector<std::string> res;
	for (int i = 0; i < originalUrlList.size(); i++){
		bool found = false;
		for (int j = 0; j < okList.size(); j++){
			if ( okList[j] == originalUrlList[i] ){
				found = true;
				continue;
			}
		}
		for (int j = 0; j < failedList.size(); j++){
			if ( failedList[j] == originalUrlList[i] ){
				found = true;
				continue;
			}
		}
		if (!found) res.push_back(originalUrlList[i]);
	}
	return res;
}


void ofxBatchDownloader::reset(){
	failedList.clear();
	okList.clear();
	responses.clear();
}
