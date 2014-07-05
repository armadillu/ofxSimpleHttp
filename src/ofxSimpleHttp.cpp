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

static bool pocoHttpInited = false;

ofxSimpleHttp::ofxSimpleHttp(){
	timeOut = 10;
	queueLenEstimation = 0;
	maxQueueLen = 1000;
	timeToStop = false;
	userAgent = "ofxSimpleHttp (Poco Powered)";
	acceptString = "";
	notifyFromMainThread = true;
	onlySkipDownloadIfChecksumMatches = false;
	idleTimeAfterEachDownload = 0.0;
	if (!pocoHttpInited){ //only register once!
		HTTPStreamFactory::registerFactory();
		HTTPSStreamFactory::registerFactory();
		SharedPtr<PrivateKeyPassphraseHandler> pConsoleHandler = new KeyConsoleHandler(false);
		SharedPtr<InvalidCertificateHandler> pInvalidCertHandler = new ConsoleCertificateHandler(true);
		Context::Ptr pContext = new Context(Context::CLIENT_USE, "", Context::VERIFY_NONE);
		SSLManager::instance().initializeClient(pConsoleHandler, pInvalidCertHandler, pContext);
		pocoHttpInited = true;
	}
}

ofxSimpleHttp::~ofxSimpleHttp(){

	timeToStop = true;	//lets flag the thread so that it doesnt try access stuff while we delete things around
	stopCurrentDownload(true);

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


void ofxSimpleHttp::setAcceptString( string newAcceptString ){
	acceptString = newAcceptString;
}


void ofxSimpleHttp::setMaxQueueLength(int len){
	maxQueueLen = len;
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
				ofLogVerbose("ofxSimpleHttp", "stopCurrentDownload() >> about to stop download of %s...", r->fileName.c_str() );
				try{
					r->emptyWholeQueue = emptyQueue;
					r->downloadCanceled = true;
					//if ( r->session != NULL ){
						//cout << "aboritng session " << r->session;
					//	r->session->abort();
					//}
				}catch(Exception& exc){
					ofLogError("ofxSimpleHttp", "stopCurrentDownload(%s) >> Exception: %s", r->fileName.c_str(), exc.displayText().c_str() );
				}
			}
		}
	unlock();
}


string ofxSimpleHttp::drawableString(){

	string aux;
	lock();
	int n = q.size();
	if( isThreadRunning() && n > 0 ){
		ofxSimpleHttpResponse * r = q.front();
		float speed = r->downloadSpeed / 1024.0f;
		string speedUnit = " Mb/sec";
		if(speed < 1.0){
			speed *= 1024.0f;
			speedUnit = " Kb/sec";
		}
		float timeSoFar = ofGetElapsedTimef() - r->timeTakenToDownload; //seconds
		float timeRemaining = 0.0f;
		if(r->downloadProgress > 0.001){
			timeRemaining = (timeSoFar / r->downloadProgress) - timeSoFar;
		}
		string remtimeUnit = "sec";
		string soFarTimeUnit = "sec";
		if (timeRemaining > 60.0){
			timeRemaining /= 60.0f;
			remtimeUnit = "min";
		}
		if (timeSoFar > 60.0){
			timeSoFar /= 60.0f;
			soFarTimeUnit = "min";
		}

		aux = "//// ofxSimpleHttp fetching //////////////////////////////////////\n"
		"//\n"
		"//   " + r->url + "\n" +
		"//   Progress: " + string((r->downloadProgress >= 0.0) ? ofToString(100.0f * r->downloadProgress, 2) : "") + "%\n" +
		"//   Server Reported Size: " + string( r->serverReportedSize != -1 ? ofToString(r->serverReportedSize / float(1024  * 1024), 1) : "-" )+ "Mb\n" +
		"//   Download Speed: " + ofToString(speed, 2) + speedUnit + "\n" +
		"//   Time Taken so far: " + ofToString(timeSoFar, 1) + soFarTimeUnit + "\n" +
		"//   Estimated Remaining Time: " + ofToString(timeRemaining, 1) + remtimeUnit + "\n" +
		"//   Queue Size: " + ofToString(n) + "\n" +
		"//  \n";
		aux += "//////////////////////////////////////////////////////////////////\n";
	}else{
		aux = "//// ofxSimpleHttp idle... ////////////////////////////////////////\n";
	}
	unlock();
	return aux;
}


void ofxSimpleHttp::draw(float x, float y , float w , float h  ){

	string aux = drawableString();
//	for(int i = 0; i < aux.length(); i+= w / 8){	//break up the string with \n to fit in supplied width
//		aux.insert(i, "\n");
//	}
	ofSetColor(0,127,255);
	ofDrawBitmapString(aux, x + 3, y + 12 );
}

void ofxSimpleHttp::draw(float x, float y){
	draw(x,y, ofGetWidth() -x, 100);
}


string ofxSimpleHttp::extractFileFromUrl(string url){
	int found = url.find_last_of("/");
	string file = url.substr(found + 1);
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
		startThread(true);
	}
}

void ofxSimpleHttp::fetchURLToDisk(string url, bool notifyOnSuccess, string dirWhereToSave){
	fetchURLToDisk(url, "", notifyOnSuccess, dirWhereToSave);
}


ofxSimpleHttpResponse ofxSimpleHttp::fetchURLtoDiskBlocking(string  url, string dirWhereToSave){

	string savePath = dirWhereToSave == "" ? extractFileFromUrl(url) : ofToDataPath(dirWhereToSave) + "/" + extractFileFromUrl(url);

	ofDirectory d;
	d.createDirectory(dirWhereToSave, true, true); //create the download dir first

	response.absolutePath = savePath;
	response.url = url;
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

	bool ok;
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
					ofLogVerbose() << "ofxSimpleHttp: about to download "<< resp->url << " but a file with same name and correct checksum is already here!";
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

		myfile.open( resp->absolutePath.c_str(), ios_base::binary );
		//myfile.open(ofToDataPath(filename).c_str()); //for not binary?

		try {

			ofHttpRequest request(resp->url, resp->url);
			URI uri(request.url);
			std::string path(uri.getPathAndQuery());
			if (path.empty()) path = "/";


			HTTPClientSession * session;

			if(uri.getScheme()=="https"){
				session = new HTTPSClientSession(uri.getHost(), uri.getPort());//,context);
			}else{
				session = new HTTPClientSession(uri.getHost(), uri.getPort());
			}

			//resp->session = &session;

			HTTPRequest req(HTTPRequest::HTTP_GET, path, HTTPMessage::HTTP_1_1);
			req.set( "User-Agent", userAgent.c_str() );

			session->setTimeout( Poco::Timespan(timeOut,0) );
			session->sendRequest(req);

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
			resp->timeTakenToDownload = ofGetElapsedTimef();

			if (resp->serverReportedSize == -1) ofLogWarning("ofxSimpleHttp", "downloadURL(%s) >> Server doesn't report download size...", resp->fileName.c_str() );
			ofLogVerbose("ofxSimpleHttp", "downloadURL() >> about to start download (%s, %d bytes)", resp->fileName.c_str(), res.getContentLength() );
			ofLogVerbose("ofxSimpleHttp", "downloadURL() >> server reports request status: (%d-%s)", resp->status, resp->reasonForStatus.c_str() );

			//StreamCopier::copyStream(rs, myfile); //write to file here!
			if(saveToDisk){
				streamCopyWithProgress(rs, myfile, resp->serverReportedSize, resp->downloadProgress, resp->downloadSpeed, resp->downloadCanceled);
			}else{
				copyToStringWithProgress(rs, resp->responseBody, resp->serverReportedSize, resp->downloadProgress, resp->downloadSpeed, resp->downloadCanceled);
			}

			resp->timeTakenToDownload = ofGetElapsedTimef() - resp->timeTakenToDownload;
			if (resp->expectedChecksum.length() > 0){
				resp->checksumOK = ofxChecksum::sha1(resp->absolutePath, resp->expectedChecksum);
				if(!resp->checksumOK){
					ofLogVerbose() << "ofxSimpleHttp: downloaded OK but Checksum FAILED";
					ofLogVerbose() << "ofxSimpleHttp: SHA1 was meant to be: " << resp->expectedChecksum;
				}
			}

			resp->downloadSpeed = 0;
			resp->avgDownloadSpeed = 0;
			resp->downloadedBytes = 0;
			//resp->session = NULL;
			delete session;
			session = NULL;

			if(saveToDisk){
				myfile.close();
			}

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

			}else{

				if(saveToDisk){
					ofLogNotice("ofxSimpleHttp", "downloadURL() downloaded to %s", resp->fileName.c_str() );
				}

				if( saveToDisk ){
					//ask the filesystem what is the real size of the file
					ofFile file;
					try{
						file.open(resp->absolutePath.c_str());
						resp->downloadedBytes = file.getSize();
						file.close();
					}catch(Exception& exc){
						ofLogError("ofxSimpleHttp", "downloadURL(%s) >> Exception at file.open: %s", resp->fileName.c_str(), exc.displayText().c_str() );

					}
				}else{
					resp->downloadedBytes = resp->responseBody.size();
				}

				resp->avgDownloadSpeed = (resp->downloadedBytes / 1024.) / resp->timeTakenToDownload; //kb/sec


				//check download file size missmatch
				if ( resp->serverReportedSize > 0 && resp->serverReportedSize !=  resp->downloadedBytes) {

					ofLogWarning("ofxSimpleHttp", "downloadURLtoDiskBlocking() >> Download size mismatch (%s) >> Server: %d Downloaded: %d",
									 resp->fileName.c_str(), resp->serverReportedSize, resp->downloadedBytes );
					resp->reasonForStatus = "Download size mismatch!";
					resp->status = -1;
					resp->ok = false;

				}else{

					if (resp->status == 200){
						resp->ok = true;
					}else{
						resp->ok = false;
					}
				}

				ofLogVerbose() << "ofxSimpleHttp: download finished! " << resp->url << " !";
				ok = TRUE;

			}

		}catch(Exception& exc){

			myfile.close();
			ofLogError("ofxSimpleHttp", "downloadURL(%s) >> Exception: %s", resp->fileName.c_str(), exc.displayText().c_str() );
			resp->reasonForStatus = exc.displayText();
			resp->ok = false;
			resp->status = -1;
			ok = false;
			ofLogError() << "ofxSimpleHttp: failed to download " << resp->url << " !";
		}
	}

	//enqueue the operation result!
	if (sendResultThroughEvents ){

		if ( resp->notifyOnSuccess ){

			if(idleTimeAfterEachDownload > 0.0){
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

	lock();
	if(responsesPendingNotification.size()){
		ofxSimpleHttpResponse r = responsesPendingNotification.front();
		responsesPendingNotification.pop();
		ofNotifyEvent( httpResponse, r, this );
	}
	unlock();
}


std::streamsize ofxSimpleHttp::streamCopyWithProgress(std::istream & istr, std::ostream & out, std::streamsize totalBytes,float & progress, float & speed, const bool &cancel){

	std::streamsize len = 0;

	try{
		Buffer<char> buffer(COPY_BUFFER_SIZE);

		istr.read(buffer.begin(), COPY_BUFFER_SIZE);
		std::streamsize n = istr.gcount();
		float avgSpeed = 0;

		while (n > 0 && !cancel){
			float t1 = ofGetElapsedTimef();
			len += n;
			out.write(buffer.begin(), static_cast<std::string::size_type>(n));
			if (istr){
				istr.read(buffer.begin(), COPY_BUFFER_SIZE);
				n = istr.gcount();
			}else{
				n = 0;
			}
			if (totalBytes > 0){
				progress = float(len) / float(totalBytes);
			}else{
				progress = 0.0;
			}
			float time = (ofGetElapsedTimef() - t1);
			float newSpeed = (n / 1024.0f ) / time;
			avgSpeed = 0.1 * newSpeed + 0.9 * avgSpeed;
			speed = avgSpeed;
		}
	}catch(Exception& exc){
		ofLogError("ofxSimpleHttp", "streamCopyWithProgress() >> Exception: %s", exc.displayText().c_str() );
	}
	return len;
}


std::streamsize ofxSimpleHttp::copyToStringWithProgress(std::istream& istr, std::string& str,std::streamsize totalBytes, float & progress, float & speed, const bool &cancel){

	Buffer<char> buffer(COPY_BUFFER_SIZE);
	std::streamsize len = 0;

	try{
		istr.read(buffer.begin(), COPY_BUFFER_SIZE);
		std::streamsize n = istr.gcount();
		float avgSpeed = 0;

		while (n > 0 && !cancel){
			float t1 = ofGetElapsedTimef();
			len += n;
			str.append(buffer.begin(), static_cast<std::string::size_type>(n));
			if (istr){
				istr.read(buffer.begin(), COPY_BUFFER_SIZE);
				n = istr.gcount();
			}else{
				n = 0;
			}
			progress = float(len) / float(totalBytes);
			float time = (ofGetElapsedTimef() - t1);
			float newSpeed = (COPY_BUFFER_SIZE / 1024.0f ) / time ;
			avgSpeed = 0.5 * newSpeed + 0.5 * avgSpeed;
			speed = avgSpeed;

		}
	}catch(Exception& exc){
		ofLogError("ofxSimpleHttp", "copyToStringWithProgress() >> Exception: %s", exc.displayText().c_str() );
	}
	return len;
}
