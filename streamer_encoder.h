#ifndef _ROCKCHIP_CAMERA_STREAMER_ENCODER_H
#define _ROCKCHIP_CAMERA_STREAMER_ENCODER_H


#include "stdlib.h"
#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum{
    H264,
    H265
}encoder_type;

void stream_encoder_handle_packet(uint8_t *ptr, size_t len);
void streamer_encoder_init(encoder_type t, char *target_ip, int port);

#ifdef __cplusplus
}
#endif

#endif