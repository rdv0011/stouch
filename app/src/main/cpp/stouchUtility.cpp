//
// Created by Dmitry Rybakov on 2018-02-10.
//

#include <math.h>
#include <malloc.h>

#include "stouchFreenect.h"
#include "stouchUtility.h"

STouchUtility::STouchUtility() {
    depthBuffer.reserve(STouchUtilityConst::DepthBufferPix * 3);

    for (int i=0; i < STouchUtilityConst::GammaSize; i++) {
        float v = i / double(STouchUtilityConst::GammaSize);
        v = powf(v, 3) * 6;
        tGamma[i] = v * 6 * 256;
    }
}

void STouchUtility::convertDepth(void *v_depth) {
    uint16_t *depth = (uint16_t*)v_depth;
    for (int i = 0; i < STouchUtilityConst::DepthBufferPix; i++) {
        int pval = tGamma[depth[i]];
        int lb = pval & 0xff;
        switch (pval>>8) {
            case 0:
                depthBuffer[3*i+0] = 255;
                depthBuffer[3*i+1] = 255-lb;
                depthBuffer[3*i+2] = 255-lb;
                break;
            case 1:
                depthBuffer[3*i+0] = 255;
                depthBuffer[3*i+1] = lb;
                depthBuffer[3*i+2] = 0;
                break;
            case 2:
                depthBuffer[3*i+0] = 255-lb;
                depthBuffer[3*i+1] = 255;
                depthBuffer[3*i+2] = 0;
                break;
            case 3:
                depthBuffer[3*i+0] = 0;
                depthBuffer[3*i+1] = 255;
                depthBuffer[3*i+2] = lb;
                break;
            case 4:
                depthBuffer[3*i+0] = 0;
                depthBuffer[3*i+1] = 255-lb;
                depthBuffer[3*i+2] = 255;
                break;
            case 5:
                depthBuffer[3*i+0] = 0;
                depthBuffer[3*i+1] = 0;
                depthBuffer[3*i+2] = 255-lb;
                break;
            default:
                depthBuffer[3*i+0] = 0;
                depthBuffer[3*i+1] = 0;
                depthBuffer[3*i+2] = 0;
                break;
        }
    }
}
