#include "ofApp.h"



void ofApp::setup(){

	ofSetFrameRate(60);
	ofBackground(22);
	ofSetWindowPosition(20, 20);

	downloader.setNeedsChecksumMatchToSkipDownload(true); //if no checksum supplied and file exists locally, assume its ok
	downloader.setIdleTimeAfterEachDownload(0.2); //let the downloader thread sleep a bit after each download (and the notification is sent)
	downloader.setVerbose(false);
	downloader.setMaxConcurrentDownloads(3);
	//downloader.setSpeedLimit(50.0f); //kb per seconds - to slow it down and test!
}


void ofApp::downloadFinished(ofxBatchDownloaderReport &report){

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

void ofApp::update(){

	downloader.update();
}


void ofApp::draw(){

	ofSetColor(255);
	drawClock();
	downloader.draw(30,30, true, false);

	string msg = "press 1 to download a file and supply a correct SHA1\n";
	msg += "press 2 to download a file and supply an incorrect SHA1\n";
	msg += "press 3 to download a few files without supplying a SHA1\n";
	msg += "press c to cancel current download\n";
	msg += "press C to cancel all downloads\n";
	ofDrawBitmapString(msg, 20, ofGetHeight() - 70);
}


void ofApp::drawClock(){

	ofPushMatrix();
	ofTranslate(ofGetWidth() - 60,60, 0);
	ofRotate( ofGetFrameNum() * 3, 0,0,1);
	ofSetColor(255,255,255);
	float h = 5;
	ofRect(-h/2,h/2, h,50);
	ofPopMatrix();
}


void ofApp::keyPressed(int key){

	vector<string> allURLS;
	vector<string> allSha1s;

	// GOOD SHA1 TEST //
	if(key == '1'){
		allURLS.push_back("http://farm3.staticflickr.com/2875/9481775605_ea43f5d4f3_o_d.jpg");
		allSha1s.push_back("852a7952aabcbf3479974d5350f973b005b23c4a");
	}

	// BAD SHA1 TEST //
	if(key == '2'){
		allURLS.push_back("http://farm8.staticflickr.com/7454/9481666291_40f7c00b80_o_d.jpg");
		allSha1s.push_back("my_Sha1_Is_Garbage_And_It_Should_Trigger_An_Error");
	}

	// NO SHA1 TEST //
	if(key == '3'){
		allURLS.push_back("http://farm8.staticflickr.com/7420/10032530563_86ff701d19_o.jpg");
		allURLS.push_back("http://farm3.staticflickr.com/2877/9481432451_c74c649515_o_d.jpg");
		allURLS.push_back("http://farm4.staticflickr.com/3702/9484220536_8f5a866b4d_o_d.jpg");
		allURLS.push_back("http://farm4.staticflickr.com/3667/9484488930_2c6527ee35_o_d.jpg");
		allURLS.push_back("http://farm3.staticflickr.com/2857/9481682413_5f251ba22d_o_d.jpg");
		allURLS.push_back("http://farm8.staticflickr.com/7438/9481688475_e83f92e8b5_o_d.jpg");
	}

	//actually start the download
	if (key >= '1' && key <= '2'){
		downloader.downloadResources(allURLS,						//list of urls to download
									 allSha1s,						//1:1 list of sha1's for those urls
									 this,							//who will be notified
									 &ofApp::downloadFinished,	//callback method
									 "downloads_"					//destination folder
									 );
		downloader.startDownloading();
	}else
	if(key == '3'){ //special case for long list, lets add each URL as a different batch (to test paralelizing)
		for(int i = 0; i < allURLS.size(); i++){
			downloader.downloadResources(allURLS[i],						//list of urls to download
										 this,							//who will be notified
										 &ofApp::downloadFinished,	//callback method
										 "downloads_"					//destination folder
										 );
		}
		downloader.startDownloading();
	}


	if(key == 'c'){
		downloader.cancelCurrentDownload();
	}

	if(key == 'C'){
		downloader.cancelAllDownloads();
	}
}

