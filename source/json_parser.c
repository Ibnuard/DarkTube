#include "json_parser.h"
#include <cJSON.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

// Helper to parse ISO 8601 duration (e.g., PT1M30S)
static int parse_iso8601_duration(const char *duration) {
    if (!duration || duration[0] != 'P' || duration[1] != 'T') return 0;
    
    int total_seconds = 0;
    int current_val = 0;
    const char *p = duration + 2;
    
    while (*p) {
        if (isdigit((unsigned char)*p)) {
            current_val = current_val * 10 + (*p - '0');
        } else {
            if (*p == 'H') total_seconds += current_val * 3600;
            else if (*p == 'M') total_seconds += current_val * 60;
            else if (*p == 'S') total_seconds += current_val;
            current_val = 0;
        }
        p++;
    }
    return total_seconds;
}

int parse_trending_json(const char *json_string, VideoItem *items, int max_items) {
    if (!json_string || !items || max_items <= 0) return 0;

    cJSON *root = cJSON_Parse(json_string);
    if (!root) {
        return 0;
    }

    // Custom API wrapper returns { "videos": [...] }
    cJSON *videos = cJSON_GetObjectItemCaseSensitive(root, "videos");
    if (!cJSON_IsArray(videos)) {
        cJSON_Delete(root);
        return 0;
    }

    int count = 0;
    cJSON *item = NULL;
    cJSON_ArrayForEach(item, videos) {
        if (count >= max_items) break;

        cJSON *title = cJSON_GetObjectItemCaseSensitive(item, "title");
        cJSON *videoId = cJSON_GetObjectItemCaseSensitive(item, "id");
        cJSON *channelTitle = cJSON_GetObjectItemCaseSensitive(item, "channelTitle");
        
        // Try to find duration in various formats
        int length = 0;
        cJSON *duration = cJSON_GetObjectItemCaseSensitive(item, "duration");
        if (cJSON_IsNumber(duration)) {
            length = (int)duration->valuedouble;
        } else if (cJSON_IsString(duration)) {
            length = parse_iso8601_duration(duration->valuestring);
        } else {
            cJSON *contentDetails = cJSON_GetObjectItemCaseSensitive(item, "contentDetails");
            if (cJSON_IsObject(contentDetails)) {
                cJSON *cd_duration = cJSON_GetObjectItemCaseSensitive(contentDetails, "duration");
                if (cJSON_IsString(cd_duration)) {
                    length = parse_iso8601_duration(cd_duration->valuestring);
                }
            }
        }
        
        if (cJSON_IsString(title) && cJSON_IsString(videoId)) {
            strncpy(items[count].title, title->valuestring, sizeof(items[count].title) - 1);
            items[count].title[sizeof(items[count].title) - 1] = '\0';

            strncpy(items[count].videoId, videoId->valuestring, sizeof(items[count].videoId) - 1);
            items[count].videoId[sizeof(items[count].videoId) - 1] = '\0';

            if (cJSON_IsString(channelTitle)) {
                strncpy(items[count].author, channelTitle->valuestring, sizeof(items[count].author) - 1);
                items[count].author[sizeof(items[count].author) - 1] = '\0';
            } else {
                items[count].author[0] = '\0';
            }

            items[count].lengthSeconds = length;
            
            // Format durationText from length
            if (length > 0) {
                int h = length / 3600;
                int m = (length % 3600) / 60;
                int s = length % 60;
                if (h > 0) snprintf(items[count].durationText, sizeof(items[count].durationText), "%d:%02d:%02d", h, m, s);
                else snprintf(items[count].durationText, sizeof(items[count].durationText), "%d:%02d", m, s);
            } else {
                strncpy(items[count].durationText, "LIVE", sizeof(items[count].durationText) - 1);
                items[count].durationText[sizeof(items[count].durationText)-1] = '\0';
            }

            // Extract thumbnail
            items[count].thumbUrl[0] = '\0';
            cJSON *thumbnails = cJSON_GetObjectItemCaseSensitive(item, "thumbnails");
            if (cJSON_IsObject(thumbnails)) {
                cJSON *thumb = cJSON_GetObjectItemCaseSensitive(thumbnails, "high");
                if (!cJSON_IsObject(thumb)) thumb = cJSON_GetObjectItemCaseSensitive(thumbnails, "medium");
                if (!cJSON_IsObject(thumb)) thumb = cJSON_GetObjectItemCaseSensitive(thumbnails, "default");
                if (cJSON_IsObject(thumb)) {
                    cJSON *url_node = cJSON_GetObjectItemCaseSensitive(thumb, "url");
                    if (cJSON_IsString(url_node)) {
                        strncpy(items[count].thumbUrl, url_node->valuestring, sizeof(items[count].thumbUrl) - 1);
                        items[count].thumbUrl[sizeof(items[count].thumbUrl) - 1] = '\0';
                    }
                }
            }

            count++;
        }
    }

    cJSON_Delete(root);
    return count;
}

int parse_video_url_json(const char *json_string, char *stream_url, int max_url_len) {
    if (!json_string || !stream_url || max_url_len <= 0) return -1;

    cJSON *root = cJSON_Parse(json_string);
    if (!root) {
        return -1;
    }

    // Try to get URL from the "formats" array first (new API format)
    cJSON *formats = cJSON_GetObjectItemCaseSensitive(root, "formats");
    if (cJSON_IsArray(formats)) {
        cJSON *first = cJSON_GetArrayItem(formats, 0);
        if (cJSON_IsObject(first)) {
            cJSON *f_url = cJSON_GetObjectItemCaseSensitive(first, "url");
            if (cJSON_IsString(f_url)) {
                strncpy(stream_url, f_url->valuestring, max_url_len - 1);
                stream_url[max_url_len - 1] = '\0';
                cJSON_Delete(root);
                return 0;
            }
        }
    }

    // Fallback: top-level "url"
    cJSON *url = cJSON_GetObjectItemCaseSensitive(root, "url");
    if (cJSON_IsString(url)) {
        strncpy(stream_url, url->valuestring, max_url_len - 1);
        stream_url[max_url_len - 1] = '\0';
        cJSON_Delete(root);
        return 0;
    }

    cJSON_Delete(root);
    return -1;
}
