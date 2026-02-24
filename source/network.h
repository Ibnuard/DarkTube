#ifndef NETWORK_H
#define NETWORK_H

#include <stddef.h>

// Initialize the networking subsystem (sockets, curl)
int init_network(void);

// Perform an HTTP(S) GET request to the specified URL.
// Response data is returned in a dynamically allocated buffer (*response).
// *response_size will contain the number of bytes written.
// Returns 0 on success (CURLE_OK), or a negative CURLcode on error.
// The caller is responsible for freeing *response!
int http_get(const char *url, char **response, size_t *response_size);

// Clean up networking resources
void cleanup_network(void);

#endif // NETWORK_H
