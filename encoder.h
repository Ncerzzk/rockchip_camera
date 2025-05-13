#ifndef _ROCKCHIP_CAMERA_ENCODER_H
#define _ROCKCHIP_CAMERA_ENCODER_H

#include "stdint.h"
#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_time.h"
#include "mpp_debug.h"
#include "mpp_common.h"
#include "mpp_soc.h"

#include "mpp_rc_api.h"

#include "rk_mpi.h"

#include "camera.h"
#include "config.h"

#include "stdio.h"

class encoder
{
public:
    encoder();
    ~encoder();
    camera cam;
    void run();

    MppBufferGroup buf_grp;
    MppBuffer cam_buffers[10];
    MppBuffer pkt_buf, md_info;
    MppCtx ctx;
    MppApi *mpi;

    bool pkt_eos;

    MppEncCfg cfg;

    virtual void packet_handle_callback(uint8_t *ptr, size_t len) {};
};

class encoder_test_to_file : public encoder
{
public:
    encoder_test_to_file() : encoder()
    {
        fp_output = fopen("test.mp4", "w+b");
    }

    ~encoder_test_to_file()
    {
        fclose(fp_output);
    }

    void packet_handle_callback(uint8_t *ptr, size_t len) override
    {
        fwrite(ptr, 1, len, fp_output);
    }

private:
    FILE *fp_output;
};

#include <sys/socket.h>
#include <arpa/inet.h>
#include "memory.h"

class encoder_test_to_udp : public encoder
{

public:
    int sock;
    struct sockaddr_in dest_addr;
    encoder_test_to_udp() : encoder()
    {
        sock = socket(AF_INET, SOCK_DGRAM, 0);

        if (sock == -1)
        {
            perror("socket creation failed");
            abort();
        }

        // 2. 设置目标地址（本地端口）

        memset(&dest_addr, 0, sizeof(dest_addr));
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(5600);
        dest_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // 目标 IP（本地环回地址 127.0.0.1）

        // command to rtp parse and send to target:
        // gst-launch-1.0 udpsrc port=5600  ! h265parse ! rtph265pay  ! udpsink host=192.168.18.248  port=5602

        // flow test:
        // test src: gst-launch-1.0 videotestsrc ! videoconvert ! x265enc tune=zerolatency ! udpsink host=127.0.0.1 port=5600
        // rtp parse:gst-launch-1.0 udpsrc port=5600  ! h265parse ! rtph265pay  ! udpsink host=127.0.0.1 port=5602
        // recv and play:
        // gst-launch-1.0   udpsrc port=5602 caps="application/x-rtp,encoding-name=H265" !   rtpjitterbuffer !   rtph265depay !   avdec_h265 !   videoconvert !   autovideosink
    }

    ~encoder_test_to_udp()
    {
        close(sock);
    }

    void packet_handle_callback(uint8_t *ptr, size_t len) override
    {
        ssize_t sent_bytes = sendto(
            sock,
            ptr,
            len,
            0,
            (struct sockaddr *)&dest_addr,
            sizeof(dest_addr));

        if (sent_bytes == -1)
        {
            perror("sendto failed");
            close(sock);
            abort();
        }
    }
};

#endif