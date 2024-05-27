#pragma once

#include <atomic>
#include <boost/interprocess/sync/interprocess_condition.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>

namespace {
constexpr int kFrameWidth = 640;
constexpr int kFrameHeight = 480;
constexpr int kNumOfChannels = 3;
constexpr int kNumOfClients = 3;
const constexpr char *kSharedMemoryName = "OriginalFrame";
const constexpr char *kSharedOpticalFlowName = "OpticalFlowFrame";
const constexpr char *kSharedCircleDetectName = "CircleDetectionFrame";
} // namespace

struct BufferMemory {
  boost::interprocess::interprocess_mutex mutex;
  boost::interprocess::interprocess_condition frame_ready;
  boost::interprocess::interprocess_condition frame_done;
  std::atomic<int> clients_read;
  std::atomic<bool> new_frames[kNumOfClients];
  char frame_data[kFrameWidth * kFrameHeight * kNumOfChannels];
};
