#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <linux/ioctl.h>
#include <linux/types.h>
#include <linux/v4l2-common.h>
#include <linux/v4l2-controls.h>
#include <linux/videodev2.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>
#include <fstream>
#include <string>

using namespace std;
#define eprintf(args...) fprintf(stderr, ##args)

#define cam_ioctl(fd,cmd,arg)  \
do {\
    if(ioctl(fd,cmd,arg)<0){ \
        eprintf("failed to do ioctl:" #cmd "\n"); \
        abort();\
    }\
}while(0)

int main() {
    // 1.  Open the device
    int fd; // A file descriptor to the video device
    fd = open("/dev/video0",O_RDWR);
    if(fd < 0){
        perror("Failed to open device, OPEN");
        return 1;
    }

    // v4l2-ctl -d /dev/video0 --get-fmt-video
    // 

    // 2. Ask the device if it can capture frames
    v4l2_capability capability;
    cam_ioctl(fd, VIDIOC_QUERYCAP, &capability);

    
    // 3. Set Image format
    v4l2_format imageFormat = {0};
    imageFormat.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    imageFormat.fmt.pix_mp.width = 640;
    imageFormat.fmt.pix_mp.height = 480;
    imageFormat.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_NV12;
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
    v4l2_plane plane = {0};
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
    char* buffer = (char*)mmap(NULL, plane.length, PROT_READ | PROT_WRITE, MAP_SHARED,
                        fd, plane.data_offset);
    memset(buffer, 0, plane.length);


    struct v4l2_exportbuffer expbuf={0};
    int dma_fd;
    memset(&expbuf, 0, sizeof(expbuf));
    expbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    expbuf.index = 0;
    expbuf.plane = 0;
    expbuf.flags = O_CLOEXEC;
    cam_ioctl(fd, VIDIOC_EXPBUF, &expbuf);

    dma_fd = expbuf.fd;

    // Activate streaming
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    cam_ioctl(fd, VIDIOC_STREAMON, &type);

/***************************** Begin looping here *********************/
    // Queue the buffer

    cam_ioctl(fd, VIDIOC_QBUF, &queryBuffer);

    // Dequeue the buffer
    cam_ioctl(fd, VIDIOC_DQBUF, &queryBuffer);

    // Frames get written after dequeuing the buffer

    cout << "Buffer has: " << (double)plane.bytesused / 1024
            << " KBytes of data" << endl;

    // Write the data out to file
    ofstream outFile;
    outFile.open("webcam_output.jpeg", ios::binary| ios::app);

    int bufPos = 0, outFileMemBlockSize = 0;  // the position in the buffer and the amoun to copy from
                                        // the buffer
    int remainingBufferSize = plane.bytesused; // the remaining buffer size, is decremented by
                                                    // memBlockSize amount on each loop so we do not overwrite the buffer
    char* outFileMemBlock = NULL;  // a pointer to a new memory block
    int itr = 0; // counts thenumber of iterations
    while(remainingBufferSize > 0) {
        bufPos += outFileMemBlockSize;  // increment the buffer pointer on each loop
                                        // initialise bufPos before outFileMemBlockSize so we can start
                                        // at the begining of the buffer

        outFileMemBlockSize = 1024;    // set the output block size to a preferable size. 1024 :)
        outFileMemBlock = new char[sizeof(char) * outFileMemBlockSize];

        // copy 1024 bytes of data starting from buffer+bufPos
        memcpy(outFileMemBlock, buffer+bufPos, outFileMemBlockSize);
        outFile.write(outFileMemBlock,outFileMemBlockSize);

        // calculate the amount of memory left to read
        // if the memory block size is greater than the remaining
        // amount of data we have to copy
        if(outFileMemBlockSize > remainingBufferSize)
            outFileMemBlockSize = remainingBufferSize;

        // subtract the amount of data we have to copy
        // from the remaining buffer size
        remainingBufferSize -= outFileMemBlockSize;

        // display the remaining buffer size
        cout << itr++ << " Remaining bytes: "<< remainingBufferSize << endl;
        
        delete outFileMemBlock;
    }

    // Close the file
    outFile.close();


/******************************** end looping here **********************/

    // end streaming
    cam_ioctl(fd, VIDIOC_STREAMOFF, &type);

    close(fd);
    return 0;
}
