#pragma once

#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <opencv2/opencv.hpp>
#include <thread>
#include <mutex>

#include "shared_memory_buffer.hpp"

class OpticalFlow final {
public:
  static std::unique_ptr<OpticalFlow> Create();

  ~OpticalFlow();

  bool StartStream();
  bool StopStream();

private:
  OpticalFlow();

  void Init();
  void Run();
  void DrawOpticalFlow(const cv::Mat& flow, cv::Mat& flow_image);

  boost::interprocess::shared_memory_object shm_sender_;
  boost::interprocess::mapped_region region_sender_;
  boost::interprocess::shared_memory_object shm_recv_;
  boost::interprocess::mapped_region region_recv_;
  BufferMemory *data_sender_;
  BufferMemory *data_recv_;
  bool enable_stream_;
  cv::Mat frame_;
};