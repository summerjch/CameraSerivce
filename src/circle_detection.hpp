#pragma once

#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <opencv2/opencv.hpp>
#include <thread>
#include <mutex>

#include "shared_memory_buffer.hpp"

class CircleDetect final {
public:
  static std::unique_ptr<CircleDetect> Create();

  ~CircleDetect();

  bool StartStream();
  bool StopStream();

private:
  CircleDetect();

  void Init();
  void Run();
  void DrawCircle(cv::Mat& gray, cv::Mat& blurred, std::vector<cv::Vec3f>& circles);

  boost::interprocess::shared_memory_object shm_sender_;
  boost::interprocess::mapped_region region_sender_;
  boost::interprocess::shared_memory_object shm_recv_;
  boost::interprocess::mapped_region region_recv_;
  BufferMemory *data_sender_;
  BufferMemory *data_recv_;
  bool enable_stream_;
  cv::Mat frame_;
};