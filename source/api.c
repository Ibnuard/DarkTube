#include "api.h"
#include "network.h"
#include "json_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char base_url[256] = "";

void api_set_base_url(const char *url) {
    strncpy(base_url, url, sizeof(base_url) - 1);
    base_url[sizeof(base_url) - 1] = '\0';
    
    // Remove trailing slash if present
    size_t len = strlen(base_url);
    if (len > 0 && base_url[len - 1] == '/') {
        base_url[len - 1] = '\0';
    }
}

int api_test_connection(void) {
    char *response = NULL;
    size_t response_size = 0;
    
    int res = http_get(base_url, &response, &response_size);
    if (res != 0) {
        return -1;
    }
    
    if (response) free(response);
    return 0;
}

int fetch_trending(VideoItem *items, int max_items) {
    char url[512];
    snprintf(url, sizeof(url), "%s/api/trending?maxResults=%d", base_url, max_items);
    
    char *response = NULL;
    size_t response_size = 0;
    
    int res = http_get(url, &response, &response_size);
    if (res != 0) {
        return -res;
    }
    
    if (!response) {
        return -999;
    }
    
    int count = parse_trending_json(response, items, max_items);
    
    free(response);
    return (count > 0) ? count : -2;
}

int fetch_video_stream_url(const char *videoId, char *stream_url, int max_url_len) {
    char url[512];
    snprintf(url, sizeof(url), "%s/api/stream?id=%s", base_url, videoId);
    
    char *response = NULL;
    size_t response_size = 0;
    
    int res = http_get(url, &response, &response_size);
    if (res != 0) {
        return -res;
    }
    
    if (!response) {
        return -999;
    }
    
    int parse_res = parse_video_url_json(response, stream_url, max_url_len);
    
    free(response);
    return parse_res;
}
