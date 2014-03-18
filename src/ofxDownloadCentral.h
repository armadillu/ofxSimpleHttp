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
 
 ofxDownloadCentral dlc;

 //sign up for the notification
 ofAddListener(dlc.resourcesDownloadFinished, this, &myClass::downloadsFinished);

 //define the callback in your app
 void myClass::downloadsFinished(ofxDownloadCentralReport &report){}

 */
class ofxDownloadCentral;

//struct DownloadInfo{
//	string url;
//	string absolutePath;
//	ofxSimpleHttpResponse response;
//};


//struct ofxDownloadCentralReport{
//	vector<DownloadInfo> downloadInfo;	//list of all asset URLs to download
//	string downloadPath;				//full path to the directory that holds the downloads
//	ofxDownloadCentral* owner;			//pointer to the object in charge of the downloads
//};



class ofxDownloadCentral{

	public:

		ofxDownloadCentral();

		void update();
		void draw(float x, float y);

		//void downloadResources( vector<string>urlList, string downloadFolder = "_downloads_" );
		void cancelDownload();



		template < typename ArgumentsType, class ListenerClass>
		void downloadResources(	vector<string>urlList,
								//EventType & event,
								ListenerClass  * listener,
								void (ListenerClass::*listenerMethod)(ArgumentsType&)
							   ){

			ofxBatchDownloader * d = new ofxBatchDownloader();
			d->addResourcesToDownloadList(urlList);
			ofAddListener(d->resourcesDownloadFinished, listener, listenerMethod); //set the notification to hit our original caller
			downloaders.push_back(d);
			if (!busy) startQueue();
		}


		//callback
		//void downloadFinished(ofxBatchDownloaderReport &report);

		//ofEvent<ofxBatchDownloaderReport>	resourcesDownloadFinished;

	private:

	
		void startQueue();

		vector<ofxBatchDownloader*>			downloaders;

		bool								busy;
		bool								needToStop; //user wants to cancel!
		bool								verbose;
		string								downloadFolder;

};

#endif

