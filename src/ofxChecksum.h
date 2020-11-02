
#pragma once

#include "ofMain.h"

class ofxChecksum{
public:

	enum class Type{
		SHA1,
		XX_HASH, //xxhash64
		XX_HASH3_64,
		XX_HASH3_128
	};

	//checks if sha1 match between file & expected sha1
	static bool sha1(const std::string & filePath,
					 const std::string & sha1String,
					 bool verbose = false);

	//calc sha1 of a file on disk - note that this is ~2.5x slower than the calcSha1() method
	//so u should use the other one calcSha1()
	static std::string calcSha1_poco(const std::string & filePath);

	//calc sha1 of a file on disk
	static std::string calcSha1(const std::string & filePath);

	//calc sha1 of a std::string
	static std::string calcSha1FromString(const std::string & data);

	//muuuuch faster than sha1 checksum, xxhash64, need to supply absolute path
	static std::string xxHash(const std::string & filePath); //xxhash64
	static std::string xxHash3_64(const std::string & filePath);
	static std::string xxHash3_128(const std::string & filePath);

	static string toString(Type t);
};

