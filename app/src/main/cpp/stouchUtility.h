//
// Created by Dmitry Rybakov on 2018-02-10.
//

#pragma once

#include <cstdint>
#include <vector>

namespace STouchUtilityConst {
    static const int GammaSize = 2048;
    static const int DepthBufferPix = FREENECT_FRAME_PIX;
}

class STouchUtility {
public:
    STouchUtility();
private:
    void convertDepth(void *v_depth);
private:
    // Depth gamma
    uint16_t tGamma[STouchUtilityConst::GammaSize];
    std::vector<uint8_t> depthBuffer;
};
