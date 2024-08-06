#include <iostream>
#include <vector>
#include <fstream>
#include <cstdint>
#include <cstring>
#include "codec_api.h"
#include "codec_def.h"
#include "codec_app_def.h"
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <iostream>
#include <fstream>
#include <vector>

static gboolean push_data(GstAppSrc *appsrc, std::vector<uint8_t> *encodedData) {
    
    std::cout << "push_data" << std::endl;
    static GstClockTime timestamp = 0;
    GstBuffer *buffer;
    GstMapInfo map;
    GstFlowReturn ret;
    // Check if there is data to send
    if (encodedData->empty()) {
        g_signal_emit_by_name(appsrc, "end-of-stream", &ret);
        return FALSE;
    }
    std::cout << "gst_buffer_new_allocate: "<< encodedData->size() << std::endl;
    buffer = gst_buffer_new_allocate(nullptr, encodedData->size(), nullptr);
    // std::cout << "gst_buffer_new_allocate" << std::endl;
    // gst_buffer_map(buffer, &map, GST_MAP_WRITE);
    // std::cout << "gst_buffer_map" << std::endl;
    // memcpy(map.data, frame_data->data(), frame_data->size());
    // std::cout << "memcpy" << std::endl;
    // gst_buffer_unmap(buffer, &map);
    // std::cout << "gst_buffer_unmap" << std::endl;

    // std::cout << "gst_buffer_unmap" << std::endl;
    // /* Set buffer timestamp and duration */
    // GST_BUFFER_PTS(buffer) = timestamp;
    // GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale_int(1, GST_SECOND, 30); // Assuming 30 FPS
    // timestamp += GST_BUFFER_DURATION(buffer);

    // std::cout << "GST_BUFFER_DURATION" << std::endl;
    // g_signal_emit_by_name(appsrc, "push-buffer", buffer, &ret);
    gst_buffer_unref(buffer);

    // std::cout << "gst_buffer_unref" << std::endl;
    // if (ret != GST_FLOW_OK) {
    //     return FALSE;
    // }

    return TRUE;
}

// Function to create a synthetic NV12 frame
void createNV12Frame(uint8_t* yPlane, uint8_t* uvPlane, int width, int height) {
    // Fill the Y plane with a gradient pattern
    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {
            yPlane[i * width + j] = (i + j) % 256;
        }
    }

    // Fill the UV plane with a constant color (128, 128) - grey
    for (int i = 0; i < height / 2; ++i) {
        for (int j = 0; j < width; j += 2) {
            uvPlane[i * width + j] = 128;     // U component
            uvPlane[i * width + j + 1] = 128; // V component
        }
    }
}

// Function to save the encoded H.264 frame to a file
void saveToFile(const std::string& fileName, const std::vector<uint8_t>& data) {
    std::ofstream outFile(fileName, std::ios::binary);
    outFile.write(reinterpret_cast<const char*>(data.data()), data.size());
}

int main(int argc, char *argv[]) {
    const int width = 1280;
    const int height = 720;

    // Allocate memory for NV12 frame
    std::vector<uint8_t> yPlane(width * height);
    std::vector<uint8_t> uvPlane(width * height / 2);

    // Create a synthetic NV12 frame
    createNV12Frame(yPlane.data(), uvPlane.data(), width, height);

    // Initialize the encoder
    ISVCEncoder* encoder = nullptr;
    SEncParamBase param;

    if (WelsCreateSVCEncoder(&encoder) != 0) {
        std::cerr << "Failed to create encoder" << std::endl;
        return -1;
    }

    param.iUsageType = CAMERA_VIDEO_REAL_TIME;
    param.fMaxFrameRate = 30.0;
    param.iPicWidth = width;
    param.iPicHeight = height;
    param.iTargetBitrate = 500000; // 500 kbps
    param.iRCMode = RC_QUALITY_MODE;

    if (encoder->Initialize(&param) != 0) {
        std::cerr << "Failed to initialize encoder" << std::endl;
        return -1;
    }

    // Create input frame
    SSourcePicture pic = {0};
    pic.iColorFormat = videoFormatI420;  // OpenH264 expects I420 format
    pic.iPicWidth = width;
    pic.iPicHeight = height;
    pic.iStride[0] = width;
    pic.iStride[1] = width / 2;
    pic.iStride[2] = width / 2;

    // Allocate memory for I420 frame
    std::vector<uint8_t> yuvPlane(width * height * 3 / 2);
    uint8_t* i420_y = yuvPlane.data();
    uint8_t* i420_u = i420_y + width * height;
    uint8_t* i420_v = i420_u + width * height / 4;

    // Convert NV12 to I420
    std::memcpy(i420_y, yPlane.data(), width * height);
    for (int i = 0; i < height / 2; ++i) {
        for (int j = 0; j < width / 2; ++j) {
            i420_u[i * (width / 2) + j] = uvPlane[i * width + j * 2];
            i420_v[i * (width / 2) + j] = uvPlane[i * width + j * 2 + 1];
        }
    }

    pic.pData[0] = i420_y;
    pic.pData[1] = i420_u;
    pic.pData[2] = i420_v;

    // Encode the frame
    SFrameBSInfo info = {0};
    int result = encoder->EncodeFrame(&pic, &info);
    if (result != cmResultSuccess) {
        std::cerr << "Failed to encode frame" << std::endl;
        return -1;
    }

    // Collect the encoded data
    std::vector<uint8_t> encodedData;
    for (int i = 0; i < info.iLayerNum; ++i) {
        SLayerBSInfo& layerInfo = info.sLayerInfo[i];
        for (int j = 0; j < layerInfo.iNalCount; ++j) {
            encodedData.insert(encodedData.end(),
                               layerInfo.pBsBuf + layerInfo.pNalLengthInByte[j] - layerInfo.pNalLengthInByte[j],
                               layerInfo.pBsBuf + layerInfo.pNalLengthInByte[j]);
        }
    }

    std::cout << "encodedData->size() " << encodedData.size() << std::endl;
    // Save the encoded frame to a file
    saveToFile("output.h264", encodedData);



// GstElement *pipeline, *appsrc, *parse, *pay, *sink;
//     GMainLoop *main_loop;
//     std::ifstream file("output.h264", std::ios::in | std::ios::binary);

//     if (!file.is_open()) {
//         std::cerr << "Failed to open H.264 file." << std::endl;
//         return -1;
//     }

//     // Read the entire file into memory
//     std::vector<char> frame_data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
//     file.close();
//     std::cout << "frame_data->size() " << frame_data.size() << std::endl;

//     gst_init(&argc, &argv);

//     /* Create the elements */
//     appsrc = gst_element_factory_make("appsrc", "source");
//     parse = gst_element_factory_make("mpegtsmux", "muxer");
//     sink = gst_element_factory_make("srtsink", "sink");

//     /* Create the empty pipeline */
//     pipeline = gst_pipeline_new("test-pipeline");

//     if (!pipeline || !appsrc || !parse || !sink) {
//         g_printerr("Not all elements could be created.\n");
//         return -1;
//     }

//     /* Build the pipeline */
//     gst_bin_add_many(GST_BIN(pipeline), appsrc, parse, sink, NULL);
//     if (gst_element_link_many(appsrc, parse, sink, NULL) != TRUE) {
//         g_printerr("Elements could not be linked.\n");
//         gst_object_unref(pipeline);
//         return -1;
//     }

//     /* Set the properties of the elements */
//     g_object_set(sink, "uri", "srt://192.168.20.15:5000?mode=listener&latency=120", NULL);
//     g_object_set(appsrc, "caps",
//                  gst_caps_new_simple("video/x-h264",
//                                      "stream-format", G_TYPE_STRING, "byte-stream",
//                                      "alignment", G_TYPE_STRING, "au",
//                                      "width", G_TYPE_INT, 1280,
//                                      "height", G_TYPE_INT, 720,
//                                      "framerate", GST_TYPE_FRACTION, 30, 1,
//                                      NULL), NULL);

//     g_object_set(appsrc, "format", GST_FORMAT_TIME, NULL);
//     g_signal_connect(appsrc, "need-data", G_CALLBACK(push_data), &encodedData);

//     /* Start playing */
//     gst_element_set_state(pipeline, GST_STATE_PLAYING);

//         std::cout << "main loop" << std::endl;
//     /* Create a GLib Main Loop and set it to run */
//     main_loop = g_main_loop_new(NULL, FALSE);
//     std::cout << "main_loop" << std::endl;
//     g_main_loop_run(main_loop);
//     std::cout << "g_main_loop_run" << std::endl;

//     /* Free resources */
//     gst_element_set_state(pipeline, GST_STATE_NULL);
//     std::cout << "gst_element_set_state" << std::endl;
//     gst_object_unref(GST_OBJECT(pipeline));
//     std::cout << "gst_object_unref" << std::endl;
//     g_main_loop_unref(main_loop);
//     std::cout << "g_main_loop_unref" << std::endl;

    // Clean up
    if (encoder) {
        encoder->Uninitialize();
        WelsDestroySVCEncoder(encoder);
    }
}
