#include <boost/interprocess/sync/scoped_lock.hpp>
#include <iostream>

#include "frame_capture.hpp"

using namespace boost::interprocess;

std::unique_ptr<FrameCapture> FrameCapture::Create() {
  auto cap = std::make_unique<cv::VideoCapture>(0);

  shared_memory_object::remove(kSharedMemoryName);

  return std::unique_ptr<FrameCapture>(
      new (std::nothrow) FrameCapture(std::move(cap)));
}

FrameCapture::FrameCapture(std::unique_ptr<cv::VideoCapture> cap)
    : cap_(std::move(cap)), shm_(create_only, kSharedMemoryName, read_write),
      data_(nullptr), enable_stream_(true) {
  Init();
}

FrameCapture::~FrameCapture() {
  shared_memory_object::remove(kSharedMemoryName);
  data_ = nullptr;
  cap_.release();
}

bool FrameCapture::StartStream() {
  Run();
  return true;
}

bool FrameCapture::StopStream() {
  enable_stream_ = false;
  return true;
}

void FrameCapture::Init() {
  shm_.truncate(sizeof(BufferMemory));
  region_ = mapped_region(shm_, read_write);
  data_ = new (region_.get_address()) BufferMemory;
  data_->clients_read.store(kNumOfClients);
  for (int i = 0; i < kNumOfClients; ++i) {
    data_->new_frames[i].store(false); // Reset client flags
  }

  if (!cap_->open(0)) {
    throw std::runtime_error("Could not open camera");
  }
}

void FrameCapture::Run() {
  while (enable_stream_) {
    *cap_ >> frame_;
    if (frame_.empty()) {
      std::cerr << "Error: Captured empty frame_" << std::endl;
      break;
    }

    cv::resize(frame_, frame_, cv::Size(kFrameWidth, kFrameHeight));

    // {
    //   std::memcpy(data_->frame_data, frame_.data_,
    //               frame_.total() * frame_.elemSize());
    // }

    {
      scoped_lock<boost::interprocess::interprocess_mutex> lock(data_->mutex);
      // std::cerr << "Get the lock" << std::endl;
      if (data_->clients_read.load() != kNumOfClients) {
        // std::cerr << "wait for lock" << std::endl;
        data_->frame_done.wait(lock);
      }
      std::memcpy(data_->frame_data, frame_.data,
                  frame_.total() * frame_.elemSize());

      // std::cerr << "Debug " << data_->clients_read << std::endl;
      data_->frame_ready.notify_all(); // Notify clients about new frame
      data_->clients_read.store(0);         // Reset client count

      for (int i = 0; i < kNumOfClients; ++i) {
        data_->new_frames[i].store(true); // Reset client flags
      }
    }
    // Debug window
    //cv::imshow("Frame", frame_);
    if (cv::waitKey(30) >= 0)
      break;
  }
}

int main() {
  try {
    auto frame_capture = FrameCapture::Create();
    frame_capture->StartStream();
  } catch (const std::exception &ex) {
    std::cerr << ex.what() << std::endl;
    return 1;
  }

  return 0;
}