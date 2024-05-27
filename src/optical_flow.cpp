#include <boost/interprocess/sync/scoped_lock.hpp>
#include <iostream>
#include <opencv2/opencv.hpp>

#include "optical_flow.hpp"

using namespace boost::interprocess;

namespace {
constexpr int kOpticalFlowID = 1;
} // namespace

std::unique_ptr<OpticalFlow> OpticalFlow::Create() {
  shared_memory_object::remove(kSharedOpticalFlowName);
  return std::unique_ptr<OpticalFlow>(new (std::nothrow) OpticalFlow());
}

OpticalFlow::OpticalFlow()
    : shm_sender_(create_only, kSharedOpticalFlowName, read_write),
      shm_recv_(open_only, kSharedMemoryName, read_write),
      data_sender_(nullptr), data_recv_(nullptr), enable_stream_(true),
      frame_(kFrameHeight, kFrameWidth, CV_8UC3) {
  Init();
}

OpticalFlow::~OpticalFlow() {
  shared_memory_object::remove(kSharedOpticalFlowName);
  data_recv_ = nullptr;
  data_sender_ = nullptr;
}

bool OpticalFlow::StartStream() {
  Run();
  return true;
}

bool OpticalFlow::StopStream() {
  enable_stream_ = false;
  return true;
}

void OpticalFlow::Init() {
  region_recv_ = mapped_region(shm_recv_, read_write);
  data_recv_ = static_cast<BufferMemory *>(region_recv_.get_address());

  shm_sender_.truncate(sizeof(BufferMemory));
  region_sender_ = mapped_region(shm_sender_, read_write);
  data_sender_ = new (region_sender_.get_address()) BufferMemory;
}

void OpticalFlow::Run() {
  cv::Mat prev_gray, gray, flow, flow_image;
  bool first_frame = true;

  while (enable_stream_) {
    // Read original frame from buffer
    {
      scoped_lock<boost::interprocess::interprocess_mutex> lock(
          data_recv_->mutex);
      // std::cerr << "Get the lock" << std::endl;
      if (!data_recv_->new_frames[kOpticalFlowID].load()) {
        // std::cerr << "wait for lock" << std::endl;
        data_recv_->frame_ready.wait(lock); // Wait for new frame
      }
      std::memcpy(frame_.data, data_recv_->frame_data,
                  frame_.total() * frame_.elemSize());

      data_recv_->clients_read++;
      data_recv_->new_frames[kOpticalFlowID].store(false);

      // std::cerr << "Debug " << data_recv_->clients_read << std::endl;
      if (data_recv_->clients_read.load() == kNumOfClients) {
        data_recv_->frame_done.notify_one();
      }
    }
    if (first_frame) {
      cv::cvtColor(frame_, prev_gray, cv::COLOR_BGR2GRAY);
      first_frame = false;
    } else {
      // Convert the frame to grayscale
      cv::cvtColor(frame_, gray, cv::COLOR_BGR2GRAY);

      // Calculate optical flow using Farneback method
      cv::calcOpticalFlowFarneback(prev_gray, gray, flow, 0.5, 3, 15, 3, 5, 1.2,
                                   0);

      // Draw the optical flow
      DrawOpticalFlow(flow, flow_image);

      // Write flow image to buffer
      {
        std::memcpy(data_sender_->frame_data, flow_image.data,
                    flow_image.total() * flow_image.elemSize());
      }
      //  Debug window
      //cv::imshow("Optical Flow", flow_image);

      std::swap(prev_gray, gray);
    }
    if (cv::waitKey(30) >= 0)
      break;
  }
}

void OpticalFlow::DrawOpticalFlow(const cv::Mat &flow, cv::Mat &flow_image) {
  // Create an HSV image
  cv::Mat hsv(flow.size(), CV_8UC3);
  for (int y = 0; y < flow.rows; y++) {
    for (int x = 0; x < flow.cols; x++) {
      // Get the flow vector at this point
      const cv::Point2f &flow_at_xy = flow.at<cv::Point2f>(y, x);
      float fx = flow_at_xy.x;
      float fy = flow_at_xy.y;

      // Compute the magnitude and angle of the flow
      float magnitude = std::sqrt(fx * fx + fy * fy);
      float angle = std::atan2(fy, fx) * 180 / CV_PI;

      // Set HSV values
      hsv.at<cv::Vec3b>(y, x)[0] = (angle + 180) / 2; // Hue
      hsv.at<cv::Vec3b>(y, x)[1] = 255;               // Saturation
      hsv.at<cv::Vec3b>(y, x)[2] = cv::saturate_cast<uchar>(
          std::min(double(magnitude * 10), 255.0)); // Value
    }
  }

  // Convert HSV image to BGR for display
  cvtColor(hsv, flow_image, cv::COLOR_HSV2BGR);
}

int main() {
  try {
    auto optical_flow = OpticalFlow::Create();
    optical_flow->StartStream();
  } catch (const std::exception &ex) {
    std::cerr << ex.what() << std::endl;
    return 1;
  }
  return 0;
}