#include <boost/interprocess/sync/scoped_lock.hpp>
#include <iostream>
#include <opencv2/opencv.hpp>

#include "circle_detection.hpp"

using namespace boost::interprocess;

namespace {
constexpr int kCircleDetectID = 0;
} // namespace

std::unique_ptr<CircleDetect> CircleDetect::Create() {
  shared_memory_object::remove(kSharedCircleDetectName);
  return std::unique_ptr<CircleDetect>(new (std::nothrow) CircleDetect());
}

CircleDetect::CircleDetect()
    : shm_sender_(create_only, kSharedCircleDetectName, read_write),
      shm_recv_(open_only, kSharedMemoryName, read_write),
      data_sender_(nullptr), data_recv_(nullptr), enable_stream_(true),
      frame_(kFrameHeight, kFrameWidth, CV_8UC3) {
  Init();
}

CircleDetect::~CircleDetect() {
  shared_memory_object::remove(kSharedCircleDetectName);
  data_recv_ = nullptr;
  data_sender_ = nullptr;
}

bool CircleDetect::StartStream() {
  Run();
  return true;
}

bool CircleDetect::StopStream() {
  enable_stream_ = false;
  return true;
}

void CircleDetect::Init() {
  region_recv_ = mapped_region(shm_recv_, read_write);
  data_recv_ = static_cast<BufferMemory *>(region_recv_.get_address());

  shm_sender_.truncate(sizeof(BufferMemory));
  region_sender_ = mapped_region(shm_sender_, read_write);
  data_sender_ = new (region_sender_.get_address()) BufferMemory;
}

void CircleDetect::DrawCircle(cv::Mat &gray, cv::Mat &blurred,
                              std::vector<cv::Vec3f> &circles) {
  // Convert the frame to grayscale
  cv::cvtColor(frame_, gray, cv::COLOR_BGR2GRAY);

  // Apply Gaussian blur to the grayscale image
  cv::GaussianBlur(gray, blurred, cv::Size(9, 9), 2, 2);

  // Detect circles using HoughCircles
  cv::HoughCircles(blurred, circles, cv::HOUGH_GRADIENT, 1, 20, 50, 30, 1, 40);

  // Draw the detected circles
  for (size_t i = 0; i < circles.size(); i++) {
    cv::Vec3i c = circles[i];
    cv::Point center = cv::Point(c[0], c[1]);
    // Circle center
    cv::circle(frame_, center, 1, cv::Scalar(0, 100, 100), 3, cv::LINE_AA);
    // Circle outline
    int radius = c[2];
    cv::circle(frame_, center, radius, cv::Scalar(255, 0, 255), 3, cv::LINE_AA);
  }
}

void CircleDetect::Run() {
  cv::Mat gray, blurred;
  std::vector<cv::Vec3f> circles;

  while (enable_stream_) {

    // Read original frame from buffer
    {
      scoped_lock<boost::interprocess::interprocess_mutex> lock(
          data_recv_->mutex);
      // std::cerr << "Get the lock" << std::endl;
      if (!data_recv_->new_frames[kCircleDetectID].load()) {
        // std::cerr << "wait for lock" << std::endl;
        data_recv_->frame_ready.wait(lock); // Wait for new frame
      }
      std::memcpy(frame_.data, data_recv_->frame_data,
                  frame_.total() * frame_.elemSize());

      data_recv_->clients_read++;
      data_recv_->new_frames[kCircleDetectID].store(false);

      // std::cerr << "Debug " << data_recv_->clients_read << std::endl;
      if (data_recv_->clients_read.load() == kNumOfClients) {
        data_recv_->frame_done.notify_one();
      }
    }

    DrawCircle(gray, blurred, circles);

    // Write detection image to buffer
    {
      std::memcpy(data_sender_->frame_data, frame_.data,
                  frame_.total() * frame_.elemSize());
    }

    //  Debug window
    //cv::imshow("Detected Circles", frame_);
    if (cv::waitKey(30) >= 0) {
      break;
    }
  }
}

int main() {
  try {
    auto circle_detection = CircleDetect::Create();
    circle_detection->StartStream();
  } catch (const std::exception &ex) {
    std::cerr << ex.what() << std::endl;
    return 1;
  }
  return 0;
}