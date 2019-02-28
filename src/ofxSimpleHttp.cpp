/*
 *  ofxSimpleHttp.cpp
 *  emptyExample
 *
 *  Created by Oriol Ferrer Mesià on 06/11/10.
 *  Copyright 2010 uri.cat. All rights reserved.
 *
 */


#include "ofxSimpleHttp.h"
#include "ofEvents.h"
#include "ofxChecksum.h"

#include "Poco/Net/HTTPStreamFactory.h"
#include "Poco/Net/HTTPSStreamFactory.h"
#include "Poco/Net/HTTPSClientSession.h"
#include "Poco/Buffer.h"
#include "Poco/Net/SSLManager.h"
#include "Poco/Net/NetException.h"
#include "Poco/URI.h"
#include "Poco/SHA1Engine.h"
#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPResponse.h"


Poco::Net::Context::Ptr ofxSimpleHttp::pContext = NULL;

using namespace Poco::Net;
using namespace Poco;
using Poco::Exception;
using Poco::Net::HTTPClientSession;

ofxSimpleHttp::ofxSimpleHttp(){

	COPY_BUFFER_SIZE = 1024 * 16; //  kb buffer size
	cancelCurrentDownloadOnDestruction = true;
	flushPendingRequestsOnDestruction = true;
	timeOut = 10;
	queueLenEstimation = 0;
	maxQueueLen = 1000;
	timeToStop = false;
	userAgent = "ofxSimpleHttp (Poco Powered)";
	acceptString = "";
	notifyFromMainThread = true;
	onlySkipDownloadIfChecksumMatches = false;
	idleTimeAfterEachDownload = 0.0;
	avgDownloadSpeed = 0.0f;
	speedLimit = 0.0f;
	useCredentials = false;
	avgSpeedNow = 0.0f;
}


ofxSimpleHttp::~ofxSimpleHttp(){

	if(flushPendingRequestsOnDestruction){
		if(queueLenEstimation > 0) ofLogWarning("ofxSimpleHttp") << "Destructor canceling pending downloads (" << queueLenEstimation << ")";
		timeToStop = true;	//lets flag the thread so that it doesnt try access stuff while we delete things around
	}

	if(cancelCurrentDownloadOnDestruction && isThreadRunning()){
		ofLogWarning("ofxSimpleHttp") << "Destructor stopping current download...";
		stopCurrentDownload(true);
	}

	
	if (queueLenEstimation > 0) {
		ofLogWarning("ofxSimpleHttp") << "Destructor finishing all pending downloads (" << queueLenEstimation << ")";
	}
	if (isThreadRunning()) {
		try {
			waitForThread(false); //wait for the thread to completelly finish
		} catch (Exception& exc) {
			ofLogError("ofxSimpleHttp") << "Exception at waitForThread " << exc.displayText();
		}
	}

	//empty queue, dont leak out
	while (q.size() > 0) {
		ofxSimpleHttpResponse * r = q.front();
		delete r;
		q.pop();
	}
	
}


void ofxSimpleHttp::setProxyConfiguration(const ProxyConfig & c){
	proxyConfig = c;
}


void ofxSimpleHttp::createSslContext(Poco::Net::Context::Usage usage, Poco::Net::Context::VerificationMode verMode ){
	if(!pContext){
		ofLogNotice("ofxSimpleHttp") << "Initing Poco SSLManager";
		//pContext = new Context(usage, "", Context::VERIFY_RELAXED, 9, true, "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH");
		std::string certFilePath = ofToDataPath("ssl/cacert.pem", true);
		if (!ofFile::doesFileExist(certFilePath)){
			certFilePath = "";
			ofLogWarning("ofxSimpleHttp") << "CA Root Certificate not found. Place in data/ssl/cacert.pem.";
		}

		pContext = new Poco::Net::Context(Poco::Net::Context::CLIENT_USE,
										  certFilePath, //ca location
										  verMode, //verification mode
										  9, //verification depth
										  false, //load default cas (openSSL)
										  "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH" //cipher list
										  );

		Poco::Net::SSLManager::instance().initializeClient(0, 0, pContext);
	}
}

void ofxSimpleHttp::destroySslContext(){
	if(pContext){
		Poco::Net::SSLManager::instance().shutdown();
		pContext = NULL;
		ofLogNotice("ofxSimpleHttp") << "Uniniting Poco SSLManager";
	}
}

void ofxSimpleHttp::setCopyBufferSize(float KB){
	COPY_BUFFER_SIZE = KB * 1024;
}

void ofxSimpleHttp::setCancelCurrentDownloadOnDestruction(bool doIt){
	cancelCurrentDownloadOnDestruction = doIt;
}

void ofxSimpleHttp::setCancelPendingDownloadsOnDestruction(bool cancelAll){
	flushPendingRequestsOnDestruction = cancelAll;
}

void ofxSimpleHttp::setIdleTimeAfterEachDownload(float seconds){
	idleTimeAfterEachDownload = seconds;
}

void ofxSimpleHttp::setNotifyFromMainThread(bool mainThread){
	notifyFromMainThread = mainThread;
}

void ofxSimpleHttp::setNeedsChecksumMatchToSkipDownload(bool needs){
	onlySkipDownloadIfChecksumMatches = needs;
}

void ofxSimpleHttp::setTimeOut(int seconds){
	timeOut = seconds;
}


void ofxSimpleHttp::setUserAgent( std::string newUserAgent ){
	userAgent = newUserAgent;
}

void ofxSimpleHttp::setCredentials(std::string username, std::string password){
	if(username.size() || password.size()){
		credentials.setUsername(username);
		credentials.setPassword(password);
		useCredentials = true;
	}
}

void ofxSimpleHttp::setMaxQueueLength(int len){
	maxQueueLen = len;
}

void ofxSimpleHttp::addCustomHttpHeader(std::string headerName, std::string headerContent){
	customHttpHeaders[headerName] = headerContent;
}

std::string ofxSimpleHttp::getCurrentDownloadFileName(){

	std::string download = "";
	lock();
	int n = q.size();
	if ( isThreadRunning() && n > 0 ){
		ofxSimpleHttpResponse * r = q.front();
		download = r->fileName;
	}
	unlock();
	return download;
}


int ofxSimpleHttp::getPendingDownloads(){
	lock();
	queueLenEstimation = q.size();
	unlock();
	return queueLenEstimation;
}


float ofxSimpleHttp::getCurrentDownloadProgress(){

	float downloadPercent = -1;
	lock();
	int n = q.size();
	if ( isThreadRunning() && n > 0){
		ofxSimpleHttpResponse * r = q.front();
		downloadPercent = r->downloadProgress;
	}
	unlock();
	return downloadPercent;
}


void ofxSimpleHttp::threadedFunction(){

	#ifdef TARGET_WIN32
	#elif defined(TARGET_LINUX)
	pthread_setname_np(pthread_self(), "ofxSimpleHttp");
	#else
	pthread_setname_np("ofxSimpleHttp");
	#endif

	ofLogVerbose("ofxSimpleHttp") << "start threadedFunction";
	queueLenEstimation = 0;

	lock();
	queueLenEstimation = q.size();

	while( queueLenEstimation > 0 && timeToStop == false && isThreadRunning()){

		ofxSimpleHttpResponse * r = q.front();

		unlock();
		downloadURL(	r,	/*response*/
					true,	/*sendResultThroughEvents*/
					false,	/*calling from main thread*/
					r->downloadToDisk
					);
		lock();
		q.pop();
		if(r->emptyWholeQueue){
			std::queue<ofxSimpleHttpResponse*> tempQ;
			q = tempQ;
		}
		delete r;
		queueLenEstimation = q.size();
	}

	unlock();

	//if no more pending requests, let the thread die...
	ofLogVerbose("ofxSimpleHttp", "exiting threadedFunction (queue len %d)", queueLenEstimation);

//not needed anymore, only for 0.81 and below I think
//#if  defined(TARGET_OSX) || defined(TARGET_LINUX) /*I'm not 100% sure of linux*/
//	if (!timeToStop){ //if we are naturally exiting the thread; if TimeToStop==true it means we are being destructed, and the thread will be joined (so no need to detach!)
//		pthread_detach( pthread_self() ); //this is a workaround for this issue https://github.com/openframeworks/openFrameworks/issues/2506
//	}
//#endif
}


void ofxSimpleHttp::stopCurrentDownload(bool emptyQueue){

	lock();
	int n = q.size();
	if ( isThreadRunning() && n > 0){
		ofxSimpleHttpResponse * r = q.front();
		if (!r->downloadCanceled){ //dont cancel it twice!
			ofLogVerbose("ofxSimpleHttp") << "stopCurrentDownload() >> about to stop download of " + r->fileName + " ...";
			try{
				r->emptyWholeQueue = emptyQueue;
				r->downloadCanceled = true;
			}catch(Exception& exc){
				ofLogError("ofxSimpleHttp") << "stopCurrentDownload(" << r->fileName << ") >> Exception: " << exc.displayText() ;
			}
		}
	}
	unlock();
}




float ofxSimpleHttp::getAvgDownloadSpeed(){
	return avgDownloadSpeed;
}


std::string ofxSimpleHttp::minimalDrawableString(){

	std::string msg;
	lock();
	int n = q.size();
	if( isThreadRunning() && n > 0 ){
		ofxSimpleHttpResponse * r = q.front();
		char aux[16];
		sprintf(aux, "% 4d%%", (int)(r->downloadProgress * 100.0f));
		msg += std::string(aux) + " [";
		float barLen = 12;
		float numFill =  r->downloadProgress * barLen;
		for(int i = 0; i < barLen; i++){
			msg += std::string(i < numFill ? "*" : " ");
		}
		msg += "] ";
		msg += std::string( r->downloadToDisk ? ofFilePath::getFileName(r->absolutePath) : "");
		msg += " (" + bytesToHumanReadable((long long)r->downloadSpeed, 1) + "/s)";

	}
	unlock();
	return msg;
}


std::string ofxSimpleHttp::drawableString(int urlLen){

	std::string aux;
	lock();
	int n = q.size();
	if( isThreadRunning() && n > 0 ){
		ofxSimpleHttpResponse * r = q.front();
		float timeSoFar = ofGetElapsedTimef() - r->timeDowloadStarted; //seconds
		float timeRemaining = 0.0f;

		if(r->downloadedSoFar > 100){
			timeRemaining = (timeSoFar / r->downloadProgress) - timeSoFar;
		}
		std::string remTime;
		std::string serverSize;

		if(r->serverReportedSize != -1){
			serverSize = bytesToHumanReadable(r->serverReportedSize, 2);
			remTime = secondsToHumanReadable(timeRemaining, 1);
		}else{
			remTime = "unknown";
			if (r->downloadedSoFar == 0){
				std::string anim[] = {"   ", ".  ", ".. ", "...", " ..", "  ."}; //6 anim states
				serverSize = "waiting for server " + anim[ int(0.2 * ofGetFrameNum())%6 ];
			}else{
				serverSize = "";
			}
		}
		std::string spa = "      ";
		std::string url = r->url;
		if (urlLen > 0){
			url = url.substr(0, MIN(url.size(), urlLen)) + "...";
		}
		aux = "//// ofxSimpleHttp now fetching //////////////////////////////////\n"
		"//\n"
		"//   URL: " + url + "\n" +
		std::string( r->downloadToDisk ? "//   Save To: " + r->absolutePath + "\n" : "") +
		std::string(serverSize.length() ?
		"//   Progress:                 " + spa + std::string((r->downloadProgress >= 0.0) ? ofToString(100.0f * r->downloadProgress, 2) : "") + "%\n" : "") +
		std::string(serverSize.length() ?
		"//   Server Reported Size:     " + spa + serverSize + "\n" : "")+
		"//   Downloaded:               " + spa + bytesToHumanReadable((long long)r->downloadedSoFar, 2) + "\n" +
		"//   Download Speed:           " + spa + bytesToHumanReadable((long long)r->downloadSpeed, 2) + "/sec\n" +
		"//   Time Taken so far:        " + spa + secondsToHumanReadable(timeSoFar, 1) + "\n" +
		//"//   Timeout after:            " + spa + secondsToHumanReadable(timeOut, 1) + "\n" +
		"//   Estimated Remaining Time: " + spa + remTime + "\n" +
		"//   Queue Size:               " + spa + ofToString(n) + "\n" +
		"//  \n";
		aux += "//////////////////////////////////////////////////////////////////\n";
	}else{
		aux = "//// ofxSimpleHttp idle... ////////////////////////////////////////\n";
	}
	unlock();
	return aux;
}


std::string ofxSimpleHttp::bytesToHumanReadable(long long bytes, int decimalPrecision){
	std::string ret;
	if (bytes < 1024 ){ //if in bytes range
		ret = ofToString(bytes) + " bytes";
	}else{
		if (bytes < 1024 * 1024){ //if in kb range
			ret = ofToString(bytes / float(1024), decimalPrecision) + " KB";
		}else{
			if (bytes < (1024 * 1024 * 1024)){ //if in Mb range
				ret = ofToString(bytes / float(1024 * 1024), decimalPrecision) + " MB";
			}else{
				ret = ofToString(bytes / float(1024 * 1024 * 1024), decimalPrecision) + " GB";
			}
		}
	}
	return ret;
}


std::string ofxSimpleHttp::secondsToHumanReadable(float secs, int decimalPrecision){
	std::string ret;
	if (secs < 60.0f ){ //if in seconds
		ret = ofToString(secs, decimalPrecision) + " seconds";
	}else{
		if (secs < 3600.0f){ //if in min range
			ret = ofToString(secs / 60.0f, decimalPrecision) + " minutes";
		}else{ //hours or days
			if (secs < 86400.0f){ // hours
				ret = ofToString(secs / 3600.0f, decimalPrecision) + " hours";
			}else{ //days
				ret = ofToString(secs / 86400.0f, decimalPrecision) + " days";
			}
		}
	}
	return ret;
}



void ofxSimpleHttp::draw(float x, float y , float w , float h  )  {
	std::string aux = drawableString();
	ofDrawBitmapString(aux, x + 3, y + 12 );
}


void ofxSimpleHttp::draw(float x, float y) {
	draw(x,y, ofGetWidth() -x, 100);
}


void ofxSimpleHttp::drawMinimal(float x, float y, bool withBg, ofColor fontColor, ofColor bgColor) {
	std::string aux = minimalDrawableString();
	if(withBg){
		if(aux.size()){
			ofDrawBitmapStringHighlight(aux, x + 3, y + 12, bgColor, fontColor );
		}
	}else{
		ofDrawBitmapString(aux, x + 3, y + 12);
	}
}


std::string ofxSimpleHttp::getFileSystemSafeString(const std::string & input){
	static char invalidChars[] = {'?', '\\', '/', '*', '<', '>', '"', ';', ':', '#' };
	int howMany = sizeof(invalidChars) / sizeof(invalidChars[0]);
	char replacementChar = '_';
	std::string output = input;
	for(int i = 0; i < howMany; i++){
		std::replace( output.begin(), output.end(), invalidChars[i], replacementChar);
	}
	return output;
}

std::string ofxSimpleHttp::extractFileFromUrl(const std::string& url){
	int found = url.find_last_of("/");
	std::string file = url.substr(found + 1);
	file = getFileSystemSafeString(file);
	if (file.length() == 0){
		file = "unnamed.file";
	}
	return file;
}


std::string ofxSimpleHttp::extractExtensionFromFileName(const std::string& fileName){
	int found = fileName.find_last_of(".");
	std::string file;
	if (found > 0){
		file = fileName.substr(found + 1, fileName.size() - found);
	}else{
		file = "";
	}
	return file;
}


void ofxSimpleHttp::fetchURL(std::string url, bool notifyOnSuccess, std::string customField){

	if (queueLenEstimation >= maxQueueLen){
		ofLogError("ofxSimpleHttp") << "fetchURL can't do that, queue is too long already (" << queueLenEstimation << ")!";
		return;
	}

	ofxSimpleHttpResponse *response = new ofxSimpleHttpResponse();
	response->who = this;
	response->url = url;
	response->customField = customField;
	response->downloadCanceled = false;
	response->fileName = extractFileFromUrl(url);
	response->extension = extractExtensionFromFileName(response->fileName);
	response->notifyOnSuccess = notifyOnSuccess;

	lock();
	q.push(response);
	unlock();

	if ( !isThreadRunning() ){	//if the queue is not running, lets start it
		startThread(true);
	}
}


ofxSimpleHttpResponse ofxSimpleHttp::fetchURLBlocking(std::string  url){

	response.url = url;
	response.who = this;
	response.downloadCanceled = false;
	response.fileName = extractFileFromUrl(url);
	response.extension = extractExtensionFromFileName(response.fileName);
	response.notifyOnSuccess = true;
	bool ok = downloadURL(&response,
						  false/*send res through events*/,
						  true/*beingCalledFromMainThread*/,
						  false/*to disk*/
						  );
	return response;
}

void ofxSimpleHttp::fetchURLToDisk(std::string url, std::string expectedSha1, bool notifyOnSuccess,
								   std::string dirWhereToSave, std::string customField){

	if (queueLenEstimation >= maxQueueLen){
		ofLogError("ofxSimpleHttp") << "fetchURL can't do that, queue is too long already (" << queueLenEstimation << ")!";
		return;
	}

	dirWhereToSave = ofFilePath::removeTrailingSlash(dirWhereToSave);


	if(!ofDirectory::doesDirectoryExist(dirWhereToSave)){
		ofDirectory d;
		bool createDirOK = d.createDirectory(dirWhereToSave, true, true); //create the download dir first
		if(!createDirOK){
			ofLogError("ofxSimpleHttp") << "Can't create directory at '" << dirWhereToSave << "'";
		}
	}

	std::string savePath = dirWhereToSave == "" ? extractFileFromUrl(url) : ofToDataPath(dirWhereToSave, true) + "/" + extractFileFromUrl(url);

	ofxSimpleHttpResponse *response = new ofxSimpleHttpResponse();
	response->who = this;
	response->customField = customField;
	response->expectedChecksum = expectedSha1;
	response->absolutePath = savePath;
	response->url = url;
	response->downloadCanceled = false;
	response->fileName = extractFileFromUrl(url);
	response->extension = extractExtensionFromFileName(response->fileName);
	response->notifyOnSuccess = notifyOnSuccess;
	response->downloadToDisk = true;

	lock();
	q.push(response);
	unlock();

	if ( !isThreadRunning() ){	//if the queue is not running, lets start it
		try{
			startThread(true);
		}catch(exception e){
			ofLogError("ofxSimpleHttp") << "ofxSimpleHttp: cant start thread!" << e.what();
		}
	}
}

void ofxSimpleHttp::fetchURLToDisk(std::string url, bool notifyOnSuccess,
								   std::string dirWhereToSave, std::string customField){
	fetchURLToDisk(url, "", notifyOnSuccess, dirWhereToSave, customField);
}


ofxSimpleHttpResponse ofxSimpleHttp::fetchURLtoDiskBlocking(std::string  url, std::string dirWhereToSave, std::string expectedSha1){

	std::string savePath = dirWhereToSave == "" ? extractFileFromUrl(url) : ofToDataPath(dirWhereToSave) + "/" + extractFileFromUrl(url);

	ofDirectory d;
	d.createDirectory(dirWhereToSave, true, true); //create the download dir first

	response.absolutePath = savePath;
	response.url = url;
	response.who = this;
	response.expectedChecksum = expectedSha1;
	response.downloadCanceled = false;
	response.fileName = extractFileFromUrl(url);
	response.extension = extractExtensionFromFileName(response.fileName);
	response.notifyOnSuccess = true;
	response.downloadToDisk = true;
	bool ok = downloadURL(&response,
						  false,/*send result through events*/
						  true, /*beingCalledFromMainThread*/
						  true/*to disk*/
						  );
	return response;
}


bool ofxSimpleHttp::downloadURL(ofxSimpleHttpResponse* resp, bool sendResultThroughEvents, bool beingCalledFromMainThread, bool saveToDisk){

	bool ok = false;
	ofstream myfile;
	bool fileIsAlreadyHere = false;
	resp->responseBody = "";

	//create a file to save the stream to
	if(saveToDisk){

		if (resp->expectedChecksum.length()){ //if user provided a checksum
			ofFile f;
			f.open(resp->absolutePath);
			if (f.exists()){
				fileIsAlreadyHere = ofxChecksum::sha1(resp->absolutePath, resp->expectedChecksum);
				if(fileIsAlreadyHere){
					resp->checksumOK = true;
					resp->status = 0;
					resp->ok = true;
					resp->fileWasHere = true;
					ofLogVerbose("ofxSimpleHttp") << "ofxSimpleHttp: about to download " << resp->url << " but a file with same name and correct checksum is already here!";
					ofLogVerbose("ofxSimpleHttp") << "ofxSimpleHttp: skipping download (" << resp->expectedChecksum << ")";
				}
			}
			f.close();
		}else{
			if (!onlySkipDownloadIfChecksumMatches){
				ofFile f;
				f.open(resp->absolutePath);
				if (f.exists() && f.getSize() > 0){
					resp->checksumOK = false;
					resp->status = 0;
					resp->ok = true;
					resp->fileWasHere = true;
					fileIsAlreadyHere = true;
					ofLogVerbose("ofxSimpleHttp") << "ofxSimpleHttp: about to download "<< resp->url << " but a file with same name and (size > 0) is already here!";
					ofLogVerbose("ofxSimpleHttp") << "ofxSimpleHttp: skipping download (missing checksum)";
				}
				f.close();
			}
		}
	}

	if (!fileIsAlreadyHere){ //if file is not here, download it!

		std::string protocol = resp->url.substr(0,7);

		if(saveToDisk || protocol == "file://"){
			myfile.open( resp->absolutePath.c_str(), ios_base::binary );
		}

		if(protocol == "file://"){

			std::string srcFilePath = resp->url.substr(7, resp->url.length() - 7);

#ifdef TARGET_WIN32
			ofStringReplace(srcFilePath, "\\", "/"); //for windows, replace escaped backslashes
#endif
			//ofFile::copyFromTo(srcFile, resp->absolutePath);

			ofFile srcOfFile;
			bool openOK = srcOfFile.open(ofToDataPath(srcFilePath, true), ofFile::ReadOnly);
			resp->downloadSpeed = 0;
			resp->downloadedBytes = 0;

			if(srcOfFile.exists()){
				uint64_t size = srcOfFile.getSize();
				resp->serverReportedSize = size;
				resp->timeDowloadStarted = ofGetElapsedTimef();
				std::ifstream rs (ofToDataPath(srcFilePath, true).c_str(), std::ifstream::binary);

				if(saveToDisk){
					streamCopyWithProgress(rs, myfile, resp->serverReportedSize, resp->downloadedSoFar, resp->chunkTested,
									   resp->downloadProgress, resp->downloadSpeed, resp->downloadCanceled);

				}else{

					std::ostringstream output;
					streamCopyWithProgress(rs, output, resp->serverReportedSize, resp->downloadedSoFar, resp->chunkTested,
													  resp->downloadProgress, resp->downloadSpeed, resp->downloadCanceled);
					resp->responseBody = output.str();

				}
				resp->ok = true;
				resp->status = 200;
				resp->timeTakenToDownload = ofGetElapsedTimef() - resp->timeDowloadStarted;
				resp->downloadedBytes = resp->serverReportedSize;

				if (resp->expectedChecksum.length() > 0){
					resp->checksumOK = ofxChecksum::sha1(resp->absolutePath, resp->expectedChecksum);
					if(!resp->checksumOK){
						ofLogVerbose("ofxSimpleHttp") << "ofxSimpleHttp: file:// copy OK but Checksum FAILED";
						ofLogVerbose("ofxSimpleHttp") << "ofxSimpleHttp: SHA1 was meant to be: " << resp->expectedChecksum;
					}
				}

				rs.close();
				ok = true;
			}else{
				ofLogError("ofxSimpleHttp") << "Source File does not exist! " << resp->url;
				resp->ok = false;
				resp->status = 404; //assume not found? todo!
				resp->reasonForStatus = "Source File does not exist!";
				resp->timeTakenToDownload = 0;
				resp->checksumOK = false;
				ok = false;
			}
			srcOfFile.close();
			myfile.close();


			if(!ok && saveToDisk){
				ofFile::removeFile(resp->absolutePath, false); //if we failed, dont leave an empty file in there
			}

		}else{

			HTTPClientSession * session = NULL;

			try {

				ofHttpRequest request(resp->url, resp->url);
				URI uri(request.url);
				std::string path(uri.getPathAndQuery());
				if (path.empty()) path = "/";

				if(uri.getScheme()=="https"){
					session = new HTTPSClientSession(uri.getHost(), uri.getPort(), pContext);
				}else{
					session = new HTTPClientSession(uri.getHost(), uri.getPort());
				}

				if(proxyConfig.useProxy){
					session->setProxy(proxyConfig.host, proxyConfig.port);
					if(proxyConfig.login.size() && proxyConfig.password.size()){
						session->setProxyCredentials(proxyConfig.login, proxyConfig.password);
					}
				}

				HTTPRequest req(HTTPRequest::HTTP_GET, path, HTTPMessage::HTTP_1_1);
				req.set( "User-Agent", userAgent.c_str() );

				//add custom headers to the request
				std::map<std::string, std::string>::iterator it = customHttpHeaders.begin();
				while(it != customHttpHeaders.end()){
					req.set( it->first, it->second );
					++it;
				}

				session->setTimeout( Poco::Timespan(timeOut,0) );

				if(useCredentials){
					credentials.authenticate(req);
				}
				
				try{
					session->sendRequest(req);
				}catch(exception e){
					ofLogWarning("ofxSimpleHttp") << "ofxSimpleHttp session send request exception: " << e.what() << " for URL: " << request.url;
				}

				HTTPResponse res;
				istream& rs = session->receiveResponse(res);

				resp->status = res.getStatus();
				try {
					resp->timestamp = res.getDate();
				} catch (exception exc) {
					resp->timestamp = 0;
				}

				resp->reasonForStatus = res.getReasonForStatus( res.getStatus() );
				resp->contentType = res.getContentType();
				resp->serverReportedSize = res.getContentLength();
				resp->timeDowloadStarted = ofGetElapsedTimef();

				std::string msg;
				if (resp->serverReportedSize == -1){
					msg = "downloadURL(" + resp->fileName + ") >> Server doesn't report download size...";
					ofLogWarning("ofxSimpleHttp") << msg;
				}

				if (resp->serverReportedSize == 0){
					ofLogWarning("ofxSimpleHttp") << "Server reports file size 0 bytes!";
				}
				msg = "downloadURL() >> about to start download (" + resp->fileName + ", " + ofToString(res.getContentLength()) + " bytes)";
				ofLogVerbose("ofxSimpleHttp") << msg;
				msg = "downloadURL() >> server reports request status: " +ofToString(resp->status) + " - ", resp->reasonForStatus + ")";
				ofLogVerbose("ofxSimpleHttp") << msg ;

				int copySize = 0;
				if(saveToDisk){
					copySize = streamCopyWithProgress(rs, myfile, resp->serverReportedSize, resp->downloadedSoFar, resp->chunkTested,
													  resp->downloadProgress, resp->downloadSpeed, resp->downloadCanceled);
				}else{
					std::ostringstream output;
					copySize = streamCopyWithProgress(rs, output, resp->serverReportedSize, resp->downloadedSoFar, resp->chunkTested,
														resp->downloadProgress, resp->downloadSpeed, resp->downloadCanceled);
					resp->responseBody = output.str();
				}

				delete session;
				session = NULL;

				if (copySize >= 0){

					if(saveToDisk){
						myfile.close();
					}

					resp->timeTakenToDownload = ofGetElapsedTimef() - resp->timeDowloadStarted;
					if (resp->expectedChecksum.length() > 0){
						resp->checksumOK = ofxChecksum::sha1(resp->absolutePath, resp->expectedChecksum);
						if(!resp->checksumOK){
							ofLogVerbose("ofxSimpleHttp") << "ofxSimpleHttp: downloaded OK but Checksum FAILED";
							ofLogVerbose("ofxSimpleHttp") << "ofxSimpleHttp: SHA1 was meant to be: " << resp->expectedChecksum;
						}
					}

					resp->downloadSpeed = 0;
					resp->downloadedBytes = 0;

					if (resp->downloadCanceled){
						//delete half-baked download file
						if (resp->downloadToDisk){
							ofFile f;
							if(f.open(resp->absolutePath)){
								if (f.isFile()){
									f.remove();
								}
							}
						}

					}else{ //download not canceled

						if(saveToDisk){
							std::string msg = "downloadURL() downloaded to \"" + resp->absolutePath + "\"";
							ofLogNotice("ofxSimpleHttp", msg);
							//ask the filesystem what is the real size of the file
							ofFile file;
							try{
								file.open(resp->absolutePath.c_str());
								resp->downloadedBytes = file.getSize();
								file.close();
							}catch(Exception& exc){
								std::string msg = "downloadURL(" + resp->fileName + ") >> Exception at file.open: " + exc.displayText();
								ofLogError("ofxSimpleHttp") << msg ;
							}
						}else{
							resp->downloadedBytes = resp->responseBody.size();
						}

						if(resp->timeTakenToDownload > 0.01f){
							resp->avgDownloadSpeed = (resp->downloadedBytes ) / resp->timeTakenToDownload; //bytes/sec
							ofLogNotice("ofxSimpleHttp") << "download completed - avg download speed: " << bytesToHumanReadable(resp->avgDownloadSpeed,1) << " - Dl Size: " << bytesToHumanReadable(resp->downloadedBytes, 1);
						}
						avgDownloadSpeed = resp->avgDownloadSpeed;

						bool isAPI = (resp->contentType == "text/json");
						bool sizeMissmatch = resp->serverReportedSize > 0 && resp->serverReportedSize != resp->downloadedBytes;

						//check download file size missmatch
						if ( sizeMissmatch && !isAPI ) {
							std::string msg;

							if (resp->downloadedBytes == 0){
								if (resp->timeTakenToDownload > timeOut){
									msg = "downloadURLtoDiskBlocking() >> TimeOut! (" + resp->fileName + ")";
									resp->reasonForStatus = "Request TimeOut";
									resp->status = HTTPResponse::HTTP_REQUEST_TIMEOUT;
								}else{
									msg = "downloadURLtoDiskBlocking() >> Download file is 0 bytes  (" + resp->fileName + ")";
									resp->reasonForStatus = "Download size is 0 bytes!";
								}
							}else{
								msg = "downloadURLtoDiskBlocking() >> Download size mismatch (" + resp->fileName + ") >> Server: " +
								ofToString(resp->serverReportedSize) + " Downloaded: " + ofToString(resp->downloadedBytes);
								resp->reasonForStatus = "Download size mismatch";
							}

							ofLogWarning("ofxSimpleHttp", msg);
							resp->status = -1;
							resp->ok = false;

						}else{

							if (resp->status == HTTPResponse::HTTP_OK){ //mmm
								resp->ok = true;
							}else{
								resp->ok = false;
							}
						}
						ofLogVerbose("ofxSimpleHttp") << "ofxSimpleHttp: download finished! " << resp->url << " !";
						ok = true;
					}

					if (copySize == 0){
						ofLogError("ofxSimpleHttp") << "downloaded file is empty!";
						resp->ok = false;
						resp->reasonForStatus += " / server file is empty!";
					}

				}else{
					resp->ok = false;
					resp->reasonForStatus = "Unknown exception at streamCopy";
					resp->status = -1;
					ok = false;
				}

				//last check for OK flag
				if(!resp->checksumOK){
					resp->ok = false;
					resp->reasonForStatus += " / SHA1 missmatch!";
				}

			}catch(Exception& exc){

				if (session) session->reset();
				myfile.close();
				std::string msg = "downloadURL(" + resp->fileName +  ") >> Exception: " + exc.displayText();
				resp->timeTakenToDownload = ofGetElapsedTimef() - resp->timeDowloadStarted;
				if (resp->timeTakenToDownload > timeOut){
					msg = "downloadURLtoDiskBlocking() >> TimeOut! (" + resp->fileName + ")";
					resp->reasonForStatus = "Request TimeOut";
					resp->status = 408;
				}else{
					resp->reasonForStatus = exc.displayText();
					if (exc.code() > 0){
						resp->status = exc.code();
					}else{
						resp->status = -1;
					}
				}
				resp->ok = false;
				ok = false;
				ofLogError("ofxSimpleHttp") << "failed to download " << resp->url;
				ofLogError("ofxSimpleHttp") << msg;
			}
            
            if(session != NULL){
                delete session;
                // session = NULL;
            }
		}
	}else{
		resp->timeTakenToDownload = 0;
		ok = true;
	}

	//enqueue the operation result!
	if (sendResultThroughEvents ){

		if ( resp->notifyOnSuccess ){

			if(idleTimeAfterEachDownload > 0.0f){
				ofSleepMillis(idleTimeAfterEachDownload * 1000);
			}

			if (beingCalledFromMainThread){ //we running on main thread, we can just snd the notif from here

				ofNotifyEvent( httpResponse, *resp, this );

			}else{ //we are running from a bg thread

				if (notifyFromMainThread){ //user wants to get notified form main thread, we need to enqueue the notification
					if (timeToStop == false){	//see if we have been destructed! dont forward events if so
						lock();
						ofxSimpleHttpResponse tempCopy = *resp;
						responsesPendingNotification.push(tempCopy);
						unlock();
					}
				}else{ //user doesnt care about main thread, the notificaiton can come from bg thread so we do it from here
					ofNotifyEvent( httpResponse, *resp, this );
				}
			}
		}
	}

	return ok;
}


void ofxSimpleHttp::update(){

	ofxSimpleHttpResponse r;
	lock();
	
	avgDownloadSpeed = 0.95f * avgDownloadSpeed + 0.05f * avgSpeedNow;
	//cout << this << " now: " << avgSpeedNow / float(1024.0f*1024) << "Mb/sec  avg: " << avgDownloadSpeed / float(1024.0f*1024) << "Mb/sec" << endl;

	if(responsesPendingNotification.size()){
		r = responsesPendingNotification.front();
		responsesPendingNotification.pop();
		unlock();
		ofNotifyEvent( httpResponse, r, this ); //we want to be able to notify from outside the lock
		//otherwise we cant start a new download from the callback (deadlock!)
	}else{
		unlock();
	}
}


std::streamsize ofxSimpleHttp::streamCopyWithProgress(std::istream & istr, std::ostream & out,
													  std::streamsize totalBytes,
													  std::streamsize &currentBytes,
													  bool & chunkTested,
													  float & progress, float & speed, const bool &cancel){
	std::streamsize len = 0;
	try{
		Buffer<char> buffer(COPY_BUFFER_SIZE);
		istr.read(buffer.begin(), COPY_BUFFER_SIZE);
		std::streamsize n = istr.gcount();
		float avgSpeed = 0;
		float avgdCurrentSpeed = 0;
		bool first = true;
		float sleepError = 0; //in sec

		float timeB4Start = ofGetElapsedTimef();
		
		while (n > 0 && !cancel){
			float t1 = ofGetElapsedTimef();
			currentBytes = len;
			len += n;
			out.write(buffer.begin(), static_cast<std::string::size_type>(n));
			if (istr){
				istr.read(buffer.begin(), COPY_BUFFER_SIZE);
				n = istr.gcount();
				if (istr.fail() && !istr.eof()){
					ofLogError("ofxSimpleHttp") << "streamCopyWithProgress() >> iostream Fail!";
					return -1;
				}
			}else{
				n = 0;
			}
			if (totalBytes > 0){
				progress = float(len) / float(totalBytes);
			}else{
				progress = 0.0;
			}

			float timeThisChunk = ofGetElapsedTimef();
			float time = (timeThisChunk - t1);

			if(speedLimit > 0.0f && n > 0){ //apply speed limit if defined

				float expectedTimeToDLCopyBuffer = COPY_BUFFER_SIZE / (speedLimit * 1024.0f);
				float sleepTime = (expectedTimeToDLCopyBuffer - time) * 1000.0f;
				//cout << "## expected DL time: " << expectedTimeToDLCopyBuffer * 1000 << " #######################" << endl;
				//cout << "actual DL time: " << time * 1000 << endl;
				//cout << "calculated sleep time: " << sleepTime << endl;
				if(sleepTime - sleepError * 1000.0f > 0.0f){
					ofSleepMillis(sleepTime - sleepError * 1000.0f);
					//cout << "sleeping " << sleepTime - sleepError * 1000.0f << endl;
				}else{
					sleepError = 0.0;
				}
				time = (ofGetElapsedTimef() - t1);
				float overslept = (time - expectedTimeToDLCopyBuffer);
				//cout << "overslept: " << overslept << endl;
				if(first){
					sleepError = overslept;
				}else{
					sleepError = 0.5 * sleepError + 0.5 * overslept;
				}
				//cout << "sleepError: " << sleepError * 1000 << endl;
			}
			
			if(totalBytes >= COPY_BUFFER_SIZE){ //at least COPY_BUFFER_SIZE bytes of download
				float newSpeed = 0;
				float timeSoLong = timeThisChunk - timeB4Start;
				newSpeed = (totalBytes) / (timeSoLong); //bytes / sec
				if(chunkTested){
					avgSpeed = 0.05 * newSpeed + 0.95 * avgSpeed;
				}else{
					avgSpeed = newSpeed;
				}
				avgSpeedNow = avgSpeed;
				chunkTested = true;
			}
			
			if(n >= COPY_BUFFER_SIZE){ //at least COPY_BUFFER_SIZE bytes of download
				if(time > 0.01f){
					float tSpeed = (n) / (time); //bytes / sec
					avgdCurrentSpeed = 0.05 * tSpeed + 0.95 * avgdCurrentSpeed;
					speed = avgdCurrentSpeed;
					chunkTested = true;
				}
			}

			first = false;
		}
	}catch(Exception& exc){
		ofLogError("ofxSimpleHttp") << "streamCopyWithProgress() >> Exception: " << exc.displayText() ;
		return -1;
	}
	return len;
}


ofxSimpleHttpResponse::ofxSimpleHttpResponse(){
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
	chunkTested = false;
}

std::string ofxSimpleHttpResponse::toString(){

	std::stringstream ss;
	ss << "URL: " << url << endl;
	if(customField.size()) ss << "CustomField: " << customField << endl;
	if (ok){
		if (fileWasHere){
			ss << "    File was already on disk, no download needed!" << endl;
			if (expectedChecksum.size()){
				ss << "    File checksum " << expectedChecksum << " matched!" << endl;
			}else{
				ss << "    File checksum not supplied, assuming file is the same blindly" << endl;
			}
			ss << "    File saved at: " << absolutePath << endl;
		}else{
			ss << "    Server Status: " << status << endl;
			ss << "    Server Reported size: " << serverReportedSize << endl;
			ss << "    Content Type: " << contentType << endl;
			if(expectedChecksum.length()){
				ss << "    Expected Checksum: " << expectedChecksum << endl;
				ss << "    Checksum Match: " << std::string(checksumOK ? "YES" : "NO") << endl;
			}
			ss << "    Download Time taken: " << timeTakenToDownload << " seconds" << endl;
			if (serverReportedSize != -1){
				ss << "    Avg Download Speed: " << (serverReportedSize / 1024.f) / timeTakenToDownload << "Kb/sec" << endl;
			}
			if(downloadToDisk){
				ss << "    File Saved at: " << absolutePath << endl;
			}
		}
	}else{
		ss << "    Download FAILED! " << endl;
		ss << "    Status: " << status << " - " << reasonForStatus << endl;
	}
	return ss.str();
}

void ofxSimpleHttpResponse::print(){
	if (ok){
		ofLogNotice("ofxSimpleHttpResponse") << toString();
	}else{
		ofLogError("ofxSimpleHttpResponse") << toString();
	}
}

void ofxSimpleHttp::setSpeedLimit(float KB_per_sec){
	speedLimit = KB_per_sec;
	if(KB_per_sec > 0.0){
		ofLogNotice("ofxSimpleHttp") << "Setting speed limit to " << KB_per_sec << " Kb/sec";
	}else{
		ofLogNotice("ofxSimpleHttp") << "Removing any download speed limits!";
	}
}
