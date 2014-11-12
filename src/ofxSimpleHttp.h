/*
 *  ofxSimpleHttp.h
 *  emptyExample
 *
 *  Created by Oriol Ferrer Mesià on 06/11/10.
 *  Copyright 2010 uri.cat. All rights reserved.
 *
 
 HOW TO

 1 - Instantiate in your project:
 
	ofxSimpleHttp		http;
 
 
 2 - implement http response method:
 
	void testApp::newResponse(ofxSimpleHttpResponse &response){
		printf("download of '%s' returned : %s\n", response.url.c_str(), response.ok ? "OK" : "KO" );
	}
 
 3 - add Listener:
 
	ofAddListener(http.httpResponse, this, &testApp::newResponse);
 
 4 - fetch URLs:

	blocking:		ofxSimpleHttpResponse response = http.fetchURLBlocking("http://uri.cat/");
	non-blocking:	ofxSimpleHttpResponse response = http.fetchURL("http://uri.cat/"); 
	
	//you can queue several non-bloking requests and they will download one after each other
 
 5 - Recieve data when available:
	
	your newResponse(...) method will be called when the download is available.
 
 */

#include <Poco/SHA1Engine.h>

#pragma once

#include "ofMain.h"
#include "ofEvents.h"
#include <queue>
#include <stdio.h>
#include "Poco/Net/HTTPClientSession.h"
#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPResponse.h"
#include "poco/Net/HTTPStreamFactory.h"
#include "Poco/Net/HTTPSStreamFactory.h"
#include "Poco/Net/HTTPSClientSession.h"
#include "Poco/Buffer.h"
#include "ofxChecksum.h"
#include "Poco/Net/SSLManager.h"
#include "Poco/Net/KeyConsoleHandler.h"
#include "Poco/Net/ConsoleCertificateHandler.h"
#include "Poco/Net/NetException.h"
#include "Poco/StreamCopier.h"
#include "Poco/Path.h"
#include "Poco/URI.h"
#include "Poco/Exception.h"
#include <iostream>

#define OFX_SIMPLEHTTP_UNTITLED_FILENAME	"unnamed.file"

using namespace Poco::Net;
using namespace Poco;
using Poco::Exception;
using Poco::Net::HTTPClientSession;

class ofxSimpleHttp;

struct ofxSimpleHttpResponse{

	
	ofxSimpleHttp * 			who;
	bool						ok;
	bool						notifyOnSuccess;	// user wants to be notified when download is ready
	bool						downloadCanceled;	// flag to cancel download
	bool						downloadToDisk;		// user wants bytes on disk; otherwise just return data as string in "responseBody"
	bool						emptyWholeQueue;	// flag
	bool						checksumOK;			// SHA1 checksum matches
	bool						fileWasHere;		// didnt even have to download the file!
	string						expectedChecksum;	// sha1
	int							status; 			// return code for the response ie: 200 = OK
	long int					serverReportedSize;
	string						reasonForStatus;	// text explaining the status
	string						responseBody;		// the actual response << DATA IS HERE!
	string						contentType;		// the mime type of the response
	Poco::Timestamp				timestamp;			// time of the response
	float						downloadProgress;	// [0..1]
	std::streamsize				downloadedSoFar;
	float						downloadSpeed;		// bytes/sec, only >0 when downloading, immediate
	float						avgDownloadSpeed;	// bytes/sec, read after download happened
	long int					downloadedBytes;
	string						url;
	string						fileName;			// file + extension, no path
	string						extension;			// file extension (no dot)
	string						absolutePath;		// where file was saved
	float						timeTakenToDownload;// seconds
	float						timeDowloadStarted; //from ofGetElapsedTimef()

	ofxSimpleHttpResponse(){
		who = NULL;
		downloadToDisk = emptyWholeQueue = false;
		checksumOK = true;
		downloadProgress = downloadSpeed = avgDownloadSpeed = downloadedBytes = 0.0f;
		timeTakenToDownload = 0.0;
		serverReportedSize = -1;
		downloadedSoFar = 0;
		status = -1;
		fileWasHere = false;
		timeDowloadStarted = ofGetElapsedTimef();
	}

	void print(){
		ofLogNotice() << "#### " << url;
		if (ok){
			if (fileWasHere){
				ofLogNotice() << "    File was already on disk, no download needed!";
				if (expectedChecksum.size()){
					ofLogNotice() << "    File checksum " << expectedChecksum << " matched!";
				}else{
					ofLogNotice() << "    File checksum not supplied, assuming file is the same blindly";
				}
				ofLogNotice() << "    File saved at: " << absolutePath;
			}else{
				ofLogNotice() << "    Server Status: " << status;
				ofLogNotice() << "    Server Reported size: " << serverReportedSize;
				ofLogNotice() << "    Content Type: " << contentType;
				if(expectedChecksum.length()){
					ofLogNotice() << "    Expected Checksum: " << expectedChecksum;
					ofLogNotice() << "    Checksum Match: " << string(checksumOK ? "YES" : "NO");
				}
				ofLogNotice() << "    Download Time taken: " << timeTakenToDownload << " seconds";
				if (serverReportedSize != -1){
					ofLogNotice() << "    Avg Download Speed: " << (serverReportedSize / 1024.f) / timeTakenToDownload << "Kb/sec";
				}
				if(downloadToDisk){
					ofLogNotice() << "    File Saved at: " << absolutePath;
				}
			}
		}else{
			ofLogError() << "    Download FAILED! ";
			ofLogError() << "    Status: " << status << " - " << reasonForStatus;
		}
		ofLogNotice() << endl;
	}
};


class ofxSimpleHttp : public ofThread{
	
	public:

		ofxSimpleHttp();
		~ofxSimpleHttp();

		// actions //////////////////////////////////////////////////////////////

		//download to RAM ( download to ofxSimpleHttpResponse->responseBody)
		void						fetchURL(string url, bool notifyOnSuccess = false);
		ofxSimpleHttpResponse		fetchURLBlocking(string url);

		//download to Disk
		void						fetchURLToDisk(string url, bool notifyOnSuccess = false, string outputDir = ".");
		void						fetchURLToDisk(string url, string expectedSha1, bool notifyOnSuccess = false, string outputDir = ".");
		ofxSimpleHttpResponse		fetchURLtoDiskBlocking(string url, string outputDir = ".", string expectedSha1 = "");

		void						update(); //this is mainly used to get notifications in the main thread

		void						draw(float x, float y , float w , float h);	//draws a box
		void						draw(float x, float y);	//draws a box
		string						drawableString();

	void						drawMinimal(float x, float y, bool withBg = false, ofColor fontColor = ofColor::white,
											ofColor bgColor = ofColor::black);	//draws a one-line status
		string						minimalDrawableString();

		float getHeight() { if ( isThreadRunning() ) return 18 * 4; else return 18; } //TODO!
		float getWidth() { return 320; }
	
		void						stopCurrentDownload(bool emptyQueue); //if there's more downloads on queue, next will start immediatelly

		int							getPendingDownloads();
		float						getCurrentDownloadProgress();	//retuns [0..1] how complete is the download
		string						getCurrentDownloadFileName();	//only while downloading
		float						getCurrentDownloadSpeed();		// kb/sec - only whilde downloading will be > 0!
		float						getAvgDownloadSpeed();			// kb/sec - also when not download

		// properties //////////////////////////////////////////////////////////
	
		void						setTimeOut(int seconds);
		void						setVerbose(bool verbose); //unused
		void						setUserAgent( string newUserAgent );
		void						setMaxQueueLength(int len);
		void 						setCopyBufferSize(int KB); /*in KiloBytes (1 -> 1024 bytes)*/
		void						setIdleTimeAfterEachDownload(float seconds); //wait a bit before notifying once the dowload is over
		void						setCancelCurrentDownloadOnDestruction(bool doIt);
		void						addCustomHttpHeader(string headerName, string headerContent);
		void						setNotifyFromMainThread(bool mainThread);

									//if a download is requested and file already exists on disk,
									//do we require a checksum match to assume the file is good?
									//ie, if the user doesnt provide a checksum, what do we do?
		void						setNeedsChecksumMatchToSkipDownload(bool needs);

		ofEvent<ofxSimpleHttpResponse>		httpResponse;

		static string				bytesToHumanReadable(long long bytes, int decimalPrecision);
		static string				secondsToHumanReadable(float sec, int decimalPrecision);

		// https support //////////////////////////////////////////////////////////////
		static void 				createSslContext(); //call once, before any https connection is made
		static void 				destroySslContext(); //call once when no longer need https, once all trasnfers are finished
														//or just b4 app exit

	private:
		
		bool downloadURL( ofxSimpleHttpResponse * resp, bool sendResultThroughEvents, bool beingCalledFromMainThread, bool saveToDisk );

		void threadedFunction();	//the queue runs here
		string extractFileFromUrl(string url);
		string extractExtensionFromFileName(string fileName);
			
		//bool							debug;	//should we print lots of stuff?
		bool							notifyFromMainThread;
		bool							onlySkipDownloadIfChecksumMatches;

		int								timeOut;
		string							userAgent;
		string							acceptString;
		queue<ofxSimpleHttpResponse*>	q;		//the pending requests
		bool							timeToStop;
		int								queueLenEstimation;
		int								maxQueueLen;
		float							idleTimeAfterEachDownload;	//seconds
		float							avgDownloadSpeed;
		bool							cancelCurrentDownloadOnDestruction;
	
		ofxSimpleHttpResponse			response;

		queue<ofxSimpleHttpResponse>	responsesPendingNotification; //we store here downloads that arrived so that we can notify from main thread
		map<string, string>				customHttpHeaders;

		std::streamsize streamCopyWithProgress(std::istream & in, std::ostream & out, std::streamsize totalBytes,
											   std::streamsize &currentBytes,
											   float & progress, float &speed, const bool &shouldCancel);

		std::streamsize copyToStringWithProgress(std::istream& istr, std::string& str, std::streamsize totalBytes,
												 std::streamsize &currentBytes,
												 float & progress, float & speed, const bool &shouldCancel);

		static Context::Ptr pContext;

		int COPY_BUFFER_SIZE;
};
