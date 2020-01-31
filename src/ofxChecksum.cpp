
#include "ofxChecksum.h"

#include <Poco/SHA1Engine.h>
#include <Poco/DigestStream.h>

#include "../libs/xxHash/xxhash.h"
#include "../libs/sha1/sha1.h"

//#include "ofxTimeMeasurements.h"
//extern "C" {
//#include "sha1.h"
//}


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


std::string ofxChecksum::calcSha1_poco(const std::string & filePath){

//	TS_START_NIF("sha1_1");
	Poco::SHA1Engine e;{
		auto f = fopen(filePath.c_str(), "rb");
		if(f == NULL){
			ofLogError("ofxChecksum") << "can't calcSha1(); can't open file at \"" << filePath << "\"";
			return "";
		}
		vector<char> buf(1024 * 1024);
		size_t bytes_read = 0;
		do {
			bytes_read = fread(buf.data(), 1, buf.size(), f);
			if (ferror(f)) ofLogError("ofxChecksum") << "Error reading " << filePath << " for SHA calculation.";
			e.update(buf.data(), bytes_read);
		} while (bytes_read == buf.size());
		fclose(f);
	}
	return Poco::DigestEngine::digestToHex(e.digest());
}


std::string ofxChecksum::calcSha1FromString(const std::string & data){
	Poco::SHA1Engine e;
	e.update(data);
	return Poco::DigestEngine::digestToHex(e.digest());
}


std::string ofxChecksum::xxHash(const std::string & filePath) {

	size_t const blockSize = 64 * 1024; //128 kb
	FILE * f = fopen( filePath.c_str(), "rb" );
	if(f == NULL){
		ofLogError("ofxChecksum") << "can't xxHash(); can't open file at \"" << filePath << "\"";
		return 0;
	}

	float t = ofGetElapsedTimef();

	int seed = 0;
	vector<char> buf(blockSize);

	XXH64_state_t* const state = XXH64_createState();

	XXH_errorcode const resetResult = XXH64_reset(state, seed);
	if (resetResult == XXH_ERROR) abort();

	size_t bytes_read = 0;
	do {
		bytes_read = fread(buf.data(), 1, buf.size(), f);
		if (ferror(f)) ofLogError("ofxChecksum") << "Error reading " << filePath << " for xxHash calculation.";
		XXH_errorcode const addResult = XXH64_update(state, buf.data(), bytes_read);
		if (addResult == XXH_ERROR) abort();
	} while (bytes_read == buf.size());

	unsigned long long const hash = XXH64_digest(state);

	XXH64_freeState(state);
	fclose(f);

	//convert long long to hex string
	char buff[64];
	sprintf(buff, "%016llx", hash); //output a 16 char hash string, note it will have leading zeroes to reach 16 characters
	float totalTime = ofGetElapsedTimef() - t;
	if(totalTime > 3.0){
		ofLogNotice("ofxChecksum") << "xxHash() of \"" << filePath << "\" is \"" << buff << "\" and it took " << ofGetElapsedTimef() - t << " seconds.";
	}
	return string(buff);
}


void digest_to_hex(const uint8_t digest[SHA1_DIGEST_SIZE], char *output){
	int i,j;
	char *c = output;

	for (i = 0; i < SHA1_DIGEST_SIZE/4; i++) {
		for (j = 0; j < 4; j++) {
			sprintf(c,"%02X", digest[i*4+j]);
			c += 2;
		}
		sprintf(c, " ");
		c += 1;
	}
	*(c - 1) = '\0';
}


std::string ofxChecksum::calcSha1(const std::string & filePath) {

	size_t const blockSize = 64 * 1024; //128 kb
	FILE * f = fopen( filePath.c_str(), "rb" );
	if(f == NULL){
		ofLogError("ofxChecksum") << "can't calcSha1(); can't open file at \"" << filePath << "\"";
		return "";
	}

	vector<char> buf(blockSize);

	SHA1_CTX context;
	SHA1_Init(&context);

	size_t bytes_read = 0;
	do {
		bytes_read = fread(buf.data(), 1, buf.size(), f);
		if (ferror(f)) ofLogError("ofxChecksum") << "Error reading " << filePath << " for calcSha1 calculation.";
		SHA1_Update(&context, (uint8_t*)buf.data(), bytes_read);
	} while (bytes_read == buf.size());

	uint8_t digest[SHA1_DIGEST_SIZE];
	SHA1_Final(&context, digest);

	fclose(f);

	char output[80];
	digest_to_hex(digest, output);
	return string(output);
}

std::string ofxChecksum::toString(ofxChecksum::Type type){
	switch (type) {
		case ofxChecksum::Type::SHA1: return "SHA1";
		case ofxChecksum::Type::XX_HASH: return "xxHash";
  		default: return "Unknown Checksum Type";
	}
}
