#ifndef _ROCKCHIP_CAMERA_CAMERA_H
#define _ROCKCHIP_CAMERA_CAMERA_H

#include <linux/ioctl.h>
#include <linux/types.h>
#include <linux/v4l2-common.h>
#include <linux/v4l2-controls.h>
#include <linux/videodev2.h>
#include "stdint.h"

class camera{
    public:
        camera(char const *dev_name);
        ~camera();
        int fd;
        int dma_fd;
        
        uint64_t frame_length;

        void enqueue_frame(int index = 0);
        int dequeue_frame();

        uint8_t *buffer;
    
    private:
        v4l2_plane plane = {0};

};

#endif