#include "network.h"
#include <switch.h>
#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct MemoryStruct {
    char *memory;
    size_t size;
};

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if(ptr == NULL) {
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

int init_network(void) {
    Result ret = socketInitializeDefault();
    if (R_FAILED(ret)) {
        return -1;
    }
    
    curl_global_init(CURL_GLOBAL_DEFAULT);
    return 0;
}

int http_get(const char *url, char **response, size_t *response_size) {
    CURL *curl_handle;
    CURLcode res;
    
    struct MemoryStruct chunk;
    chunk.memory = malloc(1);
    chunk.size = 0;

    if (!chunk.memory) {
        return -1;
    }
    
    curl_handle = curl_easy_init();
    if(curl_handle) {
        curl_easy_setopt(curl_handle, CURLOPT_URL, url);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
        
        curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "DarkTube/1.0 (Nintendo Switch)");
        curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl_handle, CURLOPT_MAXFILESIZE_LARGE, (curl_off_t)1000000);
        curl_easy_setopt(curl_handle, CURLOPT_CONNECTTIMEOUT, 10L); 
        curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 30L);
        curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 0L);
        
        res = curl_easy_perform(curl_handle);
        
        if(res == CURLE_OK) {
            long response_code = 0;
            curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &response_code);
            
            if (response_code >= 400) {
                curl_easy_cleanup(curl_handle);
                free(chunk.memory);
                return -(int)response_code; 
            }

            if (chunk.size > 1000000) {
                curl_easy_cleanup(curl_handle);
                free(chunk.memory);
                return -1;
            }
        }

        if(res != CURLE_OK) {
            curl_easy_cleanup(curl_handle);
            free(chunk.memory);
            return -(int)res; 
        }
        
        *response = chunk.memory;
        *response_size = chunk.size;
        
        curl_easy_cleanup(curl_handle);
        return 0;
    }
    
    free(chunk.memory);
    return -1;
}

void cleanup_network(void) {
    curl_global_cleanup();
    socketExit();
}
