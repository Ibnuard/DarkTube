#include <stdint.h>

extern "C" {

typedef struct AVRational {
    int num;
    int den;
} AVRational;

char *av_ts_make_time_string2(char *buf, int64_t ts, AVRational tb) {
    if (buf) {
        buf[0] = '\0';
    }
    return buf;
}

}
