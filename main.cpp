#include "streamer_encoder_wrapper.h"
#include <unistd.h>

int main(int argc, char *argv[]) {
    int port = 5600;
    int opt;
    while ((opt = getopt(argc, argv, "p:")) != -1) {
        switch (opt) {
            case 'p':
                port = strtol(optarg,NULL,0);
                break;
            default:
                printf("unknown arguments.\n");
                exit(EXIT_FAILURE);
        }
    }

    char *target_ip = argv[optind];

    printf("target ip:%s port:%d\n",target_ip,port);
    streamer_encoder_wrapper enc(target_ip, port);
    enc.run();
    return 0;
}