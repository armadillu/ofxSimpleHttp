#pragma once

#include "Poco/Timestamp.h"
#include "ofxChecksum.h"

class ofxSimpleHttp;

struct ofxSimpleHttpResponse{

	ofxSimpleHttp * 			who;				//who are you getting the event from?
	bool						ok;
	bool						notifyOnSuccess;	// user wants to be notified when download is ready
	bool						downloadCanceled;	// flag to cancel download
	bool						downloadToDisk;		// user wants bytes on disk; otherwise just return data as string in "responseBody"
	bool						emptyWholeQueue;	// flag
	bool						checksumOK;			// SHA1 checksum matches
	bool						fileWasHere;		// didnt even have to download the file!

	std::string				expectedChecksum;	// checksum type as defined above
	ofxChecksum::Type		checksumType = ofxChecksum::Type::SHA1;
	std::string				calculatedChecksum; //from downloaded file on disk

	int						status; 			// return code for the response ie: 200 = OK
	std::streamsize			serverReportedSize;
	std::string				reasonForStatus;	// text explaining the status
	std::string				responseBody;		// the actual response << DATA IS HERE!
	std::string				contentType;		// the mime type of the response
	Poco::Timestamp			timestamp;			// time of the response
	float					downloadProgress;	// [0..1]
	std::streamsize			downloadedSoFar;
	float					downloadSpeed;		// bytes/sec, only >0 when downloading, immediate
	bool						chunkTested;
	float					avgDownloadSpeed;	// bytes/sec, read after download happened
	std::streamsize			downloadedBytes;
	std::string				url;
	std::string				fileName;			// file + extension, no path
	std::string				extension;			// file extension (no dot)
	std::string				absolutePath;		// where file was saved
	float					timeTakenToDownload;// seconds
	float					timeDowloadStarted; //from ofGetElapsedTimef()

	std::string				customField;		//to be supplied when starting a download
											//so that you can identify the callback when you get it

	ofxSimpleHttpResponse();

	void print();
	std::string toString();
};

