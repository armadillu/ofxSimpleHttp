
#include "ofxChecksum.h"
#include "hashlibpp.h"


bool ofxChecksum::sha1(string filePath, string sha1String) {

	float t = ofGetElapsedTimef();
	sha1wrapper sha1;
	string path = ofToDataPath(filePath);
	string localHash = sha1.getHashFromFile(path);
	bool match = sha1String.compare(localHash) == 0;
	//cout <<"Sha1 took " << ofGetElapsedTimef() - t << " secs to calc " << endl;
	return match;
}
