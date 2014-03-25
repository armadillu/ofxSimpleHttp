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


#pragma once

#include "ofMain.h"
#include "ofEvents.h"
#include <queue>
#include <stdio.h>
#include "Poco/Net/HTTPClientSession.h"
#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPResponse.h"
#include "Poco/StreamCopier.h"
#include "Poco/Path.h"
#include "Poco/URI.h"
#include "Poco/Exception.h"
#include <iostream>


#define COPY_BUFFER_SIZE				1024 * 50   /*50 Kb buffer size*/

using namespace Poco::Net;
using namespace Poco;
using Poco::Exception;
using Poco::Net::HTTPClientSession;


struct ofxSimpleHttpResponse{

	bool						ok;
	bool						notifyOnSuccess;	// user wants to be notified when download is ready
	bool						downloadCanceled;	// flag to cancel download
	bool						downloadToDisk;		// user wants bytes on disk; otherwise just return data as string in "responseBody"
	bool						emptyWholeQueue;	//flag
	bool						checksumOK;
	string						expectedChecksum;	//sha1
	int							status; 			// return code for the response ie: 200 = OK
	int							serverReportedSize;
	string						reasonForStatus;	// text explaining the status
	string						responseBody;		// the actual response << DATA IS HERE!
	string						contentType;		// the mime type of the response
	Poco::Timestamp				timestamp;			// time of the response
	float						downloadProgress;	//[0..1]
	float						downloadSpeed;		//kb/sec
	string						url;
	string						fileName;			//file + extension, no path
	string						absolutePath;		//where file was saved
	float						timeTakenToDownload;//seconds

	//HTTPClientSession *			session;			//careful what you do with this! will be NULL if session is finished

	ofxSimpleHttpResponse(){
		downloadToDisk = emptyWholeQueue = false;
		checksumOK = true;
		downloadProgress = downloadSpeed = 0.0f;
		//session = NULL;
		timeTakenToDownload = 0.0;
		serverReportedSize = -1;
	}

	void print(){
		cout << "#### " << url << " ########" << endl;
		if (ok){
			cout << "    server status: " << status << endl;
			cout << "    server reported size: " << serverReportedSize << endl;
			cout << "    content type: " << contentType << endl;
			if(expectedChecksum.length()){
				cout << "    expected checksum: " << expectedChecksum << endl;
				cout << "    checksum match: " << string(checksumOK ? "YES" : "NO") << endl;
			}
			cout << "    time to download: " << timeTakenToDownload << " seconds" << endl;
			if (serverReportedSize != -1){
				cout << "    avg dl speed: " << (serverReportedSize / 1024.f) / timeTakenToDownload << "Kb/sec" << endl;
			}
			if(downloadToDisk){
				cout << "    saved at: " << absolutePath << endl;
			}
		}else{
			cout << "    download FAILED! " << endl;
			cout << "    status: " << status << " " << reasonForStatus << endl;
		}
		cout << endl;
	}
};


class ofxSimpleHttp : public ofThread, public ofBaseDraws{
	
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
		ofxSimpleHttpResponse		fetchURLtoDiskBlocking(string url, string outputDir = ".");

		void						update(); //this is mainly used to get notifications in the main thread

		void						draw(float x, float y , float w , float h );	//draws a box
		void						draw(float x, float y );	//draws a box

		float getHeight(){ if ( isThreadRunning() ) return 18 * 4; else return 18; }
		float getWidth(){ return 320; }
	
		void						stopCurrentDownload(bool emptyQueue); //if there's more downloads on queue, next will start immediatelly

		int							getPendingDownloads();
		float						getCurrentDownloadProgress();	//retuns [0..1] how complete is the download
		string						getCurrentDownloadFileName();
		//ofxSimpleHttpResponse*		getCurrentDownloadResponse(){ return &response;}	//get this to read progress from another thread, might return NULL if no download is running
	
		// properties //////////////////////////////////////////////////////////
	
		void						setTimeOut(int seconds);
		void						setVerbose(bool verbose);
		void						setUserAgent( string newUserAgent );
		void						setAcceptString( string newAcceptString );
		void						setMaxQueueLength(int len);

		void						setNotifyFromMainThread(bool mainThread);

		ofEvent<ofxSimpleHttpResponse>		httpResponse;

	
	private:
		
		bool downloadURL( ofxSimpleHttpResponse * resp, bool sendResultThroughEvents, bool saveToDisk );

		void threadedFunction();	//the queue runs here
		string extractFileFromUrl(string url);
			
		bool							debug;	//should we print lots of stuff?
		bool							notifyFromMainThread;
		int								timeOut;
		string							userAgent;
		string							acceptString;
		queue<ofxSimpleHttpResponse*>	q;		//the pending requests
		bool							timeToStop;
		int								queueLenEstimation;
		int								maxQueueLen;
	
		ofxSimpleHttpResponse			response;

		queue<ofxSimpleHttpResponse>	responsesPendingNotification; //we store here downloads that arrived so that we can notify from main thread

		void streamCopyWithProgress(std::istream & in, std::ostream & out, std::streamsize totalBytes,float & progress, float &speed, const bool &shouldCancel);
		std::streamsize copyToStringWithProgress(std::istream& istr, std::string& str, std::streamsize totalBytes, float & progress, float & speed, const bool &shouldCancel);
};
