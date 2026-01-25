#ifndef BIBLES_H
#define BIBLES_H

#include <cjson/cJSON.h>
#include <curl/curl.h>
#include <stdbool.h>

#ifdef _cplusplus
extern "C" {
#endif

#define BIBLES_URL "https://api.scripture.api.bible/v1/bibles"
#define BIBLE_ABBR_BUFF_SIZE 24
#define LANGUAGE_ID_BUFF_SIZE 10
#define CATHOLIC_DESC "Catholic"

typedef struct BibleVersion {
  const char *id;
  const char *language_id;
  const char *name;
  const char *abbr;
  bool is_esv;
} BibleVersion;

#define ESV_VERSION                                                            \
  (BibleVersion) {                                                             \
    .id = NULL, .language_id = ENGLISH_LANGUAGE_ID, .name = ESV_NAME,          \
    .abbr = ESV_ABBR, .is_esv = true                                           \
  }

cJSON *get_bible_versions(CURL *curl, CURLcode *result_code);
// TODO: decide about implementing picking different descriptions (e.g.
// catholic, orthodox, ecumenical, etc)
BibleVersion bible_version_from_abbreviation(cJSON *bibles_arr,
                                             const char *language_id,
                                             char *abbr_input);
void bible_version_set_from_abbreviation(CURL *curl, CURLcode *result_code, cJSON *bibles_arr, cJSON **books_arr, const char *language_id, char *abbr, BibleVersion *version);

#ifdef _cplusplus
}
#endif
#endif
