#ifndef _CVIDEODECODE_H_INCLUDED
#define _CVIDEODECODE_H_INCLUDED
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <stdint.h>
#include <mutex>
#include <condition_variable>
#include <opencv2/opencv.hpp>
#include <atomic>
#include "frame_data.h"
//
class CVideoDecode
{
private:
  GstElement *m_pipeline;
  std::mutex m_mtx;
  std::condition_variable m_cv;
  cv::Mat m_mat;
  std::atomic<bool> m_frame_ready;
  bool _push_data(const _frame_data_ *);
  bool _push_data_sub(GstElement *, void *, size_t);
  void _create_pipeline();
  bool _wait_for_decoding();
//
public:
  CVideoDecode();
  ~CVideoDecode();
  GstFlowReturn FromSample(GstAppSink *);
  bool Convert(const _frame_data_ *, cv::Mat &);
};

#endif

