//
//  ofxBatchDownloader.h
//  emptyExample
//
//  Created by Oriol Ferrer Mesià on 14/03/14.
//
//

#ifndef emptyExample_ofxBatchDownloader_h
#define emptyExample_ofxBatchDownloader_h

#include "ofMain.h"
#include "ofxSimpleHttp.h"

/*

 ####### how to get notifications ############################################

 //create a batch downloader
 ofxBatchDownloader dl;

 //sign up for the notification
 ofAddListener(dl.resourcesDownloadFinished, this, &myApp::downloadFinished);

 //define the callback in your app
 void myApp::downloadFinished(ofxBatchDownloaderReport &report){}

 */

class ofxBatchDownloader;

struct ofxBatchDownloaderReport{
	string downloadPath;						//full path to the directory that holds the downloads
	ofxBatchDownloader* owner;					//pointer to the object in charge of the downloads
	vector<string> attemptedDownloads;			//this vector contains all the user supplied urls
	vector<string> successfulDownloads;			//this vector contains all the urls that got downloaded correctly
	vector<string> failedDownloads;				//this vector contains all the urls that failed to download
	vector<ofxSimpleHttpResponse> responses;	//this vector contains a list of all url responses, with statuses, filenames, etc
	bool wasCanceled;
	ofxBatchDownloaderReport(){
		wasCanceled = false;
	}
};


class ofxBatchDownloader{

public:

	ofxBatchDownloader();
	~ofxBatchDownloader();

	void update();
	void draw(float x, float y);
	string getDrawableString();
	string getMinimalDrawableString();

	void setDownloadFolder(string f);
	void setSpeedLimit(float KB_per_sec);
	void setTimeOut(float timeOut);

	void addResourcesToDownloadList( vector<string>urlList );
	void addResourcesToDownloadList( vector<string>urlList, vector<string>sha1List );
	void startDownloading();
	void cancelBatch(bool notify = false);

	void setVerbose(bool b); //nop
	void setNeedsChecksumMatchToSkipDownload(bool needsChecksum);
	void setIdleTimeAfterEachDownload(float seconds); //wait a bit before notifying once the dowload is over

	void setProxyConfiguration(const ofxSimpleHttp::ProxyConfig & c);


	unsigned long int getDownloadedBytesSoFar(){ return downloadedSoFar;}
	int getNumSuppliedUrls();
	int getNumFailedUrls();
	int pendingDownloads();
	bool isBusy();
	float getAverageSpeed(); //bytes/sec
	vector<string> pendingURLs();

	ofEvent<ofxBatchDownloaderReport>	resourcesDownloadFinished;

private:

	ofxSimpleHttp						http;
	bool								busy;
	bool								needToStop; //user wants to cancel!
	//bool								verbose;
	string								downloadFolder;

	vector<string>						originalUrlList;
	vector<string>						originalSha1List;
	vector<string>						failedList;
	vector<string>						okList;
	vector<ofxSimpleHttpResponse>		responses;


	void httpResult(ofxSimpleHttpResponse &response);
	void reset();

	unsigned long int					downloadedSoFar; //bytes


};

#endif

