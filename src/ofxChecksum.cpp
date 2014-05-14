
#include "ofxChecksum.h"
#include "hashlibpp.h"


bool ofxChecksum::sha1(string filePath, string sha1String, bool verbose ) {

	float t = ofGetElapsedTimef();
	sha1wrapper sha1;
	string path = ofToDataPath(filePath);
	string localHash = sha1.getHashFromFile(path);
	bool match = sha1String.compare(localHash) == 0;
	float sec = ofGetElapsedTimef() - t;
	if(sec > 0.5 && verbose){
		ofLogVerbose() << "ofxChecksum::sha1() took " << sec << " secs to calc " << endl;
	}
	return match;
}
