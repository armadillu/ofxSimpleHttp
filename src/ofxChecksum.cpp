
#include "ofxChecksum.h"

#include <Poco/SHA1Engine.h>
#include <Poco/DigestStream.h>

bool ofxChecksum::sha1(const std::string& filePath,
					   const std::string& sha1String,
					   bool verbose){

	float t;
	if(verbose){
 		t = ofGetElapsedTimef();
	}

	Poco::SHA1Engine sha1e;
	std::string localHash = calcSha1(filePath);
	bool match = sha1String.compare(localHash) == 0;

 	if(verbose){
		t = ofGetElapsedTimef() - t;
		ofLogNotice("ofxChecksum") << "ofxChecksum::sha1(" << localHash << ") took " << t << " secs to calc.";
	}
	return match;
}


std::string ofxChecksum::calcSha1(const std::string & filePath){

	Poco::SHA1Engine e;
	auto f = fopen(filePath.c_str(), "rb");
	if(f == NULL){
		ofLogError("ofxChecksum") << "can't calcSha1(); can't open file at \"" << filePath << "\"";
		return "";
	}
	vector<char> buf(10 * 1024 * 1024);
	size_t bytes_read;
	do {
		bytes_read = fread(buf.data(), 1, buf.size(), f);
		if (ferror(f)) ofLogError("ofxChecksum") << "Error reading " << filePath << " for SHA calculation.";
		e.update(buf.data(), bytes_read);
	} while (bytes_read == buf.size());
	fclose(f);
	return Poco::DigestEngine::digestToHex(e.digest());
}
