#pragma once

#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <opencv2/opencv.hpp>
#include <thread>
#include <mutex>

#include "shared_memory_buffer.hpp"

class FrameCapture final {
public:
  static std::unique_ptr<FrameCapture> Create();

  ~FrameCapture();

  bool StartStream();
  bool StopStream();

private:
  FrameCapture(std::unique_ptr<cv::VideoCapture> cap);

  void Init();
  void Run();

  std::unique_ptr<cv::VideoCapture> cap_;
  boost::interprocess::shared_memory_object shm_;
  boost::interprocess::mapped_region region_;
  BufferMemory *data_;
  bool enable_stream_;
  cv::Mat frame_;
};