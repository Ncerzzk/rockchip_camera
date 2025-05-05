#include "encoder.h"
#include "stdlib.h"
#include "memory.h"
#include "signal.h"


static RK_S32 qbias_arr_hevc[18] = {
    3, 6, 13, 171, 171, 171, 171,
    3, 6, 13, 171, 171, 220, 171, 85, 85, 85, 85
};

static RK_S32 qbias_arr_avc[18] = {
    3, 6, 13, 683, 683, 683, 683,
    3, 6, 13, 683, 683, 683, 683, 341, 341, 341, 341
};

static RK_S32 aq_rnge_arr[10] = {
    5, 5, 10, 12, 12,
    5, 5, 10, 12, 12
};

static RK_S32 aq_thd_smart[16] = {
    1,  3,  3,  3,  3,  3,  5,  5,
    8,  8,  8, 15, 15, 20, 25, 28
};

static RK_S32 aq_step_smart[16] = {
    -8, -7, -6, -5, -4, -3, -2, -1,
    0,  1,  2,  3,  4,  6,  8, 10
};

static RK_S32 aq_thd[16] = {
    0,  0,  0,  0,
    3,  3,  5,  5,
    8,  8,  8,  15,
    15, 20, 25, 25
};

static RK_S32 aq_step_i_ipc[16] = {
    -8, -7, -6, -5,
    -4, -3, -2, -1,
    0,  1,  2,  3,
    5,  7,  7,  8,
};

static RK_S32 aq_step_p_ipc[16] = {
    -8, -7, -6, -5,
    -4, -2, -1, -1,
    0,  2,  3,  4,
    6,  8,  9,  10,
};



static uint32_t get_mdinfo_size(int width, int height,MppCodingType type)
{
    RockchipSocType soc_type = mpp_get_soc_type();
    uint32_t md_size;
    uint32_t w = width, h = height;

    if (soc_type == ROCKCHIP_SOC_RK3588) {
        md_size = (MPP_ALIGN(w, 64) >> 6) * (MPP_ALIGN(h, 64) >> 6) * 32;
    } else {
        md_size = (MPP_VIDEO_CodingHEVC == type) ?
                  (MPP_ALIGN(w, 32) >> 5) * (MPP_ALIGN(h, 32) >> 5) * 16 :
                  (MPP_ALIGN(w, 64) >> 6) * (MPP_ALIGN(h, 16) >> 4) * 16;
    }

    return md_size;
}


static int get_framesize(int width, int height, MppFrameFormat fmt)
{
    int hor_stride = MPP_ALIGN(width, 16);
    int ver_stride = MPP_ALIGN(height, 16);

    int frame_size = 0;
    switch (fmt & MPP_FRAME_FMT_MASK)
    {
    case MPP_FMT_YUV420SP:
    case MPP_FMT_YUV420P:
    {
        frame_size = MPP_ALIGN(hor_stride, 64) * MPP_ALIGN(ver_stride, 64) * 3 / 2;
    }
    break;

    case MPP_FMT_YUV422_YUYV:
    case MPP_FMT_YUV422_YVYU:
    case MPP_FMT_YUV422_UYVY:
    case MPP_FMT_YUV422_VYUY:
    case MPP_FMT_YUV422P:
    case MPP_FMT_YUV422SP:
    {
        frame_size = MPP_ALIGN(hor_stride, 64) * MPP_ALIGN(ver_stride, 64) * 2;
    }
    break;
    case MPP_FMT_YUV400:
    case MPP_FMT_RGB444:
    case MPP_FMT_BGR444:
    case MPP_FMT_RGB555:
    case MPP_FMT_BGR555:
    case MPP_FMT_RGB565:
    case MPP_FMT_BGR565:
    case MPP_FMT_RGB888:
    case MPP_FMT_BGR888:
    case MPP_FMT_RGB101010:
    case MPP_FMT_BGR101010:
    case MPP_FMT_ARGB8888:
    case MPP_FMT_ABGR8888:
    case MPP_FMT_BGRA8888:
    case MPP_FMT_RGBA8888:
    {
        frame_size = MPP_ALIGN(hor_stride, 64) * MPP_ALIGN(ver_stride, 64);
    }
    break;

    default:
    {
        frame_size = MPP_ALIGN(hor_stride, 64) * MPP_ALIGN(ver_stride, 64) * 4;
    }
    break;
    }

    return frame_size;
}

static MPP_RET mpp_enc_cfg_setup(MppCtx ctx,MppApi *mpi,MppEncCfg cfg)
{
    MPP_RET ret;
    RK_U32 rotation;
    RK_U32 mirroring;
    RK_U32 flip;
    MppEncRefCfg ref = NULL;

    int rc_mode = MPP_ENC_RC_MODE_VBR; 
    MppCodingType type = MPP_ENCODE_TYPE;

    uint32_t height = MPP_OUT_HEIGHT;
    uint32_t width = MPP_OUT_WIDTH;

    int fps_out_num = 30;
    int fps_out_den = 1;

    int scene_mode = 1;  // ipc scene_mode


    uint64_t bps = width * height / 8 * (fps_out_num/ fps_out_den);

    /* setup preprocess parameters */
    mpp_enc_cfg_set_s32(cfg, "base:low_delay", 1);
    mpp_enc_cfg_set_s32(cfg, "prep:width", width);
    mpp_enc_cfg_set_s32(cfg, "prep:height", height);
    mpp_enc_cfg_set_s32(cfg, "prep:hor_stride", width);
    mpp_enc_cfg_set_s32(cfg, "prep:ver_stride", height);
    mpp_enc_cfg_set_s32(cfg, "prep:format", MPP_FMT);
    mpp_enc_cfg_set_s32(cfg, "prep:range", MPP_FRAME_RANGE_JPEG);

    mpp_env_get_u32("mirroring", &mirroring, 0);
    mpp_env_get_u32("rotation", &rotation, 0);
    mpp_env_get_u32("flip", &flip, 0);

    mpp_enc_cfg_set_s32(cfg, "prep:mirroring", mirroring);
    mpp_enc_cfg_set_s32(cfg, "prep:rotation", rotation);
    mpp_enc_cfg_set_s32(cfg, "prep:flip", flip);

    /* setup rate control parameters */
    mpp_enc_cfg_set_s32(cfg, "rc:mode", rc_mode);
    mpp_enc_cfg_set_u32(cfg, "rc:max_reenc_times", 0);
    mpp_enc_cfg_set_u32(cfg, "rc:super_mode", 0);

    /* fix input / output frame rate */
    mpp_enc_cfg_set_s32(cfg, "rc:fps_in_flex", 0);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_in_num", 30);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_in_denom", 1);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_out_flex", 0);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_out_num", fps_out_num);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_out_denom", fps_out_den);

    /* drop frame or not when bitrate overflow */
    mpp_enc_cfg_set_u32(cfg, "rc:drop_mode", MPP_ENC_RC_DROP_FRM_DISABLED);
    mpp_enc_cfg_set_u32(cfg, "rc:drop_thd", 20);        /* 20% of max bps */
    mpp_enc_cfg_set_u32(cfg, "rc:drop_gap", 1);         /* Do not continuous drop frame */

    /* setup bitrate for different rc_mode */
    mpp_enc_cfg_set_s32(cfg, "rc:bps_target", bps);
    switch (rc_mode) {
    case MPP_ENC_RC_MODE_FIXQP : {
        /* do not setup bitrate on FIXQP mode */
    } break;
    case MPP_ENC_RC_MODE_CBR : {
        /* CBR mode has narrow bound */
        mpp_enc_cfg_set_s32(cfg, "rc:bps_max",  bps * 17 / 16);
        mpp_enc_cfg_set_s32(cfg, "rc:bps_min",  bps * 15 / 16);
    } break;
    case MPP_ENC_RC_MODE_VBR :
    case MPP_ENC_RC_MODE_AVBR : {
        /* VBR mode has wide bound */
        mpp_enc_cfg_set_s32(cfg, "rc:bps_max",  bps * 17 / 16);
        mpp_enc_cfg_set_s32(cfg, "rc:bps_min",  bps * 1 / 16);
    } break;
    default : {
        /* default use CBR mode */
        mpp_enc_cfg_set_s32(cfg, "rc:bps_max",  bps * 17 / 16);
        mpp_enc_cfg_set_s32(cfg, "rc:bps_min",  bps * 15 / 16);
    } break;
    }

    /* setup qp for different codec and rc_mode */
    switch (type) {
    case MPP_VIDEO_CodingHEVC : {
        switch (rc_mode) {
        case MPP_ENC_RC_MODE_FIXQP : {
            RK_S32 fix_qp = 26;

            mpp_enc_cfg_set_s32(cfg, "rc:qp_init", fix_qp);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_max", fix_qp);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_min", fix_qp);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_max_i", fix_qp);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_min_i", fix_qp);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_ip", 0);
            mpp_enc_cfg_set_s32(cfg, "rc:fqp_min_i", fix_qp);
            mpp_enc_cfg_set_s32(cfg, "rc:fqp_max_i", fix_qp);
            mpp_enc_cfg_set_s32(cfg, "rc:fqp_min_p", fix_qp);
            mpp_enc_cfg_set_s32(cfg, "rc:fqp_max_p", fix_qp);
        } break;
        case MPP_ENC_RC_MODE_CBR :
        case MPP_ENC_RC_MODE_VBR :
        case MPP_ENC_RC_MODE_AVBR :
        case MPP_ENC_RC_MODE_SMTRC : {
            mpp_enc_cfg_set_s32(cfg, "rc:qp_init",  -1);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_max",  51);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_min",  10);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_max_i",  51);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_min_i",  10);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_ip", 2);
            mpp_enc_cfg_set_s32(cfg, "rc:fqp_min_i", 10);
            mpp_enc_cfg_set_s32(cfg, "rc:fqp_max_i",  45);
            mpp_enc_cfg_set_s32(cfg, "rc:fqp_min_p", 10);
            mpp_enc_cfg_set_s32(cfg, "rc:fqp_max_p",  45);
        } break;
        default : {
            mpp_err_f("unsupport encoder rc mode %d\n", rc_mode);
        } break;
        }
    } break;
    case MPP_VIDEO_CodingVP8 : {
        /* vp8 only setup base qp range */
        mpp_enc_cfg_set_s32(cfg, "rc:qp_init",  40);
        mpp_enc_cfg_set_s32(cfg, "rc:qp_max",   127);
        mpp_enc_cfg_set_s32(cfg, "rc:qp_min",   0);
        mpp_enc_cfg_set_s32(cfg, "rc:qp_max_i",  127);
        mpp_enc_cfg_set_s32(cfg, "rc:qp_min_i",  0);
        mpp_enc_cfg_set_s32(cfg, "rc:qp_ip", 6);
    } break;
    case MPP_VIDEO_CodingMJPEG : {
        /* jpeg use special codec config to control qtable */
        mpp_enc_cfg_set_s32(cfg, "jpeg:q_factor",  80);
        mpp_enc_cfg_set_s32(cfg, "jpeg:qf_max",  99);
        mpp_enc_cfg_set_s32(cfg, "jpeg:qf_min",  1);
    } break;
    default : {
    } break;
    }

    /* setup codec  */
    mpp_enc_cfg_set_s32(cfg, "codec:type", type);
    switch (type) {
    case MPP_VIDEO_CodingAVC : {
        RK_U32 constraint_set;

        /*
         * H.264 profile_idc parameter
         * 66  - Baseline profile
         * 77  - Main profile
         * 100 - High profile
         */
        mpp_enc_cfg_set_s32(cfg, "h264:profile", 100);
        /*
         * H.264 level_idc parameter
         * 10 / 11 / 12 / 13    - qcif@15fps / cif@7.5fps / cif@15fps / cif@30fps
         * 20 / 21 / 22         - cif@30fps / half-D1@@25fps / D1@12.5fps
         * 30 / 31 / 32         - D1@25fps / 720p@30fps / 720p@60fps
         * 40 / 41 / 42         - 1080p@30fps / 1080p@30fps / 1080p@60fps
         * 50 / 51 / 52         - 4K@30fps
         */
        mpp_enc_cfg_set_s32(cfg, "h264:level", 40);
        mpp_enc_cfg_set_s32(cfg, "h264:cabac_en", 1);
        mpp_enc_cfg_set_s32(cfg, "h264:cabac_idc", 0);
        mpp_enc_cfg_set_s32(cfg, "h264:trans8x8", 1);

        mpp_env_get_u32("constraint_set", &constraint_set, 0);
        if (constraint_set & 0x3f0000)
            mpp_enc_cfg_set_s32(cfg, "h264:constraint_set", constraint_set);
    } break;
    case MPP_VIDEO_CodingHEVC : {
        mpp_enc_cfg_set_s32(cfg, "h265:diff_cu_qp_delta_depth", 0);
    } break;
    case MPP_VIDEO_CodingMJPEG :
    case MPP_VIDEO_CodingVP8 : {
    } break;
    default : {
        mpp_err_f("unsupport encoder coding type %d\n", type);
    } break;
    }

    mpp_enc_cfg_set_s32(cfg, "split:mode", 0);
    // mpp_enc_cfg_set_s32(cfg, "split:arg", p->split_arg);
    // mpp_enc_cfg_set_s32(cfg, "split:out", p->split_out);


    // config gop_len and ref cfg
    mpp_enc_cfg_set_s32(cfg, "rc:gop", fps_out_num * 2);

    // mpp_env_get_u32("gop_mode", &gop_mode, gop_mode);
    // if (gop_mode) {
    //     mpp_enc_ref_cfg_init(&ref);

    //     if (p->gop_mode < 4)
    //         mpi_enc_gen_ref_cfg(ref, gop_mode);
    //     else
    //         mpi_enc_gen_smart_gop_ref_cfg(ref, p->gop_len, p->vi_len);

    //     mpp_enc_cfg_set_ptr(cfg, "rc:ref_cfg", ref);
    // }

    /* setup fine tuning paramters */
    // mpp_enc_cfg_set_s32(cfg, "tune:anti_flicker_str", p->anti_flicker_str);
    // mpp_enc_cfg_set_s32(cfg, "tune:atf_str", cmd->atf_str);
    // mpp_enc_cfg_set_s32(cfg, "tune:atr_str_i", p->atr_str_i);
    // mpp_enc_cfg_set_s32(cfg, "tune:atr_str_p", p->atr_str_p);
    // mpp_enc_cfg_set_s32(cfg, "tune:atl_str", p->atl_str);
    // mpp_enc_cfg_set_s32(cfg, "tune:deblur_en", cmd->deblur_en);
    // mpp_enc_cfg_set_s32(cfg, "tune:deblur_str", cmd->deblur_str);
    // mpp_enc_cfg_set_s32(cfg, "tune:sao_str_i", p->sao_str_i);
    // mpp_enc_cfg_set_s32(cfg, "tune:sao_str_p", p->sao_str_p);
    // mpp_enc_cfg_set_s32(cfg, "tune:lambda_idx_p", cmd->lambda_idx_p);
    // mpp_enc_cfg_set_s32(cfg, "tune:lambda_idx_i", cmd->lambda_idx_i);
    // mpp_enc_cfg_set_s32(cfg, "tune:rc_container", cmd->rc_container);
    mpp_enc_cfg_set_s32(cfg, "tune:scene_mode", scene_mode);
    // mpp_enc_cfg_set_s32(cfg, "tune:speed", cmd->speed);
    mpp_enc_cfg_set_s32(cfg, "tune:vmaf_opt", 0);

    /* setup hardware specified parameters */
    if (rc_mode == MPP_ENC_RC_MODE_SMTRC) {
        mpp_enc_cfg_set_st(cfg, "hw:aq_thrd_i", aq_thd_smart);
        mpp_enc_cfg_set_st(cfg, "hw:aq_thrd_p", aq_thd_smart);
        mpp_enc_cfg_set_st(cfg, "hw:aq_step_i", aq_step_smart);
        mpp_enc_cfg_set_st(cfg, "hw:aq_step_p", aq_step_smart);
    } else {
        mpp_enc_cfg_set_st(cfg, "hw:aq_thrd_i", aq_thd);
        mpp_enc_cfg_set_st(cfg, "hw:aq_thrd_p", aq_thd);
        mpp_enc_cfg_set_st(cfg, "hw:aq_step_i", aq_step_i_ipc);
        mpp_enc_cfg_set_st(cfg, "hw:aq_step_p", aq_step_p_ipc);
    }
    mpp_enc_cfg_set_st(cfg, "hw:aq_rnge_arr", aq_rnge_arr);

    mpp_enc_cfg_set_s32(cfg, "hw:qbias_en", 1);
    // mpp_enc_cfg_set_s32(cfg, "hw:qbias_i", cmd->bias_i);
    // mpp_enc_cfg_set_s32(cfg, "hw:qbias_p", cmd->bias_p);

    if (type == MPP_VIDEO_CodingAVC) {
        mpp_enc_cfg_set_st(cfg, "hw:qbias_arr", qbias_arr_avc);
    } else if (type == MPP_VIDEO_CodingHEVC) {
        mpp_enc_cfg_set_st(cfg, "hw:qbias_arr", qbias_arr_hevc);
    }

    mpp_enc_cfg_set_s32(cfg, "hw:skip_bias_en", 0);
    mpp_enc_cfg_set_s32(cfg, "hw:skip_bias", 4);
    mpp_enc_cfg_set_s32(cfg, "hw:skip_sad", 8);

    ret = mpi->control(ctx, MPP_ENC_SET_CFG, cfg);
    if (ret) {
        mpp_err("mpi control enc set cfg failed ret %d\n", ret);
        abort();
    }

    if (type == MPP_VIDEO_CodingAVC || type == MPP_VIDEO_CodingHEVC) {
        RcApiBrief rc_api_brief;
        rc_api_brief.type = type;
        rc_api_brief.name = (rc_mode == MPP_ENC_RC_MODE_SMTRC) ?
                            "smart" : "default";

        ret = mpi->control(ctx, MPP_ENC_SET_RC_API_CURRENT, &rc_api_brief);
        if (ret) {
            mpp_err("mpi control enc set rc api failed ret %d\n", ret);
            abort();
        }
    }

    if (ref)
        mpp_enc_ref_cfg_deinit(&ref);

    /* optional */
    {
        RK_U32 sei_mode;

        mpp_env_get_u32("sei_mode", &sei_mode, MPP_ENC_SEI_MODE_DISABLE);

        ret = mpi->control(ctx, MPP_ENC_SET_SEI_CFG, &sei_mode);
        if (ret) {
            mpp_err("mpi control enc set sei cfg failed ret %d\n", ret);
            abort();
        }
    }

    if (type == MPP_VIDEO_CodingAVC || type == MPP_VIDEO_CodingHEVC) {
        int header_mode = MPP_ENC_HEADER_MODE_EACH_IDR;
        ret = mpi->control(ctx, MPP_ENC_SET_HEADER_MODE, &header_mode);
        if (ret) {
            mpp_err("mpi control enc set header mode failed ret %d\n", ret);
            abort();
        }
    }

    // /* setup test mode by env */
    // mpp_env_get_u32("osd_enable", &p->osd_enable, 0);
    // mpp_env_get_u32("osd_mode", &p->osd_mode, MPP_ENC_OSD_PLT_TYPE_DEFAULT);
    // mpp_env_get_u32("roi_enable", &p->roi_enable, 0);
    // mpp_env_get_u32("user_data_enable", &p->user_data_enable, 0);

    // if (p->roi_enable) {
    //     mpp_enc_roi_init(&p->roi_ctx, width, height, type, 4);
    //     mpp_assert(p->roi_ctx);
    // }

    return ret;
}


encoder::encoder():cam("/dev/video0")
{

    MppBufferInfo info;

    mpp_set_log_level(MPP_LOG_DEBUG);
    
    memset(&info, 0, sizeof(MppBufferInfo));
    info.type = MPP_BUFFER_TYPE_EXT_DMA;
    info.fd = cam.dma_fd;
    info.size = cam.frame_length & 0x07ffffff;
    info.index = (cam.frame_length & 0xf8000000) >> 27;
    mpp_buffer_import(&mpp_buffer, &info);

    int ret;
    ret = mpp_buffer_group_get_internal(&buf_grp, MPP_BUFFER_TYPE_DRM | MPP_BUFFER_FLAGS_CACHABLE);
    if (ret)
    {
        mpp_err_f("failed to get mpp buffer group ret %d\n", ret);
    }

    // ret = mpp_buffer_get(buf_grp, &p->frm_buf, frame_size + p->header_size);
    // if (ret) {
    //     mpp_err_f("failed to get buffer for input frame ret %d\n", ret);
    //     goto MPP_TEST_OUT;
    // }

    ret = mpp_buffer_get(buf_grp, &pkt_buf, get_framesize(MPP_OUT_WIDTH,MPP_OUT_HEIGHT, MPP_FMT));
    if (ret)
    {
        mpp_err_f("failed to get buffer for output packet ret %d\n", ret);
    }

    ret = mpp_buffer_get(buf_grp, &md_info, get_mdinfo_size(MPP_OUT_WIDTH,MPP_OUT_HEIGHT, MPP_ENCODE_TYPE));
    if (ret)
    {
        mpp_err_f("failed to get buffer for motion info output packet ret %d\n", ret);
    }

    ret = mpp_create(&ctx, &mpi);
    if (ret) {
        mpp_err("mpp_create failed ret %d\n", ret);
    }

    MppPollType timeout = MPP_POLL_BLOCK;

    ret = mpi->control(ctx, MPP_SET_OUTPUT_TIMEOUT, &timeout);
    if (MPP_OK != ret) {
        mpp_err("mpi control set output timeout %d ret %d\n", timeout, ret);
    }

    ret = mpp_init(ctx, MPP_CTX_ENC, MPP_VIDEO_CodingHEVC);
    if (ret) {
        mpp_err("mpp_init failed ret %d\n", ret);
    }
    
    ret = mpp_enc_cfg_init(&cfg);
    if (ret) {
        mpp_err_f("mpp_enc_cfg_init failed ret %d\n", ret);
    }

    ret = mpi->control(ctx, MPP_ENC_GET_CFG, cfg);
    if (ret) {
        mpp_err_f("get enc cfg failed ret %d\n", ret);
    }

    ret = mpp_enc_cfg_setup(ctx,mpi,cfg);
    if (ret) {
        mpp_err_f("test mpp setup failed ret %d\n", ret);

    }

}

volatile sig_atomic_t running_flag = 1;

// 信号处理函数
void signal_handler(int signal) {
    printf("recv signal:%d\n",signal);
    if (signal == SIGINT) {
        running_flag = 0; // 设置退出标志
    }
}


void encoder::run()
{
    MppPacket packet = NULL;
    int ret = 0;

    signal(SIGINT, signal_handler);

    /*
        * Can use packet with normal malloc buffer as input not pkt_buf.
        * Please refer to vpu_api_legacy.cpp for normal buffer case.
        * Using pkt_buf buffer here is just for simplifing demo.
        */
    mpp_packet_init_with_buffer(&packet, pkt_buf);
    /* NOTE: It is important to clear output packet length!! */
    mpp_packet_set_length(packet, 0);

    ret = mpi->control(ctx, MPP_ENC_GET_HDR_SYNC, packet);
    if (ret) {
        mpp_err("mpi control enc get extra info failed\n");
        abort();
    } else {
        /* get and write sps/pps for H.264 */

        void *ptr   = mpp_packet_get_pos(packet);
        size_t len  = mpp_packet_get_length(packet);

        packet_handle_callback((uint8_t *)ptr,len);
        // if (p->fp_output)
        //     fwrite(ptr, 1, len, p->fp_output);
    }

    mpp_packet_deinit(&packet);

    // sse_unit_in_pixel = p->type == MPP_VIDEO_CodingAVC ? 16 : 8;
    // psnr_const = (16 + log2(MPP_ALIGN(p->width, sse_unit_in_pixel) *
    //                         MPP_ALIGN(p->height, sse_unit_in_pixel)));

    while (running_flag) {
        MppMeta meta = NULL;
        MppFrame frame = NULL;
        int cam_frm_idx = -1;
        bool end_of_frame = true;

        cam_frm_idx = cam.dequeue_frame();
        mpp_assert(cam_frm_idx >= 0);
        mpp_assert(mpp_buffer);

        ret = mpp_frame_init(&frame);
        if (ret) {
            mpp_err_f("mpp_frame_init failed\n");
            abort();
        }

        mpp_frame_set_width(frame, MPP_OUT_WIDTH);
        mpp_frame_set_height(frame, MPP_OUT_HEIGHT);
        mpp_frame_set_hor_stride(frame, MPP_OUT_WIDTH);
        mpp_frame_set_ver_stride(frame, MPP_OUT_HEIGHT);
        mpp_frame_set_fmt(frame, MPP_FMT);
        mpp_frame_set_eos(frame, 0);

        mpp_frame_set_buffer(frame, mpp_buffer);

        meta = mpp_frame_get_meta(frame);
        mpp_packet_init_with_buffer(&packet, pkt_buf);
        /* NOTE: It is important to clear output packet length!! */
        mpp_packet_set_length(packet, 0);
        mpp_meta_set_packet(meta, KEY_OUTPUT_PACKET, packet);
        mpp_meta_set_buffer(meta, KEY_MOTION_INFO, md_info);

        ret = mpi->encode_put_frame(ctx, frame);
        if (ret) {
            mpp_frame_deinit(&frame);
            abort();
        }

        mpp_frame_deinit(&frame);

        do {
            ret = mpi->encode_get_packet(ctx, &packet);
            if (ret) {
                abort();
            }

            mpp_assert(packet);

            void *ptr   = mpp_packet_get_pos(packet);
            size_t len  = mpp_packet_get_length(packet);

            pkt_eos = mpp_packet_get_eos(packet);

            packet_handle_callback((uint8_t *)ptr,len);
            // if (p->fp_output)
            //     fwrite(ptr, 1, len, p->fp_output);

            /* for low delay partition encoding */
            if (mpp_packet_is_partition(packet)) {
                end_of_frame = mpp_packet_is_eoi(packet);
            }

            mpp_packet_deinit(&packet);

        } while (!end_of_frame);

        cam.enqueue_frame();

    }

    printf("exit!\n");
}

encoder::~encoder(){

    mpp_destroy(ctx);

    mpp_enc_cfg_deinit(cfg);

    mpp_buffer_put(pkt_buf);

    mpp_buffer_put(md_info);

    mpp_buffer_group_put(buf_grp);
}
