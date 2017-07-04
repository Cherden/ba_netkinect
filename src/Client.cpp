#include "Client.h"

#include <iostream>

#include "Logger.h"
#include "../gen/KinectFrameMessage.pb.h"

Client::Client(int socket)
	: _con(socket)
	, _video({})
	, _depth({})
	, _data_available(0)
	, _running(1)
	, _data_mutex()
	, _client_thread(&Client::_threadHandle, this) {}

Client::~Client(){
	_running = 0;
	_con.closeConnection();
	_clearData();
	_client_thread.join();
}

void Client::setInfo(struct sockaddr_in* info){
	_con.setInfo(info);
}

int Client::getData(Mat& video, Mat& depth){
	if (!_data_available){
		return -1;
	}

	_data_mutex.lock();

	cvtColor(*_video.frame, video, CV_RGB2BGR);
	_depth.frame->copyTo(depth);

	_data_available = 0;

	_data_mutex.unlock();

	return 0;
}

void Client::_clearData(){
	if (_video.data){
		free(_video.data);
		_video.data = 0;
	}

	if (_depth.data){
		free(_depth.data);
		_depth.data = 0;
	}

	if (_video.frame){
		delete _video.frame;
		_video.frame = 0;
	}

	if (_depth.frame){
		delete _depth.frame;
		_depth.frame = 0;
	}
}

int Client::_handleFrameMessage(int len){
	KinectFrameMessage frame;
	char* buf = (char*) malloc(len);

	_con.recvData((void *) buf, len);
	frame.ParseFromArray(buf, len);

	if (frame.fvideo_data() == "" || frame.fdepth_data() == ""
		|| frame.timestamp() == 0){
		LOG_ERROR << "message does not contain at least one required field"
			<< endl;
		return -1;
	}

	_clearData();

	/* Create opencv matrix for video frame */
	_video.data = (char*) malloc(frame.fvideo_size());
	memcpy(_video.data, frame.fvideo_data().c_str(), frame.fvideo_size());

	_video.frame = new Mat(Size(frame.fvideo_width(), frame.fvideo_height())
					, CV_8UC3, _video.data);


	/* Create opencv matrix for depth frame */
	_depth.data = (char*) malloc(frame.fdepth_size());
	memcpy(_depth.data, frame.fdepth_data().c_str(), frame.fdepth_size());

	_depth.frame = new Mat(Size(frame.fdepth_width(), frame.fdepth_height())
					, CV_16UC1, _depth.data);

	_depth.frame->convertTo(*_depth.frame, CV_8UC1, 255.0/2048.0);

	free(buf);

	_data_available = 1;

	return frame.timestamp();
}

void Client::_threadHandle(){
	uint64_t size = 0;
	int timestamp = 0;

	while (_running){
		if (_con.isClosed()){
			_running = 0;
			break;
		}

		_con.recvData((void*) &size, 4);
		size = ntohl(size);

		LOG_DEBUG << "next protobuf message size is " << size << endl;

		_data_mutex.lock();
		timestamp = _handleFrameMessage(size);
		_data_mutex.unlock();

		if (timestamp > 0) {
			LOG_DEBUG << "Timestamp = " << timestamp << endl;
		}
	}

	LOG_DEBUG << "leaving _threadHandle" << endl;
}
