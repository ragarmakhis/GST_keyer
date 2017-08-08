#include <gst/gst.h>
//#include <gst/video/video.h>

/* Structure to contain all our information, so we can pass it to callbacks */
typedef struct _CustomBinData {
  GstElement *pipeline;
  GstElement *sourceFG;
  GstElement *sourceBG;
  GstElement *alpha;
  GstElement *convert;
  GstElement *mixer;
  GstElement *sink;
} CustomBinData;

enum GstVideoTestSrcPattern {
  GST_VIDEO_TEST_SRC_SMPTE,
  GST_VIDEO_TEST_SRC_SNOW,
  GST_VIDEO_TEST_SRC_BLACK,
  GST_VIDEO_TEST_SRC_WHITE,
  GST_VIDEO_TEST_SRC_RED,
  GST_VIDEO_TEST_SRC_GREEN,
  GST_VIDEO_TEST_SRC_BLUE,
  GST_VIDEO_TEST_SRC_CHECKERS1,
  GST_VIDEO_TEST_SRC_CHECKERS2,
  GST_VIDEO_TEST_SRC_CHECKERS4,
  GST_VIDEO_TEST_SRC_CHECKERS8,
  GST_VIDEO_TEST_SRC_CIRCULAR,
  GST_VIDEO_TEST_SRC_BLINK,
  GST_VIDEO_TEST_SRC_SMPTE75,
  GST_VIDEO_TEST_SRC_ZONE_PLATE,
  GST_VIDEO_TEST_SRC_GAMUT,
  GST_VIDEO_TEST_SRC_CHROMA_ZONE_PLATE,
  GST_VIDEO_TEST_SRC_SOLID,
  GST_VIDEO_TEST_SRC_BALL,
  GST_VIDEO_TEST_SRC_SMPTE100,
  GST_VIDEO_TEST_SRC_BAR,
  GST_VIDEO_TEST_SRC_PINWHEEL,
  GST_VIDEO_TEST_SRC_SPOKES,
  GST_VIDEO_TEST_SRC_GRADIENT,
  GST_VIDEO_TEST_SRC_COLORS
};

enum GstAlphaMethod {
  ALPHA_METHOD_SET,
  ALPHA_METHOD_GREEN,
  ALPHA_METHOD_BLUE,
  ALPHA_METHOD_CUSTOM
};


int main(int argc, char *argv[]) {
  CustomBinData data;
  GstBus *bus;
  GstMessage *msg;
  GstStateChangeReturn ret;
  gboolean terminate = FALSE;

//  GstVideoTestSrcPattern patternBG, patternFG;

  /* Initialize GStreamer */
  gst_init (&argc, &argv);

  /* Create the elements */
  data.sourceFG = gst_element_factory_make ("videotestsrc", "sourceFG");
  data.sourceBG = gst_element_factory_make ("videotestsrc", "sourceBG");
  data.alpha = gst_element_factory_make ("alpha", "alpha");
  data.mixer = gst_element_factory_make ("videomixer", "mixer");
  data.convert = gst_element_factory_make ("videoconvert", "convert");
  data.sink = gst_element_factory_make ("autovideosink", "sink");

  /* Create the empty pipeline */
  data.pipeline = gst_pipeline_new ("test-pipeline");

  if (!data.pipeline || !data.sourceBG || !data.sourceFG || !data.alpha || !data.mixer
                     || !data.convert || !data.sink) {
    g_printerr ("Not all elements could be created.\n");
    return -1;
  }

  //patternBG = GST_VIDEO_TEST_SRC_SNOW;
  //patternFG = GST_VIDEO_TEST_SRC_SMPTE75;

  g_object_set (data.sourceBG, "pattern", GST_VIDEO_TEST_SRC_SNOW, NULL);
  g_object_set (data.sourceFG, "pattern", GST_VIDEO_TEST_SRC_SMPTE75, NULL);
  g_object_set (data.alpha, "method", ALPHA_METHOD_CUSTOM,
                            "target-r", 0, 
                            "target-g", 0, 
                            "target-b", 255, NULL);

  /* Build the pipeline. Note that we are NOT linking the source at this
   * point. We will do it later. */
  gst_bin_add_many (GST_BIN (data.pipeline), data.sourceBG, data.sourceFG, data.alpha,
                    data.mixer, data.convert , data.sink, NULL);
  if (gst_element_link_many (data.sourceBG, data.mixer, NULL) != TRUE ||
      gst_element_link_many (data.sourceFG, data.alpha, data.mixer, NULL) != TRUE ||
      gst_element_link_many (data.mixer, data.convert, data.sink, NULL) != TRUE) {
    g_printerr ("Elements could not be linked.\n");
    gst_object_unref (data.pipeline);
    return -1;
  }

  /* Start playing */
  ret = gst_element_set_state (data.pipeline, GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    g_printerr ("Unable to set the pipeline to the playing state.\n");
    gst_object_unref (data.pipeline);
    return -1;
  }

  /* Listen to the bus */
  bus = gst_element_get_bus (data.pipeline);
  do {
    msg = gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE,
        GST_MESSAGE_STATE_CHANGED | GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

    /* Parse message */
    if (msg != NULL) {
      GError *err;
      gchar *debug_info;

      switch (GST_MESSAGE_TYPE (msg)) {
        case GST_MESSAGE_ERROR:
          gst_message_parse_error (msg, &err, &debug_info);
          g_printerr ("Error received from element %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
          g_printerr ("Debugging information: %s\n", debug_info ? debug_info : "none");
          g_clear_error (&err);
          g_free (debug_info);
          terminate = TRUE;
          break;
        case GST_MESSAGE_EOS:
          g_print ("End-Of-Stream reached.\n");
          terminate = TRUE;
          break;
        case GST_MESSAGE_STATE_CHANGED:
          /* We are only interested in state-changed messages from the pipeline */
          if (GST_MESSAGE_SRC (msg) == GST_OBJECT (data.pipeline)) {
            GstState old_state, new_state, pending_state;
            gst_message_parse_state_changed (msg, &old_state, &new_state, &pending_state);
            g_print ("Pipeline state changed from %s to %s:\n",
                gst_element_state_get_name (old_state), gst_element_state_get_name (new_state));
          }
          break;
        default:
          /* We should not reach here */
          g_printerr ("Unexpected message received.\n");
          break;
      }
      gst_message_unref (msg);
    }
  } while (!terminate);

  /* Free resources */
  gst_object_unref (bus);
  gst_element_set_state (data.pipeline, GST_STATE_NULL);
  gst_object_unref (data.pipeline);
  return 0;
}

