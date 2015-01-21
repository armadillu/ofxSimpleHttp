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
#include "poco/Net/HTTPStreamFactory.h"
#include "Poco/Net/HTTPSStreamFactory.h"
#include "Poco/Net/HTTPSClientSession.h"
#include "Poco/Buffer.h"
#include "ofxChecksum.h"
#include "Poco/Net/SSLManager.h"
#include "Poco/Net/KeyConsoleHandler.h"
#include "Poco/Net/ConsoleCertificateHandler.h"
#include "Poco/Net/NetException.h"
#include <iostream>     // std::cout
#include <fstream>      // std::ifstream


Context::Ptr ofxSimpleHttp::pContext = NULL;


ofxSimpleHttp::ofxSimpleHttp(){

	useProxy = false;
	COPY_BUFFER_SIZE = 1024 * 128; //  kb buffer size
	cancelCurrentDownloadOnDestruction = true;
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
}


ofxSimpleHttp::~ofxSimpleHttp(){

	timeToStop = true;	//lets flag the thread so that it doesnt try access stuff while we delete things around
	if(cancelCurrentDownloadOnDestruction){
		stopCurrentDownload(true);
	}

	try{
		waitForThread(false);
	}catch(Exception& exc){
		ofLogError("ofxSimpleHttp", "Exception at waitForThread %s", exc.displayText().c_str() );
	}

	//empty queue
	while ( getPendingDownloads() > 0 ){
		lock();
		ofxSimpleHttpResponse * r = q.front();
		delete r;
		q.pop();
		unlock();
	}
}


void ofxSimpleHttp::setUseProxy(bool useProxy_, string proxyHost_, int proxyPort_,
								string proxyLogin_, string proxyPassword_){
	useProxy = useProxy_;
	proxyHost = proxyHost_;
	proxyPort = proxyPort_;
	proxyLogin = proxyLogin_;
	proxyPassword = proxyPassword_;
}


void ofxSimpleHttp::createSslContext(Poco::Net::Context::Usage usage ){
	if(!pContext){
		pContext = new Context(usage, "", Context::VERIFY_NONE);
		Poco::Net::SSLManager::instance().initializeClient(0, 0, pContext);
		ofLogWarning() << "initing Poco SSLManager";
	}
}

void ofxSimpleHttp::destroySslContext(){
	if(pContext){
		Poco::Net::SSLManager::instance().shutdown();
		pContext = NULL;
		ofLogWarning() << "uniniting Poco SSLManager";
	}
}

void ofxSimpleHttp::setCopyBufferSize(int KB){
	COPY_BUFFER_SIZE = KB * 1024;
}

void ofxSimpleHttp::setCancelCurrentDownloadOnDestruction(bool doIt){
	cancelCurrentDownloadOnDestruction = doIt;
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


void ofxSimpleHttp::setVerbose(bool verbose){}


void ofxSimpleHttp::setUserAgent( string newUserAgent ){
	userAgent = newUserAgent;
}


void ofxSimpleHttp::setMaxQueueLength(int len){
	maxQueueLen = len;
}

void ofxSimpleHttp::addCustomHttpHeader(string headerName, string headerContent){
	customHttpHeaders[headerName] = headerContent;
}

string ofxSimpleHttp::getCurrentDownloadFileName(){

	string download = "";
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

#ifdef TARGET_OSX
	pthread_setname_np("ofxSimpleHttp");
#endif

	ofLogVerbose("ofxSimpleHttp", "start threadedFunction");
	queueLenEstimation = 0;

	lock();
	queueLenEstimation = q.size();

	while( queueLenEstimation > 0 && timeToStop == false && isThreadRunning()){

		ofxSimpleHttpResponse * r = q.front();

		unlock();
		downloadURL(	r,		/*response*/
					true,	/*sendResultThroughEvents*/
					false,	/*calling from main thread*/
					r->downloadToDisk
					);
		lock();
		q.pop();
		if(r->emptyWholeQueue){
			queue<ofxSimpleHttpResponse*> tempQ;
			q = tempQ;
		}
		delete r;
		queueLenEstimation = q.size();
	}

	unlock();

	//if no more pending requests, let the thread die...
	ofLogVerbose("ofxSimpleHttp", "exiting threadedFunction (queue len %d)", queueLenEstimation);

#if  defined(TARGET_OSX) || defined(TARGET_LINUX) /*I'm not 100% sure of linux*/
	if (!timeToStop){ //if we are naturally exiting the thread; if TimeToStop==true it means we are being destructed, and the thread will be joined (so no need to detach!)
		pthread_detach( pthread_self() ); //this is a workaround for this issue https://github.com/openframeworks/openFrameworks/issues/2506
	}
#endif
}


void ofxSimpleHttp::stopCurrentDownload(bool emptyQueue){

	lock();
	int n = q.size();
	if ( isThreadRunning() && n > 0){
		ofxSimpleHttpResponse * r = q.front();
		if (!r->downloadCanceled){ //dont cancel it twice!
			string msg = "stopCurrentDownload() >> about to stop download of " + r->fileName  + " ...";
			ofLogVerbose("ofxSimpleHttp", msg);
			try{
				r->emptyWholeQueue = emptyQueue;
				r->downloadCanceled = true;
				//if ( r->session != NULL ){
				//cout << "aboritng session " << r->session;
				//	r->session->abort();
				//}
			}catch(Exception& exc){
				ofLogError("ofxSimpleHttp", "stopCurrentDownload(" + r->fileName + ") >> Exception: " + exc.displayText() );
			}
		}
	}
	unlock();
}

float ofxSimpleHttp::getCurrentDownloadSpeed(){
	float downloadSpeed = 0.0f;
	lock();
	int n = q.size();
	if ( isThreadRunning() && n > 0){
		ofxSimpleHttpResponse * r = q.front();
		downloadSpeed = r->downloadSpeed;
	}
	unlock();
	return downloadSpeed;
}


float ofxSimpleHttp::getAvgDownloadSpeed(){
	return avgDownloadSpeed;
}


string ofxSimpleHttp::minimalDrawableString(){

	string msg;
	lock();
	int n = q.size();
	if( isThreadRunning() && n > 0 ){
		msg = "ofxSimpleHttp:";
		ofxSimpleHttpResponse * r = q.front();
		char aux[10];
		sprintf(aux, "% 4d%%", (int)(r->downloadProgress * 100.0f));
		msg += string(aux) + " [";
		float barLen = 12;
		float numFill =  r->downloadProgress * barLen;
		for(int i = 0; i < barLen; i++){
			msg += string(i < numFill ? "*" : " ");
		}
		msg += "] ";
		msg += string( r->downloadToDisk ? ofFilePath::getFileName(r->absolutePath) : "");
	}
	unlock();
	return msg;
}


string ofxSimpleHttp::drawableString(){

	string aux;
	lock();
	int n = q.size();
	if( isThreadRunning() && n > 0 ){
		ofxSimpleHttpResponse * r = q.front();
		float timeSoFar = ofGetElapsedTimef() - r->timeDowloadStarted; //seconds
		float timeRemaining = 0.0f;
		string soFarTimeUnit = "sec";

		if (timeSoFar > 60.0f){
			timeSoFar /= 60.0f;
			soFarTimeUnit = "min";
		}

		if(r->downloadedSoFar > 100){
			timeRemaining = (timeSoFar / r->downloadProgress) - timeSoFar;
		}
		string remTime;
		string serverSize;

		if(r->serverReportedSize != -1){
			serverSize = bytesToHumanReadable(r->serverReportedSize, 2);
			remTime = secondsToHumanReadable(timeRemaining, 1);
		}else{
			remTime = "unknown";
			if (r->downloadedSoFar == 0){
				string anim[] = {"   ", ".  ", ".. ", "...", " ..", "  ."}; //6 anim states
				serverSize = "waiting for server " + anim[ int(0.2 * ofGetFrameNum())%6 ];
			}else{
				serverSize = "";
			}
		}
		string spa = "      ";

		aux = "//// ofxSimpleHttp now fetching //////////////////////////////////\n"
		"//\n"
		"//   URL: " + r->url + "\n" +
		string( r->downloadToDisk ? "//   Save To: " + r->absolutePath + "\n" : "") +
		string(serverSize.length() ?
		"//   Progress:                 " + spa + string((r->downloadProgress >= 0.0) ? ofToString(100.0f * r->downloadProgress, 2) : "") + "%\n" : "") +
		string(serverSize.length() ?
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


string ofxSimpleHttp::bytesToHumanReadable(long long bytes, int decimalPrecision){
	string ret;
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


string ofxSimpleHttp::secondsToHumanReadable(float secs, int decimalPrecision){
	string ret;
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
	string aux = drawableString();
	ofDrawBitmapString(aux, x + 3, y + 12 );
}


void ofxSimpleHttp::draw(float x, float y) {
	draw(x,y, ofGetWidth() -x, 100);
}


void ofxSimpleHttp::drawMinimal(float x, float y, bool withBg, ofColor fontColor, ofColor bgColor) {
	string aux = minimalDrawableString();
	if(withBg){
		ofDrawBitmapStringHighlight(aux, x + 3, y + 12, bgColor, fontColor );
	}else{
		ofDrawBitmapString(aux, x + 3, y + 12);
	}
}


string removeInvalidCharacters(string input){
	static char invalidChars[] = {'?', '\\', '/', '*', '<', '>', '"', ':' };
	int howMany = sizeof(invalidChars) / sizeof(invalidChars[0]);
	char replacementChar = '_';
	for(int i = 0; i < howMany; i++){
		std::replace( input.begin(), input.end(), invalidChars[i], replacementChar);
	}
	return input;
}

string ofxSimpleHttp::extractFileFromUrl(string url){
	int found = url.find_last_of("/");
	string file = url.substr(found + 1);
	file = removeInvalidCharacters(file);
	if (file.length() == 0){
		file = OFX_SIMPLEHTTP_UNTITLED_FILENAME;
	}
	return file;
}


string ofxSimpleHttp::extractExtensionFromFileName(string fileName){
	int found = fileName.find_last_of(".");
	string file;
	if (found > 0){
		file = fileName.substr(found + 1, fileName.size() - found);
	}else{
		file = "";
	}

	return file;
}


void ofxSimpleHttp::fetchURL(string url, bool notifyOnSuccess){

	if (queueLenEstimation >= maxQueueLen){
		ofLogError("ofxSimpleHttp", "fetchURL can't do that, queue is too long already (%d)!\n", queueLenEstimation );
		return;
	}

	ofxSimpleHttpResponse *response = new ofxSimpleHttpResponse();
	response->who = this;
	response->url = url;
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


ofxSimpleHttpResponse ofxSimpleHttp::fetchURLBlocking(string  url){

	response.url = url;
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

void ofxSimpleHttp::fetchURLToDisk(string url, string expectedSha1, bool notifyOnSuccess, string dirWhereToSave){

	if (queueLenEstimation >= maxQueueLen){
		ofLogError("ofxSimpleHttp", "fetchURL can't do that, queue is too long already (%d)!\n", queueLenEstimation );
		return;
	}

	ofDirectory d;
	d.createDirectory(dirWhereToSave, true, true); //create the download dir first

	string savePath = dirWhereToSave == "" ? extractFileFromUrl(url) : ofToDataPath(dirWhereToSave, true) + "/" + extractFileFromUrl(url);

	ofxSimpleHttpResponse *response = new ofxSimpleHttpResponse();
	response->who = this;
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
		}catch(Exception e){
			ofLogError() << "ofxSimpleHttp: cant start thread!" << e.what();
		}
	}
}

void ofxSimpleHttp::fetchURLToDisk(string url, bool notifyOnSuccess, string dirWhereToSave){
	fetchURLToDisk(url, "", notifyOnSuccess, dirWhereToSave);
}


ofxSimpleHttpResponse ofxSimpleHttp::fetchURLtoDiskBlocking(string  url, string dirWhereToSave, string expectedSha1){

	string savePath = dirWhereToSave == "" ? extractFileFromUrl(url) : ofToDataPath(dirWhereToSave) + "/" + extractFileFromUrl(url);

	ofDirectory d;
	d.createDirectory(dirWhereToSave, true, true); //create the download dir first

	response.absolutePath = savePath;
	response.url = url;
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

	bool ok = FALSE;
	ofstream myfile;
	bool fileIsAlreadyHere = false;

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
					ofLogVerbose() << "ofxSimpleHttp: about to download " << resp->url << " but a file with same name and correct checksum is already here!";
					ofLogVerbose() << "ofxSimpleHttp: skipping download (" << resp->expectedChecksum << ")";
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
					ofLogVerbose() << "ofxSimpleHttp: about to download "<< resp->url << " but a file with same name and (size > 0) is already here!";
					ofLogVerbose() << "ofxSimpleHttp: skipping download (missing checksum)";
				}
				f.close();
			}
		}
	}

	if (!fileIsAlreadyHere){ //if file is not here, download it!

		string protocol = resp->url.substr(0,7);

		if(saveToDisk || protocol == "file://"){
			myfile.open( resp->absolutePath.c_str(), ios_base::binary );
		}

		if(protocol == "file://"){

			string srcFile = resp->url.substr(7, resp->url.length() - 7);

#ifdef TARGET_WIN32
			ofStringReplace(srcFile, "\\", "/"); //for windows, replace escaped backslashes
#endif
			//ofFile::copyFromTo(srcFile, resp->absolutePath);

			ofFile f;
			bool openOK = f.open(ofToDataPath(srcFile, true), ofFile::ReadOnly);
			resp->downloadSpeed = 0;
			resp->downloadedBytes = 0;
			if(f.exists()){
				uint64_t size = f.getSize();
				resp->serverReportedSize = size;
				resp->timeDowloadStarted = ofGetElapsedTimef();

				std::ifstream rs (ofToDataPath(srcFile, true).c_str(), std::ifstream::binary);
				streamCopyWithProgress(rs, myfile, resp->serverReportedSize, resp->downloadedSoFar,
									   resp->downloadProgress, resp->downloadSpeed, resp->downloadCanceled);
				resp->ok = true;
				resp->status = 200;
				resp->timeTakenToDownload = ofGetElapsedTimef() - resp->timeDowloadStarted;
				rs.close();
				ok = TRUE;
			}else{
				resp->ok = false;
				resp->status = 404; //assume not found? todo!
				resp->reasonForStatus = "Can't load File!!";
				resp->timeTakenToDownload = 0;
				ok = TRUE;
			}
			f.close();
			myfile.close();

		}else{

			//myfile.open(ofToDataPath(filename).c_str()); //for not binary?

			HTTPClientSession * session = NULL;

			try {

				ofHttpRequest request(resp->url, resp->url);
				URI uri(request.url);
				std::string path(uri.getPathAndQuery());
				if (path.empty()) path = "/";

				if(uri.getScheme()=="https"){
					session = new HTTPSClientSession(uri.getHost(), uri.getPort());//,context);
				}else{
					session = new HTTPClientSession(uri.getHost(), uri.getPort());
				}

				if(useProxy){
					session->setProxy(proxyHost, proxyPort);
					if(proxyLogin.size() && proxyPassword.size()){
						session->setProxyCredentials(proxyLogin, proxyPassword);
					}
				}

				HTTPRequest req(HTTPRequest::HTTP_GET, path, HTTPMessage::HTTP_1_1);
				req.set( "User-Agent", userAgent.c_str() );

				//add custom headers to the request
				map<string, string>::iterator it = customHttpHeaders.begin();
				while(it != customHttpHeaders.end()){
					req.set( it->first, it->second );
					++it;
				}

				session->setTimeout( Poco::Timespan(timeOut,0) );
				try{
					session->sendRequest(req);
				}catch(Exception e){
					ofLogWarning() << "ofxSimpleHttp session send request exception: " << e.what() << " - " << request.url;
				}

				HTTPResponse res;
				istream& rs = session->receiveResponse(res);

				resp->status = res.getStatus();
				try {
					resp->timestamp = res.getDate();
				} catch (Exception& exc) {
					resp->timestamp = 0;
				}

				resp->reasonForStatus = res.getReasonForStatus( res.getStatus() );
				resp->contentType = res.getContentType();
				resp->serverReportedSize = res.getContentLength();
				resp->timeDowloadStarted = ofGetElapsedTimef();

				string msg = "downloadURL(" + resp->fileName + ") >> Server doesn't report download size...";
				if (resp->serverReportedSize == -1){
					ofLogWarning("ofxSimpleHttp", msg);
				}

				if (resp->serverReportedSize == 0){
					ofLogWarning("ofxSimpleHttp", "Server reports file size 0 bytes!");
				}
				msg = "downloadURL() >> about to start download (" + resp->fileName + ", " + ofToString(res.getContentLength()) + " bytes)";
				ofLogVerbose("ofxSimpleHttp", msg);
				msg = "downloadURL() >> server reports request status: " +ofToString(resp->status) + " - ", resp->reasonForStatus + ")";
				ofLogVerbose("ofxSimpleHttp", msg );

				int copySize = 0;
				if(saveToDisk){
					copySize = streamCopyWithProgress(rs, myfile, resp->serverReportedSize, resp->downloadedSoFar,
													  resp->downloadProgress, resp->downloadSpeed, resp->downloadCanceled);
				}else{
					copySize = copyToStringWithProgress(rs, resp->responseBody, resp->serverReportedSize, resp->downloadedSoFar,
														resp->downloadProgress, resp->downloadSpeed, resp->downloadCanceled);
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
							ofLogVerbose() << "ofxSimpleHttp: downloaded OK but Checksum FAILED";
							ofLogVerbose() << "ofxSimpleHttp: SHA1 was meant to be: " << resp->expectedChecksum;
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
							string msg = "downloadURL() downloaded to " + resp->fileName;
							ofLogNotice("ofxSimpleHttp", msg);
							//ask the filesystem what is the real size of the file
							ofFile file;
							try{
								file.open(resp->absolutePath.c_str());
								resp->downloadedBytes = file.getSize();
								file.close();
							}catch(Exception& exc){
								string msg = "downloadURL(" + resp->fileName + ") >> Exception at file.open: " + exc.displayText();
								ofLogError("ofxSimpleHttp", msg );
							}
						}else{
							resp->downloadedBytes = resp->responseBody.size();
						}

						if(resp->timeTakenToDownload > 0.05){
							resp->avgDownloadSpeed = (resp->downloadedBytes ) / resp->timeTakenToDownload; //kb/sec
						}
						if (avgDownloadSpeed == 0.0f){
							avgDownloadSpeed = resp->avgDownloadSpeed;
						}else{
							avgDownloadSpeed = avgDownloadSpeed * 0.75f + 0.25f * resp->avgDownloadSpeed;
						}

						//check download file size missmatch
						if ( resp->serverReportedSize > 0 && resp->serverReportedSize != resp->downloadedBytes) {
							string msg;

							if (resp->downloadedBytes == 0){
								if (resp->timeTakenToDownload > timeOut){
									msg = "downloadURLtoDiskBlocking() >> TimeOut! (" + resp->fileName + ")";
									resp->reasonForStatus = "Request TimeOut";
									resp->status = 408;
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

							if (resp->status == 200){ //mmm
								resp->ok = true;
							}else{
								resp->ok = false;
							}
						}
						ofLogVerbose() << "ofxSimpleHttp: download finished! " << resp->url << " !";
						ok = TRUE;
					}

					if (copySize == 0){
						ofLogError() << "downloaded file is empty!";
						resp->ok = false;
						resp->reasonForStatus += " / server file is empty!";
					}

				}else{
					resp->ok = false;
					resp->reasonForStatus = "Unknown exception at streamCopy";
					resp->status = -1;
					ok = FALSE;
				}

				//last check for OK flag
				if(!resp->checksumOK){
					resp->ok = false;
					resp->reasonForStatus += " / SHA1 missmatch!";
				}

			}catch(Exception& exc){

				if (session) session->reset();
				myfile.close();
				string msg = "downloadURL(" + resp->fileName +  ") >> Exception: " + exc.displayText();
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
				ofLogError("ofxSimpleHttp", "failed to download " + resp->url );
				ofLogError("ofxSimpleHttp", msg );

			}
            
            if(session != NULL){
                delete session;
                // session = NULL;
            }
		}
	}else{
		resp->timeTakenToDownload = 0;
		ok = TRUE;
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
													  float & progress, float & speed, const bool &cancel){

	std::streamsize len = 0;

	try{
		Buffer<char> buffer(COPY_BUFFER_SIZE);

		istr.read(buffer.begin(), COPY_BUFFER_SIZE);
		std::streamsize n = istr.gcount();
		float avgSpeed = 0;

		while (n > 0 && !cancel){
			float t1 = ofGetElapsedTimef();
			currentBytes = len;
			len += n;
			out.write(buffer.begin(), static_cast<std::string::size_type>(n));
			if (istr){
				istr.read(buffer.begin(), COPY_BUFFER_SIZE);
				n = istr.gcount();
				if (istr.fail() && !istr.eof()){
					ofLogError("ofxSimpleHttp", "streamCopyWithProgress() >> iostream Fail!");
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
			float time = (ofGetElapsedTimef() - t1);

			if(speedLimit > 0.0f){ //apply speed limit if defined
				float expectedTimeToDLCopyBuffer = COPY_BUFFER_SIZE / (speedLimit * 1024.0f);
				//ofLog() << "expected " << expectedTimeToDLCopyBuffer << " and took " << time;
				if (time < expectedTimeToDLCopyBuffer){
					float sleepTime = (expectedTimeToDLCopyBuffer - time);
					ofSleepMillis(sleepTime * 1000.0f);
					//ofLog() << "sleeping a bit to match speed limit " << sleepTime;
				}
				time = (ofGetElapsedTimef() - t1);
			}
			float newSpeed = 0;
			if(time > 0.0f){
				newSpeed = (n) / time;
			}
			avgSpeed = 0.5 * newSpeed + 0.5 * avgSpeed;
			speed = avgSpeed;
		}
	}catch(Exception& exc){
		ofLogError("ofxSimpleHttp", "streamCopyWithProgress() >> Exception: %s", exc.displayText().c_str() );
		return -1;
	}
	return len;
}

//todo this is silly, dup code with method above!
std::streamsize ofxSimpleHttp::copyToStringWithProgress(std::istream& istr, std::string& str,
														std::streamsize totalBytes,
														std::streamsize &currentBytes,
														float & progress, float & speed, const bool &cancel){

	Buffer<char> buffer(COPY_BUFFER_SIZE);
	std::streamsize len = 0;

	try{
		istr.read(buffer.begin(), COPY_BUFFER_SIZE);
		std::streamsize n = istr.gcount();
		float avgSpeed = 0;

		while (n > 0 && !cancel){
			float t1 = ofGetElapsedTimef();
			currentBytes = len;
			len += n;
			str.append(buffer.begin(), static_cast<std::string::size_type>(n));
			if (istr){
				istr.read(buffer.begin(), COPY_BUFFER_SIZE);
				n = istr.gcount();
				if (istr.fail() && !istr.eof()){
					ofLogError("ofxSimpleHttp", "copyToStringWithProgress() >> Fail");
					return -1;
				}
			}else{
				n = 0;
			}
			progress = float(len) / float(totalBytes);
			float time = (ofGetElapsedTimef() - t1);
			float newSpeed = 0;
			if(time > 0.0f){
				newSpeed = (COPY_BUFFER_SIZE ) / time ;
			}
			avgSpeed = 0.5 * newSpeed + 0.5 * avgSpeed;
			speed = avgSpeed;
			
		}
	}catch(Exception& exc){
		ofLogError("ofxSimpleHttp", "copyToStringWithProgress() >> Exception: %s", exc.displayText().c_str() );
		return -1;
	}
	return len;
}
