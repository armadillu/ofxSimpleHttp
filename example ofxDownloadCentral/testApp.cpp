#include "testApp.h"



void testApp::setup(){

	ofSetFrameRate(63);
	ofSetVerticalSync(true);
	ofEnableAlphaBlending();
	ofBackground(22);
	ofSetWindowPosition(20, 20);


	for(int i = 0 ; i < NUM_OBJECTS; i++){
		AssetObject * o = new AssetObject();
		o->setup(&downloader);
		objects.push_back(o);
	}

}


void testApp::update(){

	downloader.update();
	for(int i = 0 ; i < NUM_OBJECTS; i++){
		objects[i]->update();
		if (ofGetFrameNum()%120 == 1){
			objects[i]->loadRandomAsset();
		}
	}
}


void testApp::draw(){

	downloader.draw(30,30);
	ofSetColor(255);

	for(int i = 0 ; i < NUM_OBJECTS; i++){
		objects[i]->draw();
	}

	//clock hand to see threading in action
	ofPushMatrix();

	ofTranslate(ofGetWidth() - 60,60, 0);
	ofRotate( ofGetFrameNum() * 3, 0,0,1);
    ofSetColor(255,255,255);
	float h = 5;
	ofRect(-h/2,h/2, h,50);

	ofPopMatrix();
}


void testApp::keyPressed(int key){

	if(key=='c'){
		//downloader.
		downloader.cancelCurrentDownload();
	}


	if(key=='C'){
		downloader.cancelAllDownloads();
	}

	if(key=='r'){
		for(int i = 0 ; i < NUM_OBJECTS; i++){
			objects[i]->loadRandomAsset();
		}
	}
}

