#ifndef API_H
#define API_H

#include <stdbool.h>

typedef struct {
    char title[256];
    char videoId[32];
    char author[128];
    int lengthSeconds;
    char durationText[16];
    char thumbUrl[256];
    void *thumb_tex;
} VideoItem;

// Set the base URL for the API server (e.g., "http://192.168.1.10:3000")
void api_set_base_url(const char *url);

// Test connection to the API server
int api_test_connection(void);

// Fetches the trending videos from the custom API.
// Overwrites the item array up to max_items.
// Returns the number of items fetched, or < 0 on error.
int fetch_trending(VideoItem *items, int max_items);

// Search videos by query
int fetch_search(const char *query, VideoItem *items, int max_items);

// Fetches video info and retrieves direct MP4 stream URL.
// The URL should be a pre-allocated buffer of reasonable size (e.g. 4096).
// Returns 0 on success, < 0 on error.
int fetch_video_stream_url(const char *videoId, char *stream_url, int max_url_len);

#endif // API_H
