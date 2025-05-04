#ifndef _ROCKCHIP_CAMERA_ENCODER_H
#define _ROCKCHIP_CAMERA_ENCODER_H

#include "stdint.h"

int encoder_init(int dma_fd, uint64_t buf_len);
#endif