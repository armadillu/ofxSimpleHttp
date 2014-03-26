#include "testApp.h"

#include <OpenGL/OpenGL.h>

void testApp::setup(){

	ofSetFrameRate(63);
	ofSetVerticalSync(true);
	ofEnableAlphaBlending();
	ofBackground(22);
	ofSetWindowPosition(20, 20);

	downloader.setNeedsChecksumMatchToSkipDownload(false);
	downloader.setVerbose(false);

	#ifdef TARGET_OSX
//    // Enable the OPEN_GL multi-threading, not sure what this really does! TODO!
//    CGLError err;
//    CGLContextObj ctx = CGLGetCurrentContext();
//    err =  CGLEnable( ctx, kCGLCEMPEngine);
	#endif

	for(int i = 0 ; i < NUM_OBJECTS; i++){
		AssetObject * o = new AssetObject();
		o->setup(&downloader);
		objects.push_back(o);
	}

	//ofSetDrawBitmapMode(OF_BITMAPMODE_MODEL);
	cam.setDistance(1100);

}


void testApp::update(){

	downloader.update();
	for(int i = 0 ; i < NUM_OBJECTS; i++){
		objects[i]->update();

		if (ofGetFrameNum()%120 == 1){ //load a random asset every now and then
			if(ofRandom(1) > 0.5){
				objects[i]->loadRandomAsset();
			}
		}
	}
}


void testApp::draw(){

	ofSetColor(255);

	cam.begin();
	ofEnableDepthTest();
	for(int i = 0 ; i < NUM_OBJECTS; i++){
		objects[i]->draw();
	}
	ofDisableDepthTest();
	cam.end();

	//clock hand to see threading in action
	ofPushMatrix();
	ofTranslate(ofGetWidth() - 60,60, 0);
	ofRotate( ofGetFrameNum() * 3, 0,0,1);
	ofSetColor(255,255,255);
	float h = 5;
	ofRect(-h/2,h/2, h,50);
	ofPopMatrix();

	downloader.draw(30,30, true);
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

