#ifndef JSON_PARSER_H
#define JSON_PARSER_H

#include "api.h"

// Parse the raw JSON array from /api/v1/trending
// Populates the items array up to max_items.
// Returns the number of items successfully parsed.
int parse_trending_json(const char *json_string, VideoItem *items, int max_items);

// Parse the raw JSON object from /api/v1/videos/<id>
// Extracts the highest bitrate mp4 direct stream URL.
// The URL is copied into stream_url, truncated if longer than max_url_len - 1.
// Returns 0 on success, < 0 on error.
int parse_video_url_json(const char *json_string, char *stream_url, int max_url_len);

#endif // JSON_PARSER_H
