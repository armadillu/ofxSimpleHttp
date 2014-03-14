#include "testApp.h"



void testApp::setup(){

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

	http.setVerbose(true);

	//add download listener
	ofAddListener(http.newResponseEvent, this, &testApp::newResponse);
}


void testApp::update(){

	http.update();
}

void testApp::draw(){
	http.draw(30,30);
	ofSetColor(255);

	//clock hand to see threading in action
	ofPushMatrix();
	ofTranslate(ofGetWidth()/2, ofGetHeight()/2);
	ofRotate(360 * ofGetElapsedTimef(), 0, 0, 1);
	ofTriangle(10, 0, -10, 0, 0, 200);
	ofPopMatrix();

	//instructions
	ofDrawBitmapStringHighlight("press '1' to download asset to disk on a bg thread\n"
								"press '2' to download multiple assets to disk on a bg thread\n"
								"press '3' to download asset on main thread\n"
								"press '4' to download multiple assets on main thread"
								,
								20,
								ofGetHeight() - 60
								);


}


void testApp::keyPressed(int key){

	string outputDirectory = "tempDownloads";

	if(key=='1'){
		string url = downloadList[floor(ofRandom(downloadList.size()))];
		http.fetchURLToDisk(url , true/*notify when done*/, outputDirectory);
	}


	if(key=='2'){
		for(int i=0 ; i < downloadList.size(); i++){
			http.fetchURLToDisk(downloadList[i], true/*notify when done*/, outputDirectory );
		}
	}

	if(key=='3'){
		string url = downloadList[floor(ofRandom(downloadList.size()))];
		http.fetchURLtoDiskBlocking(url, outputDirectory);
	}

	if(key=='4'){
		for(int i=0 ; i < downloadList.size(); i++){
			http.fetchURLtoDiskBlocking(downloadList[i], outputDirectory );
		}
	}

}


void testApp::newResponse(ofxSimpleHttpResponse &r){

	cout << "download of " << r.url << " returned : "<< string(r.ok ? "OK" : "KO") << endl;
	cout << "server reported size is " << r.serverReportedSize << endl;
	cout << "server status is " << r.status << endl;
	cout << "file content type is " << r.contentType << endl;
	cout << "file name is " << r.fileName << endl;

	if(r.downloadToDisk){
		cout << "file was saved to " << r.absolutePath << endl;

		//move the file to wherever you need..
		ofDirectory dir;
		dir.createDirectory("mySortedDownloads");

		ofDirectory f;
		f.open( r.absolutePath );
		if( f.exists() ){
			f.moveTo("mySortedDownloads/" + r.fileName);
		}else{
			cout << "file was not downloaded???!" << r.absolutePath << endl;
		}
	}

}
