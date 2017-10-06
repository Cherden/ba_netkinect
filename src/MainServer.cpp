#include <iostream>
#include <ratio>
#include <chrono>
#include <thread>
#include <time.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "Common.h"

#include "ServerAPI.h"
#include "Client.h"


#define LOG_LEVEL DEBUG


using namespace std;
using namespace cv;

int main(){
	ServerAPI api = ServerAPI::getInstance();

	char* video = NULL;
	//char* depth = NULL;
	float* cloud = NULL;

	int video_size = 0;
	//int depth_size = 0;
	int cloud_size = 0;

	cout << "Waiting for clients ..." << endl;
	while (true){
		if (api.isAbleToDeliverData()){
			for(int i = 0; i < api.getClientCount(); i++){
				Client* c = api.getClient(i);

				if ((video_size = c->getVideo(&video, video_size)) == -1){
					continue;
				}

				/*if ((depth_size = c.getDepth(&depth, depth_size)) == -1){
					continue;
				}*/

				if ((cloud_size = c->getCloud(&cloud, cloud_size)) == -1){
					continue;
				}

				c->processedData();

#ifdef SHOW_IMAGE
				Mat video_mat(Size(640, 480), CV_8UC3, video);
				Mat depth_mat(Size(640, 480), CV_16UC1, depth);

				cvtColor(video_mat, video_mat, CV_RGB2BGR);
				depth_mat.convertTo(depth_mat, CV_8UC1, 255.0/2048.0);

				imshow("depth " + to_string(i), depth_mat);
				moveWindow("depth " + to_string(i), i*640, 500);
				imshow("rgb " + to_string(i), video_mat);
				moveWindow("rgb " + to_string(i), i*640, 0);
	 			cvWaitKey(1);
#endif
			}
		}
	}

	return 0;
}
