#include <iostream>
#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>
#include <fstream>
#include <string>

#include "config.h"
#include "camera.h"

#define eprintf(args...) fprintf(stderr, ##args)

#define cam_ioctl(fd,cmd,arg)  \
do {\
    if(ioctl(fd,cmd,arg)<0){ \
        eprintf("failed to do ioctl:" #cmd "\n"); \
        perror("reason:"); \
        abort();\
    }\
}while(0)

camera::camera(char *dev_name){
    fd = open(dev_name,O_RDWR);
    if(fd < 0){
        perror("Failed to open device, OPEN");
        abort();
    }

    // v4l2-ctl -d /dev/video0 --get-fmt-video

    // 2. Ask the device if it can capture frames
    v4l2_capability capability;
    cam_ioctl(fd, VIDIOC_QUERYCAP, &capability);
    
    // 3. Set Image format
    v4l2_format imageFormat = {0};
    imageFormat.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    imageFormat.fmt.pix_mp.width = SENSOR_WIDTH;
    imageFormat.fmt.pix_mp.height = SENSOR_HEIGHT;
    imageFormat.fmt.pix_mp.pixelformat = V4L2_FMT;
    imageFormat.fmt.pix_mp.field = V4L2_FIELD_NONE;
    imageFormat.fmt.pix_mp.num_planes = 1;

    // tell the device you are using this format
    cam_ioctl(fd, VIDIOC_S_FMT, &imageFormat);


    // 4. Request Buffers from the device
    v4l2_requestbuffers requestBuffer = {0};
    requestBuffer.count = 1; // one request buffer
    requestBuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE; // request a buffer wich we an use for capturing frames
    requestBuffer.memory = V4L2_MEMORY_MMAP;

    cam_ioctl(fd, VIDIOC_REQBUFS, &requestBuffer);

    // 5. Quety the buffer to get raw data ie. ask for the you requested buffer
    // and allocate memory for it
    
    v4l2_buffer queryBuffer = {0};
    queryBuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    queryBuffer.memory = V4L2_MEMORY_MMAP;
    queryBuffer.index = 0;
    queryBuffer.m.planes = &plane;
    queryBuffer.length = 1;

    cam_ioctl(fd, VIDIOC_QUERYBUF, &queryBuffer);

    // use a pointer to point to the newly created buffer
    // mmap() will map the memory address of the device to
    // an address in memory
    buffer = (uint8_t *)mmap(NULL, plane.length, PROT_READ | PROT_WRITE, MAP_SHARED,
                        fd, plane.data_offset);
    memset(buffer, 0, plane.length);


    struct v4l2_exportbuffer expbuf={0};
    memset(&expbuf, 0, sizeof(expbuf));
    expbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    expbuf.index = 0;
    expbuf.plane = 0;
    expbuf.flags = O_CLOEXEC;
    cam_ioctl(fd, VIDIOC_EXPBUF, &expbuf);

    // Activate streaming
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    cam_ioctl(fd, VIDIOC_STREAMON, &type);

    dma_fd = expbuf.fd;
    frame_length = plane.length;
};

camera::~camera(){
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    cam_ioctl(fd, VIDIOC_STREAMOFF, &type);

    v4l2_requestbuffers requestBuffer = {0};
    requestBuffer.count = 0; // set to zero to release buffer
    requestBuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    requestBuffer.memory = V4L2_MEMORY_MMAP;

    cam_ioctl(fd, VIDIOC_REQBUFS, &requestBuffer);

    close(fd);
}

void camera::dequeue_frame(){
    v4l2_buffer tmp = {0};
    tmp.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    tmp.memory = V4L2_MEMORY_MMAP;
    tmp.index = 0;
    tmp.m.planes = &plane;
    tmp.length = 1;
    cam_ioctl(fd, VIDIOC_DQBUF, &tmp);
}

void camera::enqueue_frame(){
    v4l2_buffer tmp = {0};
    tmp.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    tmp.memory = V4L2_MEMORY_MMAP;
    tmp.index = 0;
    tmp.m.planes = &plane;
    tmp.length = 1;
    cam_ioctl(fd, VIDIOC_QBUF, &tmp); 
}
