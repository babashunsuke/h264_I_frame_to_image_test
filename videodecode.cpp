#include "videodecode.h"
#include <syslog.h>
#include <gst/app/gstappsrc.h>
//
static GstFlowReturn on_new_sample_from_sink(GstAppSink *appsink, gpointer user_data)
{
  return ((CVideoDecode *)(user_data))->FromSample(appsink);
}
//
CVideoDecode::CVideoDecode()
{
  if (gst_is_initialized() == false) {
    syslog(LOG_ERR, "You need to initialize the gstreamer library.");
  }
  _create_pipeline();
}

CVideoDecode::~CVideoDecode()
{
  if (m_pipeline) {
    gst_object_unref(m_pipeline);
  }
}

void CVideoDecode::_create_pipeline()
{
  const char *str = "appsrc name=src ! h264parse ! openh264dec ! videoconvert ! video/x-raw ! appsink name=sink";
  m_pipeline = gst_parse_launch(str, NULL);
  GstElement *appsrc = gst_bin_get_by_name(GST_BIN(m_pipeline), "src");
  if (appsrc) {
    GstCaps *src_caps = gst_caps_new_simple(
      "video/x-h264",
      "stream-format", G_TYPE_STRING, "byte-stream",
      "alignment", G_TYPE_STRING, "nal",
      NULL
    );
    if (src_caps) {
      g_object_set(appsrc, "caps", src_caps, NULL);
      gst_caps_unref(src_caps);
    }
    gst_object_unref(appsrc);
  }
//
  GstElement *sink = gst_bin_get_by_name(GST_BIN(m_pipeline), "sink");
  if (sink) {
    GstCaps *sink_caps = gst_caps_new_simple(
      "video/x-raw",
      "format", G_TYPE_STRING, "BGR",
      NULL
    );
    if (sink_caps) {
      g_object_set(sink, "emit-signals", TRUE, "caps", sink_caps, NULL);
      gst_caps_unref(sink_caps);
    }
    g_signal_connect(sink, "new-sample", G_CALLBACK(on_new_sample_from_sink), this);
    gst_object_unref(sink);
  }
}

bool CVideoDecode::_push_data(const _frame_data_ *p)
{
  bool ok = false;
  GstElement *appsrc = gst_bin_get_by_name(GST_BIN(m_pipeline), "src");
  if (appsrc) {
    if (_push_data_sub(appsrc, p->sps, p->sps_size)) {
      if (_push_data_sub(appsrc, p->pps, p->pps_size)) {
        if (_push_data_sub(appsrc, p->idr, p->idr_size)) {
          ok = true;
        }
      }
    }
    int ret;
    g_signal_emit_by_name(appsrc, "end-of-stream", &ret);
    gst_object_unref(appsrc);
  }
  return ok;
}

bool CVideoDecode::_push_data_sub(GstElement *appsrc, void *p, size_t size)
{
  if (p != NULL && 0 < size) {
    GstBuffer *buffer = gst_buffer_new_allocate(NULL, size, NULL);
    if (buffer) {
      GstMapInfo map;
      if (gst_buffer_map(buffer, &map, GST_MAP_WRITE)) {
        memcpy(map.data, p, size);
        gst_buffer_unmap(buffer, &map);
        GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(appsrc), buffer);
        if (ret == GST_FLOW_OK) { // buffer will be handled in gst_app_src_push_buffer
          return true;
        }
      }
      gst_buffer_unref(buffer);
    }
  }
  return false;
}

bool CVideoDecode::_wait_for_decoding()
{
  std::unique_lock<std::mutex> lock(m_mtx);
  if (m_cv.wait_for(lock, std::chrono::seconds(2), [this] { return m_frame_ready.load(); })) {
    syslog(LOG_DEBUG, "Frame successfully decoded and processed.");
    return true;
  }
  syslog(LOG_ERR, "Timeout waiting for frame to be decoded.");
  return false;
}

bool CVideoDecode::Convert(const _frame_data_ *p, cv::Mat &m)
{
  gst_element_set_state(m_pipeline, GST_STATE_PLAYING);
  m_frame_ready.store(false);
  m_mat.release();
  _push_data(p);
  bool r = _wait_for_decoding();
  gst_element_set_state(m_pipeline, GST_STATE_NULL);
//
  if (r) {
    m = m_mat.clone();
  }
  return r;
}

GstFlowReturn CVideoDecode::FromSample(GstAppSink *appsink)
{
  GstSample *sample = gst_app_sink_pull_sample(appsink);
  if (sample) {
    GstBuffer *buffer = gst_sample_get_buffer(sample);
    if (buffer) {
      GstMapInfo map;
      if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
        int width = 0;
        int height = 0;
        GstCaps *caps = gst_sample_get_caps(sample);
	if (caps) {
          GstStructure *structure = gst_caps_get_structure(caps, 0);
          gst_structure_get_int(structure, "width", &width);
          gst_structure_get_int(structure, "height", &height);
	  gst_caps_unref(caps);
	}
	if (0 < height && 0 < width) {
          m_mat.create(height, width, CV_8UC3);
          memcpy(m_mat.data, map.data, (height * width * 3));
          syslog(LOG_DEBUG, "decode OK");
	}
        gst_buffer_unmap(buffer, &map);
      }
    }
    gst_sample_unref(sample);
    m_frame_ready.store(true);
    m_cv.notify_one();
    return GST_FLOW_OK;
  }
  return GST_FLOW_ERROR;
}

