#pragma once

#include "ofMain.h"
#include "ofxBatchDownloader.h"
#include "ofxDownloadCentral.h"
#include "ofxJSONElement.h"

#define NUM_OBJECTS 5

static int OBJECT_ID = 0;

class Object{

public:

	Object(){ ID = OBJECT_ID; OBJECT_ID++; };

	void downloadFinished(ofxBatchDownloaderReport &report){
		cout << "#### Object with ID "<< ID << " got report back!!" << endl;
		cout << "#### " << report.attemptedDownloads.size() << " attempted; " ;
		cout << report.failedDownloads.size() << " failed, ";
		cout << report.successfulDownloads.size() << " ok!!" << endl;
		for(int i = 0; i < report.responses.size(); i++){
			ofxSimpleHttpResponse r = report.responses[i];
			r.print();
		}
	}

	void setup(ofxDownloadCentral *downloader){

		d = downloader;
		//ofAddListener(d->resourcesDownloadFinished, this, &Object::downloadFinished);
		vector<string> downloadList;

		if(ofRandom(1) > 0.5){
			downloadList.push_back("http://farm8.staticflickr.com/7420/10032530563_86ff701d19_o.jpg");
			downloadList.push_back("http://farm4.staticflickr.com/3686/9225463176_d0bf83a992_o.jpg");
		}else{
			downloadList.push_back("http://farm8.staticflickr.com/7047/7034809565_5f80871bff_o.jpg");
			downloadList.push_back("http://farm8.staticflickr.com/7438/9481688475_e83f92e8b5_o.jpg__");
		}
		d->downloadResources(	downloadList,				//list of urls to download
								this,						//who will be notified
								&Object::downloadFinished,	//callback method
								"downloads_" + ofToString(ID)	//destination folder
							 );
	};


	int ID;
	ofxDownloadCentral *d;
};


class testApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();

		void keyPressed(int key);

	vector<Object*> objects;
	ofxDownloadCentral downloader;

};
