#include <boost/interprocess/sync/scoped_lock.hpp>
#include <iostream>

#include "frame_reader.hpp"

using namespace boost::interprocess;

namespace {
constexpr int kFrameReaderID = 2;
} // namespace

std::unique_ptr<FrameReader> FrameReader::Create() {
  return std::unique_ptr<FrameReader>(new (std::nothrow) FrameReader());
}

FrameReader::FrameReader()
    : shm_original_(open_only, kSharedMemoryName, read_write),
      shm_optical_(open_only, kSharedOpticalFlowName, read_write),
      shm_circle_(open_only, kSharedCircleDetectName, read_write),
      data_original_(nullptr), data_optical_(nullptr), data_circle_(nullptr),
      enable_stream_(true), frame_original_(kFrameHeight, kFrameWidth, CV_8UC3),
      frame_optical_(kFrameHeight, kFrameWidth, CV_8UC3),
      frame_circle_(kFrameHeight, kFrameWidth, CV_8UC3) {
  Init();
}

FrameReader::~FrameReader() {
  data_original_ = nullptr;
  data_optical_ = nullptr;
  data_circle_ = nullptr;
}

bool FrameReader::StartStream() {
  Run();
  return true;
}

bool FrameReader::StopStream() {
  enable_stream_ = false;
  return true;
}

void FrameReader::Init() {
  region_original_ = mapped_region(shm_original_, read_write);
  data_original_ = static_cast<BufferMemory *>(region_original_.get_address());

  region_optical_ = mapped_region(shm_optical_, read_write);
  data_optical_ = static_cast<BufferMemory *>(region_optical_.get_address());

  region_circle_ = mapped_region(shm_circle_, read_write);
  data_circle_ = static_cast<BufferMemory *>(region_circle_.get_address());
}

void FrameReader::Run() {
  while (enable_stream_) {
    {
      scoped_lock<boost::interprocess::interprocess_mutex> lock(
          data_original_->mutex);
      // std::cerr << "Get the lock" << std::endl;
      if (!data_original_->new_frames[kFrameReaderID].load()) {
        // std::cerr << "wait for lock" << std::endl;
        data_original_->frame_ready.wait(lock); // Wait for new frame
      }
      std::memcpy(frame_original_.data, data_original_->frame_data,
                  frame_original_.total() * frame_original_.elemSize());

      data_original_->clients_read++;
      data_original_->new_frames[kFrameReaderID].store(false);

      // std::cerr << "Debug " << data_recv_->clients_read << std::endl;
      if (data_original_->clients_read.load() == kNumOfClients) {
        data_original_->frame_done.notify_one();
      }
    }

    {
      std::memcpy(frame_optical_.data, data_optical_->frame_data,
                  frame_optical_.total() * frame_optical_.elemSize());
    }

    {
      std::memcpy(frame_circle_.data, data_circle_->frame_data,
                  frame_circle_.total() * frame_circle_.elemSize());
    }

    cv::imshow("Original Frame", frame_original_);
    cv::imshow("Optical Frame", frame_optical_);
    cv::imshow("Detected Circles", frame_circle_);
    if (cv::waitKey(30) >= 0)
      break;
  }
}

int main(int argc, char *argv[]) {
  try {
    auto frame_reader = FrameReader::Create();
    frame_reader->StartStream();
  } catch (const std::exception &ex) {
    std::cerr << ex.what() << std::endl;
    return 1;
  }

  return 0;
}