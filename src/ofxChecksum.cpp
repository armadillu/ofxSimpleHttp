
#include "ofxChecksum.h"

#include <Poco/SHA1Engine.h>
#include <Poco/DigestStream.h>

bool ofxChecksum::sha1(const string& filePath,
					   const string& sha1String,
					   bool verbose){

	float t;
	if(verbose){
 		t = ofGetElapsedTimef();
	}

	Poco::SHA1Engine sha1e;
	string localHash;
	std::ifstream ifHash(ofToDataPath(filePath, true).c_str(), std::ios::binary);
	if (ifHash.is_open()){
		Poco::DigestInputStream isSha1(sha1e, ifHash);
		const size_t bufSize = 1024 * 50; //50kb buffer
		char buf[bufSize];

		isSha1.read(buf, bufSize);
		while ( (isSha1.rdstate() & std::ifstream::failbit) == 0 ){
			isSha1.read(buf, bufSize);
		}

		if ( (isSha1.rdstate() & std::ifstream::eofbit) != 0){
			localHash = Poco::DigestEngine::digestToHex(sha1e.digest());
		}
	}

	bool match = sha1String.compare(localHash) == 0;

	float sec;
 	if(verbose){
		sec = ofGetElapsedTimef() - t;
	}

	if(verbose){
		ofLog() << "ofxChecksum::sha1(" << localHash << ") took " << sec << " secs to calc ";
	}

	return match;
}
