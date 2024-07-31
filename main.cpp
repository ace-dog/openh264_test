#include <iostream>
#include "codec_api.h"
#include "codec_def.h"
#include "sample_frame.h"

int main() {
    // Initialize OpenH264 encoder
    ISVCEncoder* pEncoder = nullptr;
    int ret = WelsCreateSVCEncoder(&pEncoder);
    if (ret != 0 || pEncoder == nullptr) {
        std::cerr << "Failed to create encoder" << std::endl;
        return -1;
    }

    // Set encoder parameters
    SEncParamBase encParams;
    pEncoder->GetDefaultParams(&encParams);
    encParams.iUsageType = CAMERA_VIDEO_REAL_TIME;
    encParams.fMaxFrameRate = 30;
    encParams.iPicWidth = 640;
    encParams.iPicHeight = 480;
    encParams.iTargetBitrate = 500000; // 500 Kbps
    encParams.iRCMode = RC_ConstantQuality;
    encParams.iTemporalLayerNum = 1;
    encParams.iSpatialLayerNum = 1;
    ret = pEncoder->Initialize(&encParams);
    if (ret != 0) {
        std::cerr << "Failed to initialize encoder" << std::endl;
        return -1;
    }

    // Prepare sample NV12 frame
    int width = 640;
    int height = 480;
    int ySize = width * height;
    int uvSize = (width * height) / 2;

    unsigned char* yPlane = new unsigned char[ySize];
    unsigned char* uvPlane = new unsigned char[uvSize];
    GenerateSampleNV12Frame(yPlane, uvPlane, width, height);

    SFrameBSInfo info;
    memset(&info, 0, sizeof(SFrameBSInfo));

    SFrameSource frame;
    frame.iPicWidth = width;
    frame.iPicHeight = height;
    frame.iColorFormat = videoFormatI420; // NV12 is compatible with I420 format
    frame.pData[0] = yPlane; // Y plane
    frame.pData[1] = uvPlane; // UV plane
    frame.iStride[0] = width; // Stride for Y plane
    frame.iStride[1] = width; // Stride for UV plane
    frame.iStride[2] = 0; // Not used

    // Encode frame
    ret = pEncoder->EncodeFrame(&frame, &info);
    if (ret != 0) {
        std::cerr << "Failed to encode frame" << std::endl;
        delete[] yPlane;
        delete[] uvPlane;
        pEncoder->Uninitialize();
        WelsDestroySVCEncoder(pEncoder);
        return -1;
    }

    // Process encoded data
    for (int i = 0; i < info.iLayerNum; i++) {
        SLayerBSInfo layerInfo = info.sLayerInfo[i];
        for (int j = 0; j < layerInfo.iNalCount; j++) {
            SNalUnit* nal = &layerInfo.sNalBuf[j];
            std::cout << "Encoded NAL unit: size=" << nal->iSize << std::endl;
            // Process or save nal->pBuf (encoded data)
        }
    }

    // Clean up
    delete[] yPlane;
    delete[] uvPlane;
    pEncoder->Uninitialize();
    WelsDestroySVCEncoder(pEncoder);

    return 0;
}
