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
 
	void ofApp::newResponse(ofxSimpleHttpResponse &response){
		printf("download of '%s' returned : %s\n", response.url.c_str(), response.ok ? "OK" : "KO" );
	}
 
 3 - add Listener:
 
	ofAddListener(http.httpResponse, this, &ofApp::newResponse);
 
 4 - fetch URLs:

	blocking:		ofxSimpleHttpResponse response = http.fetchURLBlocking("http://uri.cat/");
	non-blocking:	ofxSimpleHttpResponse response = http.fetchURL("http://uri.cat/"); 
	
	//you can queue several non-bloking requests and they will download one after each other
 
 5 - Recieve data when available:
	
	your newResponse(...) method will be called when the download is available.
 
 */

#pragma once


#include "Poco/Net/Context.h"
#include "Poco/Net/HTTPBasicCredentials.h"
#include "ofxSimpleHttpResponse.h"
#include "ofThread.h"
#include "ofEvents.h"
#include "ofColor.h"
#include <queue>
#include <map>

class ofxSimpleHttp : public ofThread{

	public:

		ofxSimpleHttp();
		~ofxSimpleHttp();

		struct ProxyConfig{
			bool useProxy;
			std::string host;
			int port;
			std::string password;
			std::string login;
			ProxyConfig(){useProxy = 0; port = 80;}
		};

		// actions //////////////////////////////////////////////////////////////

		//download to RAM ( download to ofxSimpleHttpResponse->responseBody)
		void						fetchURL(std::string url,
											 bool notifyOnSuccess = false,
											 std::string customField = ""); //supply any info you need, get it back when you are notified

		ofxSimpleHttpResponse		fetchURLBlocking(std::string url);

		//download to Disk
		void						fetchURLToDisk(std::string url,
												   bool notifyOnSuccess = false,
												   std::string outputDir = ".",
												   std::string customField = ""); //supply any info you need, get it back when you are notified

		void						fetchURLToDisk(std::string url,
												   std::string expectedSha1,
												   bool notifyOnSuccess = false,
												   std::string outputDir = ".", std::string
												   customField = ""); //supply any info you need, get it back when you are notified

		ofxSimpleHttpResponse		fetchURLtoDiskBlocking(std::string url,
														   std::string outputDir = ".",
														   std::string expectedSha1 = "");

		void						update(); //this is mainly used to get notifications in the main thread

		void						draw(float x, float y , float w , float h);	//draws a box
		void						draw(float x, float y);	//draws a box
		std::string					drawableString(int urlLen = 0);
		void						drawMinimal(float x, float y,
												bool withBg = false,
												ofColor fontColor = ofColor::white,
												ofColor bgColor = ofColor::black);	//draws a one-line status
		std::string						minimalDrawableString();

		void						stopCurrentDownload(bool emptyQueue); //if there's more downloads on queue, next will start immediatelly

		int							getPendingDownloads();
		float						getCurrentDownloadProgress();	//retuns [0..1] how complete is the download
		std::string					getCurrentDownloadFileName();	//only while downloading
		float						getAvgDownloadSpeed();			// bytes/sec - also when not download

		// properties //////////////////////////////////////////////////////////

		void 						setProxyConfiguration(const ProxyConfig & c);

		void						setTimeOut(int seconds);
		void						setUserAgent(std::string newUserAgent );
		void						setCredentials(std::string username, std::string password);
		void						setMaxQueueLength(int len);
		void 						setCopyBufferSize(float KB); /*in KiloBytes (1 -> 1024 bytes)*/
		void						setIdleTimeAfterEachDownload(float seconds); //wait a bit before notifying once the dowload is over

		void						setCancelCurrentDownloadOnDestruction(bool doIt);
		void						setCancelPendingDownloadsOnDestruction(bool cancelAll);

		void						addCustomHttpHeader(std::string headerName, std::string headerContent);
		void						setNotifyFromMainThread(bool mainThread);

									//if a download is requested and file already exists on disk,
									//do we require a checksum match to assume the file is good?
									//ie, if the user doesnt provide a checksum, what do we do?
		void						setNeedsChecksumMatchToSkipDownload(bool needs);
		void						setSpeedLimit(float KB_per_sec);


		ofEvent<ofxSimpleHttpResponse>		httpResponse;

		// https support //////////////////////////////////////////////////////////////

		//call once, before any https connection is made
		static void 				createSslContext(Poco::Net::Context::Usage = Poco::Net::Context::CLIENT_USE,
													 Poco::Net::Context::VerificationMode verMode = Poco::Net::Context::VERIFY_NONE);

		static void 				destroySslContext(); //call once when no longer need https, once all trasnfers are finished
														//or just b4 app exit

		// STATIC Utility Methods ////////////////////////////////////////////////////////

		static std::string				bytesToHumanReadable(long long bytes, int decimalPrecision);
		static std::string				secondsToHumanReadable(float sec, int decimalPrecision);
		static std::string				extractFileFromUrl(const std::string & url);
		static std::string				extractExtensionFromFileName(const std::string& fileName);
		static std::string				getFileSystemSafeString(const std::string & input);

private:

		bool downloadURL( ofxSimpleHttpResponse * resp, bool sendResultThroughEvents, bool beingCalledFromMainThread, bool saveToDisk );
		void threadedFunction();	//the queue runs here

		//bool							debug;	//should we print lots of stuff?
		bool							notifyFromMainThread;
		bool							onlySkipDownloadIfChecksumMatches;

		int								timeOut;
		std::string						userAgent;
		std::string						acceptString;
		std::queue<ofxSimpleHttpResponse*>	q;		//the pending requests
		bool							timeToStop;
		int								queueLenEstimation;
		int								maxQueueLen;
		float							idleTimeAfterEachDownload;	//seconds
		float							avgDownloadSpeed;
		bool							cancelCurrentDownloadOnDestruction;
		bool							flushPendingRequestsOnDestruction;

		float							speedLimit; //in KiloBytes / sec
		ofxSimpleHttpResponse			response;

		ProxyConfig						proxyConfig;
		bool							useCredentials;
		Poco::Net::HTTPBasicCredentials	credentials;

		std::queue<ofxSimpleHttpResponse>	responsesPendingNotification; //we store here downloads that arrived so that we can notify from main thread
		std::map<std::string, std::string>				customHttpHeaders;

		std::streamsize streamCopyWithProgress(std::istream & in, std::ostream & out, std::streamsize totalBytes,
											   std::streamsize &currentBytes, bool & speedSampled,
											   float & progress, float &speed, const bool &shouldCancel);

		static Poco::Net::Context::Ptr pContext;

		int COPY_BUFFER_SIZE;

		float avgSpeedNow;

};
