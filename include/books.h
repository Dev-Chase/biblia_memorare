#ifndef API_H
#define API_H

#include <cjson/cJSON.h>
#include <curl/curl.h>
#include <stdbool.h>

#define MAX_BOOK_NAME_LEN 64
#define MAX_BOOK_ID_LEN 12
// #define MAX_BOOK_NAME_LEN_MINUS_ONE 63
// #define BOOK_NAME_INPUT_MAX #MAX_BOOK_NAME_LEN_MINUS_ONE

// TODO: review that char * returned is never/can never be corrupted by a
// cjson_delete
cJSON *books_get_from_bible_version(CURL *curl, CURLcode *result_code,
                                    const char *bible_id, bool save_to_file);
const char *book_get_name(cJSON *books_arr, char *input);
const char *book_get_id(cJSON *books_arr, char *input);

#endif
