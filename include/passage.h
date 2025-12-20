#ifndef PASSAGE_H
#define PASSAGE_H

#include "bibles.h"
#include "books.h"
#include <cjson/cJSON.h>
#include <curl/curl.h>

#ifdef _cplusplus
extern "C" {
#endif

#define MAX_PASSAGE_ID_LEN 32
#define PASSAGE_INPUT_BUFF_SIZE 84 // NOTE: >= BIBLE_MAX_BOOK_NAME_LEN + 30
#define MAX_ESV_PASSAGE_QUIERY_LEN 100
#define PASSAGE_MESSAGE_BUFF_SIZE 1024
#define PASSAGE_CONTEXT_BUFF_SIZE 512
#define INVALID_ESV_QUERY ".0.0"

typedef char PassageId[MAX_PASSAGE_ID_LEN];
typedef enum PassageObjField {
  PassageObjId = 0,
  PassageObjMessage = 1,
  PassageObjContext = 2,
} PassageObjField;
// NOTE: the enum numbers must correspond to their index in the array
static const char *PASSAGE_OBJ_FIELD_KEYS[3] = {"id", "message", "context"};

typedef char ESVPassageQuiery[MAX_ESV_PASSAGE_QUIERY_LEN];
typedef struct PassageInfo {
  char book_id[MAX_BOOK_ID_LEN];
  int beg_chap;
  int beg_verse;
  int end_chap;
  int end_verse;
} PassageInfo;

// Passage Retrieval
void passage_get_id(PassageInfo passage, PassageId passage_id);
void passage_get_info_from_id(PassageId passage_id, PassageInfo *passage_info);
// NOTE: returns whether input was valid
bool passage_get_book_name_from_passage_string(
    const char passage_str[PASSAGE_INPUT_BUFF_SIZE],
    char book_name[MAX_BOOK_NAME_LEN], size_t *last_token_start_i);
// NOTE: returns whether input was valid
bool passage_info_get_from_string(
    const char passage_str[PASSAGE_INPUT_BUFF_SIZE], PassageInfo *passage,
    CURL *curl, CURLcode *result_code, BibleVersion *version, cJSON *bibles_arr,
    cJSON **books_arr);
// NOTE: returns whether input was valid
bool passage_info_get_from_input(char *message, PassageInfo *passage,
                                 CURL *curl, CURLcode *result_code,
                                 BibleVersion *version, cJSON *bibles_arr,
                                 cJSON **books_arr);
cJSON *esv_passage_get_data(PassageInfo passage, CURL *curl,
                            CURLcode *result_code, const char *bible_id);
cJSON *passage_get_data(PassageInfo passage, CURL *curl, CURLcode *result_code,
                        BibleVersion bible_version);

// Saving Passages
cJSON *passages_get_json(void);
// NOTE: returns whether successfully saved
bool passage_save_input(PassageId passage_id, cJSON *passages_json);
// NOTE: returns whether successfully saved

// Passage Interactions
void passage_print_reference(PassageInfo passage, cJSON *books_arr,
                             bool newline);
void passage_print_text(cJSON *passage_data, const char *bible,
                        bool print_reference);
bool passage_id_matches_key(PassageId passage_id, PassageInfo key);

// Saved Passages
cJSON *passages_array_get(cJSON *passages_json);
cJSON *passages_get_by_id(cJSON *passages_json, PassageId req_id);
int passages_get_passage_ind(cJSON *passages_json, cJSON *cmp_passage);
cJSON *passages_get_random_entry(cJSON *passages_json);
bool passages_passage_matches_key(cJSON *saved_passage, PassageInfo key);

// JSON Passage Obj Manipulations
cJSON *passage_obj_create(PassageId id, char *message, char *context);
cJSON *passage_obj_get_field(cJSON *passage_obj, PassageObjField attr);

#ifdef _cplusplus
}
#endif
#endif
