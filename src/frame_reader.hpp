#pragma once

#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <opencv2/opencv.hpp>

#include "shared_memory_buffer.hpp"

class FrameReader final {
public:
  static std::unique_ptr<FrameReader> Create();

  ~FrameReader();

  bool StartStream();
  bool StopStream();

private:
  FrameReader();

  void Init();
  void Run();

  boost::interprocess::shared_memory_object shm_original_;
  boost::interprocess::shared_memory_object shm_optical_;
  boost::interprocess::shared_memory_object shm_circle_;
  boost::interprocess::mapped_region region_original_;
  boost::interprocess::mapped_region region_optical_;
  boost::interprocess::mapped_region region_circle_;
  BufferMemory *data_original_;
  BufferMemory *data_optical_;
  BufferMemory *data_circle_;
  bool enable_stream_;
  cv::Mat frame_original_;
  cv::Mat frame_optical_;
  cv::Mat frame_circle_;
};