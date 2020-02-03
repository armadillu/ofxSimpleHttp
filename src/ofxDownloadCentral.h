//
//  ofxDownloadCentral.h
//  emptyExample
//
//  Created by Oriol Ferrer Mesià on 14/03/14.
//
//

#pragma once
#include "ofMain.h"
#include "ofxBatchDownloader.h"
#include "ofxSimpleHttp.h"
#include "ofEvents.h"
#include "ofEventUtils.h"

/*
// WHAT IS THIS?

 Centralised downloads from any number of objects.
 creates a download queue, each object that requests a download list 
 will be notified when its downloads are ready.
 this is meant mainly to download assets on demand, telling its owner 
 when the asset is ready to load.

 // HOW IS THIS MEANT TO BE USED ?

 1 - define a central downloads object
	ofxDownloadCentral dlc;

 2 - implement a notification callback
	void downloadsFinished(ofxBatchDownloaderReport &report){}

 3 - fill in a list of url with assets you need downloaded
	std::vector<std::string> urlList;
 
 4 - supply a list the download
 
	dlc.downloadResources( urlList, this, &myClass::downloadsFinished, destinationFolder );

	it will all be downloaded in a bg thread
	you will be notified from the main thread when they are done
	you will get a report
 
 6 - start the download
	
	startDownloading();
 
 7 - update the dlc object
	
	dlc.update();
	
	and wait for the notification to arrive

 */

class ofxBatchDownloader;

class ofxDownloadCentral{

	public:

		ofxDownloadCentral();
		~ofxDownloadCentral();

		void update();
		void draw(float x, float y, bool drawAllPending = false, bool detailedDownloadInfo = true);
		std::string getDrawableInfo(bool drawAllPending = false, bool detailedDownloadInfo = true); //draw with a monospaced font!

		void cancelCurrentDownload();
		void cancelAllDownloads();

		bool isBusy();
		int getNumPendingDownloads();
		int getNumActiveDownloads();

		void setVerbose(bool b);
		void setNeedsChecksumMatchToSkipDownload(bool needs);	//if downloaded file is on disk, should I assume its good?
															//or only if you provided sha1 checksum and it matches

		void setSpeedLimit(float KB_per_sec);
		void setTimeOut(float timeoutSecs);
		void setMaxConcurrentDownloads(int numConcurrentDownloads);
		void setMaxRetries(int maxRet);

		void setChecksumType(ofxChecksum::Type type); //when you supply a checksum to compare a downloaded file against, what type will it be?
		void setProxyConfiguration(const ofxSimpleHttp::ProxyConfig & c);
		void setCredentials(const std::string& user, const std::string& password);

		void setCopyBufferSize(float bufferInKb);

		void setIdleTimeAfterEachDownload(float seconds);		//wait a bit before notifying once the dowload is over
		void setMaxURLsToList(int max);

		void startDownloading();

		///////////////////////////////////////////////////////////////////////////////
		template <typename ArgumentsType, class ListenerClass>
		void downloadResources(	const std::string & url,
							   ListenerClass  * listener,
							   void (ListenerClass::*listenerMethod)(ArgumentsType&),
							   const std::string & destinationFolder = "ofxDownloadCentral_downloads"
							   ){

			std::vector<std::string> list;
			list.push_back(url);
			downloadResources(list, listener, listenerMethod, destinationFolder);
		}

		///////////////////////////////////////////////////////////////////////////////
		template <typename ArgumentsType, class ListenerClass>
		void downloadResources( const std::string & url,
								const std::string & sha1String,
							   ListenerClass  * listener,
							   void (ListenerClass::*listenerMethod)(ArgumentsType&),
							   const std::string & destinationFolder = "ofxDownloadCentral_downloads"
							   ){

			std::vector<std::string> list;
			std::vector<std::string> shas;
			list.push_back(url);
			shas.push_back(sha1String);
			downloadResources(list, shas, listener, listenerMethod, destinationFolder);
		}


		///////////////////////////////////////////////////////////////////////////////
		template <typename ArgumentsType, class ListenerClass>
		void downloadResources(	const std::vector<std::string> & urlList,
								const std::vector<std::string> & sha1List,
								ListenerClass  * listener,
								void (ListenerClass::*listenerMethod)(ArgumentsType&),
								const std::string & destinationFolder = "ofxDownloadCentral_downloads"
							   ){

			if (urlList.size() > 0 ){
				ofxBatchDownloader * d = new ofxBatchDownloader();
				d->setProxyConfiguration(proxyConfig);
				if(credentials.first.size() || credentials.second.size()){
					d->setCredentials(credentials.first, credentials.second);
				}
				if(timeOut > 0.0f) d->setTimeOut(timeOut);
				if(speedLimit > 0.0f) d->setSpeedLimit(speedLimit);

				d->setDownloadFolder(destinationFolder);
				d->setVerbose(verbose);
				d->setNeedsChecksumMatchToSkipDownload(onlySkipDownloadIfChecksumMatches);
				d->setChecksumType(checksumType);
				d->setCopyBufferSize(copyBufferSize);
				d->setIdleTimeAfterEachDownload(idleTimeAfterDownload);
				d->addResourcesToDownloadList(urlList, sha1List);
				d->setMaxRetries(maxRetries);
				ofAddListener(d->resourcesDownloadFinished, listener, listenerMethod); //set the notification to hit our original caller
				if(downloaders.size() == 0){
					downloadStartTime = ofGetElapsedTimef();
					downloadedSoFar = 0;
				}else{
					downloadStartJobsNumber++;
				}
				downloaders.push_back(d);
			}
		}

		///////////////////////////////////////////////////////////////////////////////
		template <typename ArgumentsType, class ListenerClass>
		void downloadResources(const std::vector<std::string> & urlList,
							   ListenerClass  * listener,
							   void (ListenerClass::*listenerMethod)(ArgumentsType&),
							   const std::string & destinationFolder = "ofxDownloadCentral_downloads"
							   ){

			std::vector<std::string> shas;
			downloadResources(urlList, shas, listener, listenerMethod, destinationFolder);
		}

	protected:

		void startQueue();

		std::vector<ofxBatchDownloader*>		downloaders;
		std::vector<ofxBatchDownloader*>		activeDownloaders;

		bool									busy;
		bool									verbose;
		bool									onlySkipDownloadIfChecksumMatches;
		float								idleTimeAfterDownload; //float sec
		float								downloadStartTime;
		std::streamsize						downloadedSoFar; //bytes
		int									downloadStartJobsNumber;

		int									failedJobsStartNumber;
		int									failedJobsSoFar;
		int 								maxRetries = 5;

		std::map<int, float>					avgSpeed;	 //bytes/sec
		int									maxURLsToList;

		int									maxConcurrentDownloads;
		float								speedLimit;
		float								timeOut;
		float								copyBufferSize = 1024.0f; //Kb
		ofxChecksum::Type 					checksumType = ofxChecksum::Type::SHA1;

		ofxSimpleHttp::ProxyConfig			proxyConfig;
		std::pair<std::string,std::string> 	credentials;


};


