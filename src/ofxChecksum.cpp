
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
	Poco::SHA1Engine e;
	{
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
	sprintf(buff, "%llx", hash);
	return string(buff);
}

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

char *sha1_to_hex_r(char *buffer, const unsigned char *sha1)
{
	static const char hex[] = "0123456789abcdef";
	char *buf = buffer;
	int i;

	for (i = 0; i < 20; i++) {
		unsigned int val = *sha1++;
		*buf++ = hex[val >> 4];
		*buf++ = hex[val & 0xf];
	}
	*buf = '\0';

	return buffer;
}

char*  sha1_to_hex(const unsigned char *sha1){
	static int bufno;
	static char hexbuffer[4][20 + 1];
	bufno = (bufno + 1) % ARRAY_SIZE(hexbuffer);
	return sha1_to_hex_r(hexbuffer[bufno], sha1);
}
#undef ARRAY_SIZE

std::string ofxChecksum::calcSha1(const std::string & filePath) {

	size_t const blockSize = 64 * 1024; //128 kb
	FILE * f = fopen( filePath.c_str(), "rb" );
	if(f == NULL){
		ofLogError("ofxChecksum") << "can't xxHash(); can't open file at \"" << filePath << "\"";
		return 0;
	}

	int seed = 0;
	vector<char> buf(blockSize);

	SHA1_CTX context;
	SHA1_Init(&context);

	size_t bytes_read = 0;
	do {
		bytes_read = fread(buf.data(), 1, buf.size(), f);
		if (ferror(f)) ofLogError("ofxChecksum") << "Error reading " << filePath << " for xxHash calculation.";
		SHA1_Update(&context, (uint8_t*)buf.data(), bytes_read);
	} while (bytes_read == buf.size());

	uint8_t digest[SHA1_DIGEST_SIZE];
	SHA1_Final(&context, digest);

	fclose(f);

	//convert long long to hex string
	char * bufferHash2 = sha1_to_hex(digest);
	string sha12 = bufferHash2;
	return sha12;
}

std::string ofxChecksum::toString(ofxChecksum::Type type){
	switch (type) {
		case ofxChecksum::Type::SHA1: return "SHA1";
		case ofxChecksum::Type::XX_HASH: return "xxHash";
  		default: return "Unknown Checksum Type";
	}
}