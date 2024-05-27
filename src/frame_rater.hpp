#pragma once

#include <opencv2/opencv.hpp>
#include <iostream>

class FrameRater
{
public:
    FrameRater() : fps_(0.0), frame_count_(0), elapsed_time_(0.0), start_time_(cv::getTickCount()) {}

    void Update()
    {
        frame_count_++;
        double current_time = cv::getTickCount();
        double delta_time = (current_time - start_time_) / cv::getTickFrequency();
        elapsed_time_ += delta_time;
        start_time_ = current_time;
        if (elapsed_time_ > 1.0)
        {
            fps_ = frame_count_ / elapsed_time_;
            frame_count_ = 0;
            elapsed_time_ = 0.0;
        }
    }

    double GetFPS() const
    {
        return fps_;
    }

private:
    double fps_;
    int frame_count_;
    double elapsed_time_;
    double start_time_;
};