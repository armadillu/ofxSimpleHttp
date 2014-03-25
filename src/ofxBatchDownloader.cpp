//
//  ofxBatchDownloader.cpp
//  emptyExample
//
//  Created by Oriol Ferrer Mesià on 14/03/14.
//
//


#include "ofxBatchDownloader.h"

ofxBatchDownloader::ofxBatchDownloader(){

	verbose = false;
	http.setVerbose(false);
	busy = false;
	downloadFolder = "_downloads_";

	//add download listener
	ofAddListener(http.httpResponse, this, &ofxBatchDownloader::httpResult);
}


ofxBatchDownloader::~ofxBatchDownloader(){
	cancelBatch();
}


void ofxBatchDownloader::update(){
	http.update();
}


void ofxBatchDownloader::cancelBatch(){
	http.stopCurrentDownload(true);
	busy = false;
}


void ofxBatchDownloader::setDownloadFolder(string f){
	downloadFolder = f;
}


void ofxBatchDownloader::draw(float x, float y){
	http.draw(x, y);
}


void ofxBatchDownloader::addResourcesToDownloadList( vector<string> _urlList ){

	if(!busy){

		for(int i = 0; i < _urlList.size(); i++){
			originalUrlList.push_back(_urlList[i]);
			if(verbose) cout << "ofxBatchDownloader queueing " << _urlList[i] << " for download" << endl;
		}

	}else{
		cout << "ofxBatchDownloader already working, wait for it to finish!" << endl;
	}
}


int ofxBatchDownloader::pendingDownloads(){
	return http.getPendingDownloads();
}


bool ofxBatchDownloader::isBusy(){
	return busy;
};


void ofxBatchDownloader::startDownloading(){

	if (!busy){

		busy = true;
		reset();

		if(verbose) cout << "ofxBatchDownloader starting downloads! " << endl;

		for(int i = 0; i < originalUrlList.size(); i++){
			http.fetchURLToDisk(originalUrlList[i], true, downloadFolder);
		}
	}
}


//we know this will be called from the main thread!
void ofxBatchDownloader::httpResult(ofxSimpleHttpResponse &r){

	responses.push_back(r);

	if(r.ok){
		okList.push_back( r.url );
		if(verbose) cout << "ofxBatchDownloader downloaded OK " << r.url << endl;
	}else{
		failedList.push_back( r.url );
		if(verbose) cout << "ofxBatchDownloader FAILED TO download " << r.url << endl;
	}

	if (originalUrlList.size() == failedList.size() + okList.size()){
		//we are done!
		ofxBatchDownloaderReport report;
		report.downloadPath = downloadFolder;
		report.attemptedDownloads = originalUrlList;
		report.successfulDownloads = okList;
		report.failedDownloads = failedList;
		report.responses = responses;
		ofNotifyEvent( resourcesDownloadFinished, report, this );
		busy = false;
	}
}


vector<string> ofxBatchDownloader::pendingURLs(){

	vector<string> res;
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
