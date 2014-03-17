//
//  ofxDownloadCentral.h
//  emptyExample
//
//  Created by Oriol Ferrer Mesià on 14/03/14.
//
//

#ifndef emptyExample_ofxBatchDownloader_h
#define emptyExample_ofxBatchDownloader_h

#include "ofMain.h"
#include "ofxBatchDownloader.h"

/*
 
 
 
 */
class ofxDownloadCentral;

struct AssetInfo{
	string url;
	string absolutePath;
	ofxSimpleHttpResponse response;
}


struct ofxDownloadCentralReport{
	vector<AssetInfo> downloadInfo;	//list of all asset URLs to download
	string downloadPath;			//full path to the directory that holds the downloads
	ofxDownloadCentral* owner;		//pointer to the object in charge of the downloads
};


class ofxDownloadCentral{

	public:

		ofxDownloadCentral();

		void update();
		void draw(float x, float y);

		void downloadResources( vector<string>urlList, string downloadFolder = "_downloads_" );
		void cancelDownload();

		ofEvent<ofxDownloadCentralReport>	resourcesDownloadFinished;

	private:

		ofxBatchDownloader					downloader;

		bool								busy;
		bool								needToStop; //user wants to cancel!
		bool								verbose;
		string								downloadFolder;


		void downloadsFinished(ofxBatchDownloaderReport &report);

};

#endif

