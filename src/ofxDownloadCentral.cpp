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
	//downloader.update();

	if (busy){

		if(downloaders.size() > 0){
			ofxBatchDownloader * bd = downloaders[0];
			bd->update();
			if (!bd->isBusy()){ //this one is over! start the next one!
				//ofRemoveEvent???
				downloaders.erase(downloaders.begin());
				delete bd;
				busy = false;
				startQueue();
			}
		}
	}
}

void ofxDownloadCentral::cancelDownload(){
	//downloader.ca(true);
}


//void ofxDownloadCentral::downloadFinished(ofxBatchDownloaderReport &report){
//	//ofxDownloadCentralReport r;
//	//ofNotifyEvent( resourcesDownloadFinished, r, this );
//}

void ofxDownloadCentral::draw(float x, float y){

	if (busy){

		if(downloaders.size() > 0){
			ofxBatchDownloader * bd = downloaders[0];
			bd->draw(20, 20);
		}
	}
}


//void ofxDownloadCentral::downloadResources( vector<string> _urlList, string dlFolder ){
//
//	//add download listener
//	ofxBatchDownloader * d = new ofxBatchDownloader();
//
////	ofAddListener(downloaders.resourcesDownloadFinished, this, &ofxDownloadCentral::downloadFinished);
////
////	downloaders.downloadResources(_urlList);
//
//}

//template <typename ArgumentsType, class ListenerClass>
//void ofxDownloadCentral::downloadResources(	vector<string>urlList,
//											//EventType & event,
//											ListenerClass  * listener,
//											void (ListenerClass::*listenerMethod)(ArgumentsType&)
//										){
//
//
//	ofxBatchDownloader * d = new ofxBatchDownloader();
//	ofAddListener(d->resourcesDownloadFinished, listener, listenerMethod); //set the notification to hit our original caller
//	downloaders.push_back(d);
//
//}
