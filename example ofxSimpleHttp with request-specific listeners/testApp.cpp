#include "ofMain.h"
#include "ofxSimpleHttp.h"

//========================================================================
// testApp.h

class testApp : public ofBaseApp{

public:
    void setup();
    void update();
    void draw();

    void keyPressed(int key);

    ofxSimpleHttp http;
    string theOneUrl;
    string feedback;

    // response callbacks
    void onResponseLogStatus( ofxSimpleHttpResponse & response );
    void onFailureWheep( ofxSimpleHttpResponse & response );
};


//========================================================================
// testApp.cpp

void testApp::setup(){

    ofxSimpleHttp::createSslContext();
    theOneUrl = "https://raw.githubusercontent.com/armadillu/ofxSimpleHttp/master/ofxaddons_thumbnail.png";
    feedback = "";

    http.addCustomHttpHeader("Accept", "application/json"); //you can supply custom headers if you need to
    http.setCopyBufferSize(16);
    http.setSpeedLimit(100);
}


void testApp::update(){
    http.update();
}

void testApp::draw(){
    http.draw(30,30);
    ofSetColor(255);

    //clock hand to see threading in action
    ofPushMatrix();
    ofTranslate(ofGetWidth() - 60,60, 0);
    ofRotate( ofGetFrameNum() * 3, 0,0,1);
    ofSetColor(255,255,255);
    float h = 5;
    ofRect(-h/2,h/2, h,50);
    ofPopMatrix();

    //instructions
    ofDrawBitmapStringHighlight("press '1', '2', '3' or '4' and see what happens\n",
                                20,
                                ofGetHeight() - 100);
    ofDrawBitmapStringHighlight(feedback, 20, ofGetHeight() - 70);
}


void testApp::keyPressed(int key){

    // fetch url and register listener for a response to this specific request

    if(key=='1'){
        auto notifier = http.fetchURL(theOneUrl , true/*notify when done*/);
        if(notifier){
            ofAddListener(notifier->responseEvent, this, &testApp::onResponseLogStatus);
        }
    }

    // fetch url and register inline lambda success-callback

    if(key=='2'){
        auto notifier = http.fetchURL(theOneUrl , true/*notify when done*/);
        if(notifier){
            notifier->onSuccess([this](ofxSimpleHttpResponse & response){
                feedback = response.url + " gave us\n" + ofToString(response.serverReportedSize) + " bytes of data";
            });
        }
    }

    // separate lambda callbacks for both success and failure responses

    if(key=='3'){
        auto notifier = http.fetchURLToDisk(theOneUrl , true/*notify when done*/);
        if(notifier){
            notifier->onSuccess([this](ofxSimpleHttpResponse & response){
                feedback = response.url + " downloaded to:\n" + response.absolutePath;
            });

            notifier->onFailure([this](ofxSimpleHttpResponse & response){
                feedback = response.url + " failed.\nStatus code: " + ofToString(response.status) + "\nMessage:\n" + response.responseBody;
            });
        }
    }

    // they can't all be winners

    if(key=='4'){
        auto notifier = http.fetchURLToDisk("http://localhost:4321/are/you/_really_/running/this/server?#!" , true/*notify when done*/);
        if(notifier){
            ofAddListener(notifier->failureEvent, this, &testApp::onFailureWheep);
        }
    }
}

void testApp::onResponseLogStatus( ofxSimpleHttpResponse & response ){
    feedback = "HTTP request to " + response.url + "\nresponded with status code: " + ofToString(response.status);
}

void testApp::onFailureWheep( ofxSimpleHttpResponse & response ){
    feedback = "You requested " + response.url + ".\nThat was never gonna work, now was it?";

    if(response.status == 200){
        feedback += "\nWait, what?! This is impossible. (Literally, it is.)";
    }
}


//========================================================================
// main.cpp

#include "ofAppGLFWWindow.h"

int main( ){
    ofSetupOpenGL(800,500, OF_WINDOW);
    ofRunApp(new testApp());
}
