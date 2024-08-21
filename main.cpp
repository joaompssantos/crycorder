#include <gst/gst.h>
#include <gst/video/video.h>

#include <iostream>

void on_pad_added(GstElement* src, GstPad* new_pad, GstElement* depay) {
  std::cout << "New pad added: " << GST_PAD_NAME(new_pad) << std::endl;

  GstPad* sink_pad = gst_element_get_static_pad(depay, "sink");
  if (gst_pad_is_linked(sink_pad)) {
    std::cout << "Pad already linked." << std::endl;
    g_object_unref(sink_pad);
    return;
  }

  GstCaps* new_pad_caps = gst_pad_query_caps(new_pad, nullptr);
  GstStructure* new_pad_struct = gst_caps_get_structure(new_pad_caps, 0);
  const gchar* new_pad_type = gst_structure_get_name(new_pad_struct);

  if (g_str_has_prefix(new_pad_type, "application/x-rtp")) {
    if (gst_pad_link(new_pad, sink_pad) != GST_PAD_LINK_OK) {
      std::cerr << "Failed to link RTP pad" << std::endl;
    } else {
      std::cout << "RTP pads linked successfully." << std::endl;
    }
  }

  g_object_unref(sink_pad);
  gst_caps_unref(new_pad_caps);
}

void on_pad_added_audio(GstElement* src, GstPad* new_pad, GstElement* depay) {
  std::cout << "New audio pad added: " << GST_PAD_NAME(new_pad) << std::endl;

  GstPad* sink_pad = gst_element_get_static_pad(depay, "sink");
  if (gst_pad_is_linked(sink_pad)) {
    std::cout << "Pad already linked." << std::endl;
    g_object_unref(sink_pad);
    return;
  }

  GstCaps* new_pad_caps = gst_pad_query_caps(new_pad, nullptr);
  GstStructure* new_pad_struct = gst_caps_get_structure(new_pad_caps, 0);
  const gchar* new_pad_type = gst_structure_get_name(new_pad_struct);

  if (g_str_has_prefix(new_pad_type, "application/x-rtp")) {
    if (gst_pad_link(new_pad, sink_pad) != GST_PAD_LINK_OK) {
      std::cerr << "Failed to link RTP audio pad" << std::endl;
    } else {
      std::cout << "RTP audio pads linked successfully." << std::endl;
    }
  }

  g_object_unref(sink_pad);
  gst_caps_unref(new_pad_caps);
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <rtsp-url>" << std::endl;
    return -1;
  }

  gst_init(&argc, &argv);

  GstElement* pipeline = gst_pipeline_new("rtsp-pipeline");
  GstElement* src = gst_element_factory_make("rtspsrc", "src");
  GstElement* depay_video =
      gst_element_factory_make("rtph264depay", "depay-video");
  GstElement* decode_video =
      gst_element_factory_make("avdec_h264", "decode-video");
  GstElement* convert_video =
      gst_element_factory_make("videoconvert", "convert-video");
  GstElement* sink_video =
      gst_element_factory_make("autovideosink", "sink-video");

  GstElement* depay_audio =
      gst_element_factory_make("rtppcmadepay", "depay-audio");
  GstElement* decode_audio =
      gst_element_factory_make("alawdec", "decode-audio");
  GstElement* convert_audio =
      gst_element_factory_make("audioconvert", "convert-audio");
  GstElement* resample_audio =
      gst_element_factory_make("audioresample", "resample-audio");
  GstElement* sink_audio =
      gst_element_factory_make("autoaudiosink", "sink-audio");

  if (!pipeline || !src || !depay_video || !decode_video || !convert_video ||
      !sink_video || !depay_audio || !decode_audio || !convert_audio ||
      !resample_audio || !sink_audio) {
    std::cerr << "Failed to create elements" << std::endl;
    return -1;
  }

  g_object_set(src, "location", argv[1], nullptr);

  gst_bin_add_many(GST_BIN(pipeline), src, depay_video, decode_video,
                   convert_video, sink_video, nullptr);
  gst_bin_add_many(GST_BIN(pipeline), depay_audio, decode_audio, convert_audio,
                   resample_audio, sink_audio, nullptr);

  if (!gst_element_link_many(depay_video, decode_video, convert_video,
                             sink_video, nullptr)) {
    std::cerr << "Failed to link video elements" << std::endl;
    return -1;
  }

  if (!gst_element_link_many(depay_audio, decode_audio, convert_audio,
                             resample_audio, sink_audio, nullptr)) {
    std::cerr << "Failed to link audio elements" << std::endl;
    return -1;
  }

  g_signal_connect(src, "pad-added", G_CALLBACK(on_pad_added), depay_video);
  g_signal_connect(src, "pad-added", G_CALLBACK(on_pad_added_audio),
                   depay_audio);

  GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    std::cerr << "Failed to set pipeline state to playing" << std::endl;
    return -1;
  }

  GstBus* bus = gst_element_get_bus(pipeline);
  GstMessage* msg = nullptr;
  bool terminate = false;

  while (!terminate) {
    msg = gst_bus_timed_pop_filtered(
        bus, GST_CLOCK_TIME_NONE,
        static_cast<GstMessageType>(GST_MESSAGE_STATE_CHANGED |
                                    GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

    if (msg != nullptr) {
      GError* err;
      gchar* debug_info;

      switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_ERROR:
          gst_message_parse_error(msg, &err, &debug_info);
          std::cerr << "Error received from element "
                    << GST_OBJECT_NAME(msg->src) << ": " << err->message
                    << std::endl;
          std::cerr << "Debugging information: "
                    << (debug_info ? debug_info : "none") << std::endl;
          g_clear_error(&err);
          g_free(debug_info);
          terminate = true;
          break;
        case GST_MESSAGE_EOS:
          std::cout << "End-Of-Stream reached." << std::endl;
          terminate = true;
          break;
        case GST_MESSAGE_STATE_CHANGED:
          if (GST_MESSAGE_SRC(msg) == GST_OBJECT(pipeline)) {
            GstState old_state, new_state, pending_state;
            gst_message_parse_state_changed(msg, &old_state, &new_state,
                                            &pending_state);
            std::cout << "Pipeline state changed from "
                      << gst_element_state_get_name(old_state) << " to "
                      << gst_element_state_get_name(new_state) << std::endl;
          }
          break;
        default:
          break;
      }
      gst_message_unref(msg);
    }
  }

  gst_object_unref(bus);
  gst_element_set_state(pipeline, GST_STATE_NULL);
  gst_object_unref(pipeline);
  gst_deinit();

  return 0;
}