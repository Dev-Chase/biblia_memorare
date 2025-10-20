#ifndef CURL_HANDLE_H
#define CURL_HANDLE_H

#include <curl/curl.h>
#include <stddef.h>

typedef struct CurlResponse {
  char *string;
  size_t size;
} CurlResponse;

void curl_response_init(CurlResponse *res);

void curl_get_at_url(CURL *curl, CURLcode *result_code, CurlResponse *output,
                     const char *url, const char *auth_header);
size_t curl_write_chunk(void *data, size_t size, size_t nmemb, void *userdata);

#endif
