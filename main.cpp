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

#include "config.h"
#include "encoder.h"

#include "camera.h"

using namespace std;


int main() {
    // 1.  Open the device

    camera cam("/dev/video0");
    encoder_init(cam.fd, cam.frame_length);

    cam.enqueue_frame();
    cam.dequeue_frame();

    // Frames get written after dequeuing the buffer

    cout << "Buffer has: " << (double)cam.frame_length / 1024
            << " KBytes of data" << endl;

    // Write the data out to file
    ofstream outFile;
    outFile.open("webcam_output.jpeg", ios::binary| ios::app);

    int bufPos = 0, outFileMemBlockSize = 0;  // the position in the buffer and the amoun to copy from
                                        // the buffer
    int remainingBufferSize = cam.frame_length; // the remaining buffer size, is decremented by
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
        memcpy(outFileMemBlock, cam.buffer+bufPos, outFileMemBlockSize);
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
    return 0;
}
