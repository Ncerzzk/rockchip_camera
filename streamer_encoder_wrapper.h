#ifndef _ROCKCHIP_CAMERA_ENCODER_WITH_STREAMER_H
#define _ROCKCHIP_CAMERA_ENCODER_WITH_STREAMER_H

#include "encoder.h"
#include "streamer_encoder.h"

class streamer_encoder_wrapper:public encoder{
    public:
    streamer_encoder_wrapper(char *target_ip,int port):encoder(){
        streamer_encoder_init(H265,target_ip, port);
    }

    void packet_handle_callback(uint8_t *ptr, size_t len) override {
        stream_encoder_handle_packet(ptr,len);
    }

};

#endif