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

class encoder_test_fp : public encoder
{
public:
    encoder_test_fp() : encoder()
    {
        fp_output = fopen("test.mp4", "w+b");
    }

    ~encoder_test_fp()
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
#endif