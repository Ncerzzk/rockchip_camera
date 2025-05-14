#include "streamer_encoder.h"
#include <arpa/inet.h>
#include "smolrtsp.h"

typedef struct
{
    uint8_t expect_nalu_uint_type;
    int udp_fd;
    SmolRTSP_NalTransport *transport;
    SmolRTSP_NalStartCodeTester start_code_tester;
    uint32_t timestamp;
    encoder_type enc_type;
} streamer_encoder;

static streamer_encoder enc;

void streamer_encoder_init(encoder_type t, char *target_ip, int port)
{
    int payload_ty = -1;
    if (t == H264)
    {
        enc.expect_nalu_uint_type = SMOLRTSP_H264_NAL_UNIT_AUD;
        enc.enc_type = H264;
        payload_ty = 96;
    }
    else
    {
        enc.expect_nalu_uint_type = SMOLRTSP_H265_NAL_UNIT_AUD_NUT;
        enc.enc_type = H265;
        payload_ty = 97;
    }

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
    };

    addr.sin_addr.s_addr = inet_addr(target_ip);

    enc.udp_fd = smolrtsp_dgram_socket(addr.sin_family, smolrtsp_sockaddr_ip((struct sockaddr *)&addr), port);
    enc.transport = SmolRTSP_NalTransport_new(
        SmolRTSP_RtpTransport_new(
            smolrtsp_transport_udp(enc.udp_fd), payload_ty, 900000));

    enc.start_code_tester = NULL;
    enc.timestamp = 0;
}

void stream_encoder_handle_packet(uint8_t *ptr, size_t len)
{

    SmolRTSP_NalHeader header;

    U8Slice99 video = U8Slice99_new(ptr, len);

    if (enc.start_code_tester == NULL)
    {
        enc.start_code_tester = smolrtsp_determine_start_code(video);
        if (enc.start_code_tester == NULL)
        {
            abort();
        }
    }

    int start_code_len = enc.start_code_tester(video);
    if (start_code_len == 0)
    {
        printf("could not find start code.\n");
    }
    video = U8Slice99_advance(video, start_code_len);

    uint8_t *nalu_start = video.ptr;
    int nalu_header_len = 0;
    if (enc.enc_type == H264)
    {
        header = SmolRTSP_NalHeader_H264(SmolRTSP_H264NalHeader_parse(nalu_start[0]));
        nalu_header_len = 1;
    }
    else
    {
        header = SmolRTSP_NalHeader_H265(SmolRTSP_H265NalHeader_parse(nalu_start));
        nalu_header_len = 2;
    }
    const SmolRTSP_NalUnit nalu = {
        .header = header,
        .payload = U8Slice99_new(nalu_start + nalu_header_len, len - nalu_header_len - start_code_len),
    };

    enc.timestamp += 900000 / 30;

    if (SmolRTSP_NalTransport_send_packet(
            enc.transport, SmolRTSP_RtpTimestamp_Raw(enc.timestamp), nalu) ==
        -1)
    {
        perror("Failed to send RTP/NAL");
    }
}