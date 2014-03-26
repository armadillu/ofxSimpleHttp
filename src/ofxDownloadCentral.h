//
//  ofxDownloadCentral.h
//  emptyExample
//
//  Created by Oriol Ferrer Mesià on 14/03/14.
//
//

#ifndef emptyExample_ofxDownloadCentral_h
#define emptyExample_ofxDownloadCentral_h

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
	vector<string> urlList;
 
 4 - start the download
 
	dlc.downloadResources( urlList, this, &myClass::downloadsFinished, destinationFolder );

	it will all will be downloaded in a bg thread
	you will be notified from the main thread when they are done
	you will get a report

 */

class ofxDownloadCentral{

	public:

		ofxDownloadCentral();
		~ofxDownloadCentral();

		void update();
		void draw(float x, float y, bool drawAllPending = false);

		void cancelCurrentDownload();
		void cancelAllDownloads();

		void setVerbose(bool b);
		void setNeedsChecksumMatchToSkipDownload(bool needs);

		///////////////////////////////////////////////////////////////////////////////
		template <typename ArgumentsType, class ListenerClass>
		void downloadResources(	string url,
							   ListenerClass  * listener,
							   void (ListenerClass::*listenerMethod)(ArgumentsType&),
							   string destinationFolder = "ofxDownloadCentral_downloads"
							   ){

			vector<string> list;
			list.push_back(url);
			downloadResources(list, listener, listenerMethod, destinationFolder);
		}

		///////////////////////////////////////////////////////////////////////////////
		template <typename ArgumentsType, class ListenerClass>
		void downloadResources(	string url,
								string sha1String,
							   ListenerClass  * listener,
							   void (ListenerClass::*listenerMethod)(ArgumentsType&),
							   string destinationFolder = "ofxDownloadCentral_downloads"
							   ){

			vector<string> list;
			vector<string> shas;
			list.push_back(url);
			shas.push_back(sha1String);
			downloadResources(list, shas, listener, listenerMethod, destinationFolder);
		}


		///////////////////////////////////////////////////////////////////////////////
		template <typename ArgumentsType, class ListenerClass>
		void downloadResources(	vector<string>urlList,
								vector<string>sha1List,
								ListenerClass  * listener,
								void (ListenerClass::*listenerMethod)(ArgumentsType&),
								string destinationFolder = "ofxDownloadCentral_downloads"
							   ){

			ofxBatchDownloader * d = new ofxBatchDownloader();
			d->setDownloadFolder(destinationFolder);
			d->setVerbose(verbose);
			d->setNeedsChecksumMatchToSkipDownload(onlySkipDownloadIfChecksumMatches);
			d->addResourcesToDownloadList(urlList, sha1List);
			ofAddListener(d->resourcesDownloadFinished, listener, listenerMethod); //set the notification to hit our original caller
			downloaders.push_back(d);
			if (!busy) startQueue();
		}

		///////////////////////////////////////////////////////////////////////////////
		template <typename ArgumentsType, class ListenerClass>
		void downloadResources(	vector<string>urlList,
							   ListenerClass  * listener,
							   void (ListenerClass::*listenerMethod)(ArgumentsType&),
							   string destinationFolder = "ofxDownloadCentral_downloads"
							   ){

			vector<string> shas;
			downloadResources(urlList, shas, listener, listenerMethod, destinationFolder);
		}


	private:

		void startQueue();

		vector<ofxBatchDownloader*>			downloaders;

		bool								busy;
		bool								verbose;
		bool								onlySkipDownloadIfChecksumMatches;
		ofMutex								mutex;

};

#endif

