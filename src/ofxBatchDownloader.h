//
//  ofxBatchDownloader.h
//  emptyExample
//
//  Created by Oriol Ferrer Mesià on 14/03/14.
//
//

#ifndef emptyExample_ofxBatchDownloader_h
#define emptyExample_ofxBatchDownloader_h

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
	std::string downloadPath;						//full path to the directory that holds the downloads
	ofxBatchDownloader* owner;					//pointer to the object in charge of the downloads
	std::vector<std::string> attemptedDownloads;			//this vector contains all the user supplied urls
	std::vector<std::string> successfulDownloads;			//this vector contains all the urls that got downloaded correctly
	std::vector<std::string> failedDownloads;				//this vector contains all the urls that failed to download
	std::vector<ofxSimpleHttpResponse> responses;	//this vector contains a list of all url responses, with statuses, filenames, etc
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
	std::string getDrawableString();
	std::string getMinimalDrawableString();

	void setDownloadFolder(std::string f);
	void setSpeedLimit(float KB_per_sec);
	void setTimeOut(float timeOut);

	void addResourcesToDownloadList( std::vector<std::string>urlList );
	void addResourcesToDownloadList( std::vector<std::string>urlList, std::vector<std::string>sha1List );
	void startDownloading();
	void cancelBatch(bool notify = false);

	void setVerbose(bool b); //nop
	void setNeedsChecksumMatchToSkipDownload(bool needsChecksum);
	void setIdleTimeAfterEachDownload(float seconds); //wait a bit before notifying once the dowload is over

	void setProxyConfiguration(const ofxSimpleHttp::ProxyConfig & c);
	void setCredentials(const std::string& user, const std::string& password);

	unsigned long int getDownloadedBytesSoFar(){ return downloadedSoFar;}
	int getNumSuppliedUrls();
	int getNumFailedUrls();
	int pendingDownloads();
	bool isBusy();
	float getAverageSpeed(); //bytes/sec
	std::vector<std::string> pendingURLs();

	ofEvent<ofxBatchDownloaderReport>	resourcesDownloadFinished;

private:

	ofxSimpleHttp						http;
	bool								busy;
	bool								needToStop; //user wants to cancel!
	//bool								verbose;
	std::string								downloadFolder;

	std::vector<std::string>						originalUrlList;
	std::vector<std::string>						originalSha1List;
	std::vector<std::string>						failedList;
	std::vector<std::string>						okList;
	std::vector<ofxSimpleHttpResponse>		responses;


	void httpResult(ofxSimpleHttpResponse &response);
	void reset();

	unsigned long int					downloadedSoFar; //bytes


};

#endif

