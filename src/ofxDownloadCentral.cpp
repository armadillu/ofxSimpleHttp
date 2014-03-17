//
//  ofxBatchDownloader.cpp
//  emptyExample
//
//  Created by Oriol Ferrer Mesià on 14/03/14.
//
//


#include "ofxBatchDownloader.h"

ofxBatchDownloader::ofxBatchDownloader(){

	verbose = true;
	http.setVerbose(false);
	busy = false;

	//add download listener
	ofAddListener(http.newResponseEvent, this, &ofxBatchDownloader::httpResult);
}


void ofxBatchDownloader::update(){
	http.update();
}

void ofxBatchDownloader::cancelDownload(){
	http.stopCurrentDownload(true);
}


void ofxBatchDownloader::draw(float x, float y){
	http.draw(x, y);
}

void ofxBatchDownloader::downloadResources( vector<string> _urlList, string downloadFolder_ ){

	if(!busy){

		downloadFolder = downloadFolder_;
		originalUrlList = _urlList;
		reset();

		for(int i = 0; i < originalUrlList.size(); i++){
			if(verbose) cout << "ofxBatchDownloader queueing " << originalUrlList[i] << " for download" << endl;
			http.fetchURLToDisk(originalUrlList[i], true, downloadFolder);
		}

	}else{
		cout << "ofxBatchDownloader already working, wait for it to finish!" << endl;
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
	}
}


void ofxBatchDownloader::reset(){
	failedList.clear();
	okList.clear();
	responses.clear();
}
