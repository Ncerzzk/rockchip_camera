#include "streamer_encoder_wrapper.h"

int main() {
    streamer_encoder_wrapper enc((char *)"127.0.0.1", 5600);
    enc.run();
    return 0;
}