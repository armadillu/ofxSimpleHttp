#include "ofApp.h"



void ofApp::setup(){

	ofSetFrameRate(63);
	ofSetVerticalSync(true);
	ofEnableAlphaBlending();
	ofBackground(22);
	ofSetWindowPosition(20, 20);

	downloadList.push_back("http://farm8.staticflickr.com/7420/10032530563_86ff701d19_o.jpg");
	downloadList.push_back("http://farm4.staticflickr.com/3686/9225463176_d0bf83a992_o.jpg");
	downloadList.push_back("http://farm8.staticflickr.com/7255/6888724266_158ce261a2_o.jpg");
	downloadList.push_back("http://farm8.staticflickr.com/7047/7034809565_5f80871bff_o.jpg");
	downloadList.push_back("http://farm8.staticflickr.com/7438/9481688475_e83f92e8b5_o.jpg");
	downloadList.push_back("http://farm8.staticflickr.com/7321/9481647489_e73bed28e1_o.jpg");
	downloadList.push_back("http://farm8.staticflickr.com/7367/9484432454_9701453c66_o.jpg");
	downloadList.push_back("http://farm6.staticflickr.com/5537/9481654243_7b73b87ceb_o.jpg");
	downloadList.push_back("http://farm4.staticflickr.com/3740/9481659071_3159d318dc_o.jpg");
	downloadList.push_back("http://farm4.staticflickr.com/3802/9484323300_6d3a6a78b5_o.jpg");
	downloadList.push_back("http://farm6.staticflickr.com/5346/9484309488_11ee39298e_o.jpg");

	//add download listener
	ofAddListener(downloader.resourcesDownloadFinished, this, &ofApp::downloadFinished);
	downloader.setDownloadFolder("tempDownloads");
	downloader.setNeedsChecksumMatchToSkipDownload(true);
	downloader.setIdleTimeAfterEachDownload(0.2);
	downloader.setVerbose(false);

}


void ofApp::update(){

	downloader.update();
}

void ofApp::draw(){

	downloader.draw(30,30);
	ofSetColor(255);

	//clock hand to see threading in action
	ofPushMatrix();

	ofTranslate(ofGetWidth() - 60,60, 0);
	ofRotate( ofGetFrameNum() * 3, 0,0,1);
    ofSetColor(255,255,255);
	float h = 5;
	ofRect(-h/2,h/2, h,50);

	ofPopMatrix();
}


void ofApp::keyPressed(int key){

	if(key=='a'){
		downloader.addResourcesToDownloadList(downloadList);
	}

	if(key==' '){
		downloader.startDownloading();
	}

	if(key=='c'){
		downloader.cancelBatch();
	}
}


void ofApp::downloadFinished(ofxBatchDownloaderReport &report){

	cout << "download Finished!" << endl;
	cout << report.successfulDownloads.size() << " successful downloads, " <<  report.failedDownloads.size() << " failed downloads." << endl;

	if( report.failedDownloads.size() ){
		cout << "these downloads failed: " ;
		for(int i = 0; i < report.failedDownloads.size(); i++ ){
			cout << " - " << report.failedDownloads[i] << endl;
		}
	}

}