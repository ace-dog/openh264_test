#include "sample_frame.h"
#include <cstring> // For memset

void GenerateSampleNV12Frame(unsigned char* yPlane, unsigned char* uvPlane, int width, int height) {
    // Fill Y plane with a solid color (e.g., gray)
    memset(yPlane, 128, width * height);

    // Fill UV plane with a solid color (e.g., purple)
    memset(uvPlane, 128, (width * height) / 2);
}
