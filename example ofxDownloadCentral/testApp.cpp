#include "testApp.h"

#include <OpenGL/OpenGL.h>

void testApp::setup(){

	ofSetFrameRate(63);
	ofSetVerticalSync(true);
	ofEnableAlphaBlending();
	ofBackground(22);
	ofSetWindowPosition(20, 20);

	downloader.setNeedsChecksumMatchToSkipDownload(false);
	downloader.setIdleTimeAfterEachDownload(0.2);
	downloader.setVerbose(false);

//	#ifdef TARGET_OSX
//    // Enable the OPEN_GL multi-threading, not sure what this really does! TODO!
//    CGLError err;
//    CGLContextObj ctx = CGLGetCurrentContext();
//    err =  CGLEnable( ctx, kCGLCEMPEngine);
//	#endif

	for(int i = 0 ; i < NUM_OBJECTS; i++){
		AssetObject * o = new AssetObject();
		o->setup(&downloader);
		objects.push_back(o);
	}

	//ofSetDrawBitmapMode(OF_BITMAPMODE_MODEL);
	cam.setDistance(800);

	TIME_SAMPLE_SET_FRAMERATE(60);
	TIME_SAMPLE_SET_AVERAGE_RATE(0.1);

	frameTimePlot = new ofxHistoryPlot( NULL, "frameTime", 1000, false); //NULL cos we don't want it to auto-update. confirmed by "true"
	frameTimePlot->setRange(0, 16.0);
	frameTimePlot->setColor( ofColor(255,0,0) );
	frameTimePlot->setShowNumericalInfo(true);
	frameTimePlot->setRespectBorders(true);
	frameTimePlot->setLineWidth(1.5);
	frameTimePlot->setBackgroundColor(ofColor(0, 64));
	numAssetLoads = 0;
	autoAlloc = false;
}


void testApp::update(){

	downloader.update();

	for(int i = 0 ; i < NUM_OBJECTS; i++){

		objects[i]->update();

		if(autoAlloc){
			if ( 1 == ofGetFrameNum()%(int)(30 + ofRandom(60)) ){ //load a random asset every now and then
				objects[i]->loadRandomAsset();
				numAssetLoads++;
			}
		}
	}

	float ms = TIME_SAMPLE_GET_LAST_DURATION("draw()");
	frameTimePlot->update(ms);
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

	drawClock();

	downloader.draw(30,30, true);
	ofSetColor(255);

	float plotH = 125;
	frameTimePlot->draw(0, ofGetHeight() - plotH, ofGetWidth(), plotH);

	string dbg =	"num vid players: " + ofToString(ofxThreadedVideoPlayer::getNumInstances()) + "\n" +
					"total loaded: " + ofToString(numAssetLoads) + "\n" +
					"autoAlloc : " + ofToString(autoAlloc);

	ofDrawBitmapString(dbg, ofGetWidth() - 200, 50);
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
			numAssetLoads++;
		}
	}

	if(key=='u'){
		for(int i = 0 ; i < NUM_OBJECTS; i++){
			objects[i]->unloadAssets();
		}
	}

	if (key==' '){
		autoAlloc = !autoAlloc;
	}
}

