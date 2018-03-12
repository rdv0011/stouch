//
// Created by Dmitry Rybakov on 2018-01-13.
//
#include "freenect_internal.h"
#include "stouchFreenect.h"

STouchFreenectDevice::STouchFreenectDevice(freenect_context *_ctx,
                                           int _index):FreenectDevice(_ctx, _index) {
}