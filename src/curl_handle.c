#include "curl_handle.h"
#include "constants.h"
#include <curl/curl.h>
#include <global.h>
#include <stdlib.h>
#include <string.h>

void curl_response_init(CurlResponse *res) {
  res->size = 0;
  res->string = malloc(1);
  error_if(res->string == NULL, "malloc failed");
}

void curl_get_at_url(CURL *curl, CURLcode *result_code, CurlResponse *output,
                     const char *url, const char *auth_header) {
  // Creating Curl Headers
  struct curl_slist *headers = NULL;
  // char auth_header[256];
  // snprintf(auth_header, sizeof(auth_header), "api-key: %s", BIBLE_API_KEY);
  headers = curl_slist_append(
      headers, (auth_header == NULL) ? DEFAULT_AUTH_HEADER : auth_header);

  // Setting Curl Options
  printf("Performing a GET request at %s\n", url);
  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_chunk);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)output);

  // Performing Request
  *result_code = curl_easy_perform(curl);
  error_if(*result_code != CURLE_OK, curl_easy_strerror(*result_code));
  curl_slist_free_all(headers);
}

size_t curl_write_chunk(void *data, size_t size, size_t nmemb, void *userdata) {
  size_t real_size = size * nmemb;
  CurlResponse *response = (CurlResponse *)userdata;

  // Reallocating response string as necessary
  char *ptr = realloc(response->string,
                      response->size + real_size + 1); // +1 for \0 terminator
  if (ptr == NULL) {
    return CURL_WRITEFUNC_ERROR;
  }
  response->string = ptr;

  // Copying over data
  memcpy(&(response->string[response->size]), data, real_size);
  response->size += real_size;
  response->string[response->size] = '\0';

  return real_size;
}
