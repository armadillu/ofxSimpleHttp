#include "testApp.h"



void testApp::setup(){

	ofSetFrameRate(60);
	ofBackground(22);
	ofSetWindowPosition(20, 20);

	downloader.setNeedsChecksumMatchToSkipDownload(false); //if no checksum supplied and file exists locally, assume its ok
	downloader.setIdleTimeAfterEachDownload(0.2); //let the downloader thread sleep a bit after each download (and the notification is sent)
	downloader.setVerbose(false);
}


void testApp::downloadFinished(ofxBatchDownloaderReport &report){

	cout << "#################################################" << endl;
	cout << "#### download finished!!" << endl;
	cout << "#### [ " << report.attemptedDownloads.size() << " attempted  |  " ;

	if(report.wasCanceled){
		cout << "#### Download Was CANCELED BY USER!" << endl;
	}

	cout << report.failedDownloads.size() << " failed  |  ";
	cout << report.successfulDownloads.size() << " ok ]" << endl;
	for(int i = 0; i < report.responses.size(); i++){
		ofxSimpleHttpResponse r = report.responses[i];
		r.print();
	}

	//look through all downloads in the report, look for a good asset
	bool ok = false;
	int i = 0;
	while (!ok && i < report.responses.size()){

		if (report.responses[i].ok){
			//do something with that downloaded file!
		}
		i++;
	}
}

void testApp::update(){

	downloader.update();
}


void testApp::draw(){

	ofSetColor(255);
	drawClock();
	downloader.draw(30,30, true);

	string msg = "press 1 to download a file and supply a correct SHA1\n";
	msg += "press 2 to download a file and supply an incorrect SHA1\n";
	msg += "press 3 to download a file without supplying a SHA1\n";
	msg += "press c to cancel current download\n";
	msg += "press C to cancel all downloads\n";
	ofDrawBitmapString(msg, 20, ofGetHeight() - 70);
}


void testApp::drawClock(){

	ofPushMatrix();
	ofTranslate(ofGetWidth() - 60,60, 0);
	ofRotate( ofGetFrameNum() * 3, 0,0,1);
	ofSetColor(255,255,255);
	float h = 5;
	ofRect(-h/2,h/2, h,50);
	ofPopMatrix();
}


void testApp::keyPressed(int key){

	vector<string> allURLS;
	vector<string> allSha1s;

	// GOOD SHA1 TEST //
	if(key == '1'){
		allURLS.push_back("http://uri.cat/dontlook/localProjects/CWRU/df780aa89408ab095240921d5fa3f7e59ffab412.jpg");
		allSha1s.push_back("df780aa89408ab095240921d5fa3f7e59ffab412");
	}

	// BAD SHA1 TEST //
	if(key == '2'){
		allURLS.push_back("http://uri.cat/dontlook/localProjects/CWRU/71827399.mov");
		allSha1s.push_back("mySha1IsJustGarbageAndItShouldTriggerAnError");
	}

	// NO SHA1 TEST //
	if(key == '3'){
		allURLS.push_back("http://farm8.staticflickr.com/7420/10032530563_86ff701d19_o.jpg");
	}

	//actually start the download
	if (key >= '1' && key <= '3'){
		downloader.downloadResources(allURLS,						//list of urls to download
									 allSha1s,						//1:1 list of sha1's for those urls
									 this,							//who will be notified
									 &testApp::downloadFinished,	//callback method
									 "downloads_"					//destination folder
									 );
	}

	if(key == 'c'){
		downloader.cancelCurrentDownload();
	}

	if(key == 'C'){
		downloader.cancelAllDownloads();
	}
}

