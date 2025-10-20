#ifndef APP_H
#define APP_H

#include "bibles.h"
#include <cjson/cJSON.h>
#include <curl/curl.h>

#ifdef _cplusplus
extern "C" {
#endif

typedef struct AppEnv {
  CURL *curl;
  CURLcode *curl_code;
  cJSON *bibles_arr;
  cJSON **books_arr;
  BibleVersion *bible_version;
  cJSON *saved_passages_json;
} AppEnv;

#ifdef _cplusplus
}
#endif
#endif
