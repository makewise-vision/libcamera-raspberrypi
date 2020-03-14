/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2019, Google Inc.
 *
 * libcamera Camera API tests
 */

#include <iostream>

#include "camera_test.h"
#include "test.h"

using namespace std;

namespace {

class Capture : public CameraTest, public Test
{
public:
	Capture()
		: CameraTest("VIMC Sensor B")
	{
	}

protected:
	unsigned int completeBuffersCount_;
	unsigned int completeRequestsCount_;

	void bufferComplete(Request *request, FrameBuffer *buffer)
	{
		if (buffer->metadata().status != FrameMetadata::FrameSuccess)
			return;

		completeBuffersCount_++;
	}

	void requestComplete(Request *request)
	{
		if (request->status() != Request::RequestComplete)
			return;

		const std::map<Stream *, FrameBuffer *> &buffers = request->buffers();

		completeRequestsCount_++;

		/* Create a new request. */
		Stream *stream = buffers.begin()->first;
		FrameBuffer *buffer = buffers.begin()->second;

		request = camera_->createRequest();
		request->addBuffer(stream, buffer);
		camera_->queueRequest(request);
	}

	int init() override
	{
		if (status_ != TestPass)
			return status_;

		config_ = camera_->generateConfiguration({ StreamRole::VideoRecording });
		if (!config_ || config_->size() != 1) {
			cout << "Failed to generate default configuration" << endl;
			return TestFail;
		}

		allocator_ = new FrameBufferAllocator(camera_);

		return TestPass;
	}

	void cleanup() override
	{
		delete allocator_;
	}

	int run() override
	{
		StreamConfiguration &cfg = config_->at(0);

		if (camera_->acquire()) {
			cout << "Failed to acquire the camera" << endl;
			return TestFail;
		}

		if (camera_->configure(config_.get())) {
			cout << "Failed to set default configuration" << endl;
			return TestFail;
		}

		Stream *stream = cfg.stream();

		int ret = allocator_->allocate(stream);
		if (ret < 0)
			return TestFail;

		std::vector<Request *> requests;
		for (const std::unique_ptr<FrameBuffer> &buffer : allocator_->buffers(stream)) {
			Request *request = camera_->createRequest();
			if (!request) {
				cout << "Failed to create request" << endl;
				return TestFail;
			}

			if (request->addBuffer(stream, buffer.get())) {
				cout << "Failed to associating buffer with request" << endl;
				return TestFail;
			}

			requests.push_back(request);
		}

		completeRequestsCount_ = 0;
		completeBuffersCount_ = 0;

		camera_->bufferCompleted.connect(this, &Capture::bufferComplete);
		camera_->requestCompleted.connect(this, &Capture::requestComplete);

		if (camera_->start()) {
			cout << "Failed to start camera" << endl;
			return TestFail;
		}

		for (Request *request : requests) {
			if (camera_->queueRequest(request)) {
				cout << "Failed to queue request" << endl;
				return TestFail;
			}
		}

		EventDispatcher *dispatcher = cm_->eventDispatcher();

		Timer timer;
		timer.start(1000);
		while (timer.isRunning())
			dispatcher->processEvents();

		unsigned int nbuffers = allocator_->buffers(stream).size();

		if (completeRequestsCount_ <= nbuffers * 2) {
			cout << "Failed to capture enough frames (got "
			     << completeRequestsCount_ << " expected at least "
			     << nbuffers * 2 << ")" << endl;
			return TestFail;
		}

		if (completeRequestsCount_ != completeBuffersCount_) {
			cout << "Number of completed buffers and requests differ" << endl;
			return TestFail;
		}

		if (camera_->stop()) {
			cout << "Failed to stop camera" << endl;
			return TestFail;
		}

		return TestPass;
	}

	std::unique_ptr<CameraConfiguration> config_;
	FrameBufferAllocator *allocator_;
};

} /* namespace */

TEST_REGISTER(Capture);
