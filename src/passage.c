#include "passage.h"
#include "bibles.h"
#include "books.h"
#include "constants.h"
#include "curl_handle.h"
#include "global.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

// Passage Retrieval
void passage_get_id(PassageInfo passage, PassageId passage_id) {
  if (passage.end_verse != 0 && passage.end_chap != 0 &&
      (passage.beg_chap != passage.end_chap ||
       passage.end_verse != passage.beg_verse)) {
    snprintf(passage_id, MAX_PASSAGE_ID_LEN * sizeof(char), "%s.%d.%d-%s.%d.%d",
             passage.book_id, passage.beg_chap, passage.beg_verse,
             passage.book_id, passage.end_chap, passage.end_verse);
  } else {
    snprintf(passage_id, MAX_PASSAGE_ID_LEN * sizeof(char), "%s.%d.%d",
             passage.book_id, passage.beg_chap, passage.beg_verse);
  }
}

// NOTE: passage_id must be valid
void passage_get_info_from_id(PassageId passage_id, PassageInfo *passage_info) {
  sscanf(passage_id, "%[^.].%d.%d-%*s.%d.%d", passage_info->book_id,
         &passage_info->beg_chap, &passage_info->beg_verse,
         &passage_info->end_chap, &passage_info->end_verse);
}

void passage_url(PassageInfo passage, const char *bible_id,
                 char url[URL_BUFF_LEN]) {
  PassageId passage_id;
  passage_get_id(passage, passage_id);
  snprintf(url, URL_BUFF_LEN * sizeof(char),
           "https://api.scripture.api.bible/v1/bibles/%s/passages/"
           "%s?content-type=text&include-notes=false&include-"
           "titles=true&include-chapter-numbers=false&include-verse-numbers="
           "true&include-verse-spans=false&use-org-id=false",
           bible_id, passage_id);
}

void esv_passage_url(PassageInfo passage, char url[URL_BUFF_LEN]) {
  PassageId passage_id;
  passage_get_id(passage, passage_id);
  snprintf(url, URL_BUFF_LEN * sizeof(char),
           "https://api.esv.org/v3/passage/text/?q="
           "%s&include-passage-references=false&include-footnotes=false",
           passage_id);
  // "&include-heading-horizontal-lines=true"
}

#define PASSAGE_INPUT_BUFF_LEN 64 // NOTE: >= BIBLE_MAX_BOOK_NAME_LEN + 10
bool passage_info_get_from_input(PassageInfo *passage, CURL *curl,
                                 CURLcode *result_code, BibleVersion *version,
                                 cJSON *bibles_arr, cJSON **books_arr) {
  *passage = (PassageInfo){0};

  char passage_input[PASSAGE_INPUT_BUFF_LEN] = "";
  char passage_input_tokenized[PASSAGE_INPUT_BUFF_LEN] = "";
  char book_name[MAX_BOOK_NAME_LEN] = "";

  // Getting Input
  printf("Please Input a Passage: ");
  fgets(passage_input, sizeof(passage_input), stdin);
  strncpy(passage_input_tokenized, passage_input, PASSAGE_INPUT_BUFF_LEN - 1);
  fflush(stdout);

  // Tokenizing Beginning of Inputted String to get Book Name
  char *token = strtok(passage_input_tokenized, " ");
  if (token == NULL || token[0] == '\n') {
    fprintf(stderr, "Passage Inputted was empty\n");
    *passage = (PassageInfo){0};
    return false;
  }
  strncat(book_name, token, sizeof(book_name) - 1);
  while ((token = strtok(NULL, " ")) != NULL && !isdigit(token[0])) {
    strncat(book_name, " ",
            sizeof(book_name) - strnlen(book_name, MAX_BOOK_NAME_LEN - 1) - 1);

    // Capitalize Book Name if length is above 2 characters (to account for Song
    // of Solomon)
    if (strnlen(token, MAX_BOOK_NAME_LEN - 1) > 2) {
      token[0] = toupper(token[0]);
    }
    strncat(book_name, token,
            sizeof(book_name) - strnlen(book_name, MAX_BOOK_NAME_LEN - 1) - 1);
  }

  // Offsetting the inputted string so that the book name is no longer included
  if (token == NULL) {
    fprintf(stderr, "No chapter or verse inputted after the book name\n");
    *passage = (PassageInfo){0};
    return false;
  }

  size_t numbers_start = (token - passage_input_tokenized);
  memmove(passage_input, (char *)(passage_input + numbers_start),
          strnlen((char *)(passage_input + numbers_start),
                  PASSAGE_INPUT_BUFF_LEN - 1));

  // Getting Passage Information
  sscanf(passage_input, "%d:%d-%d:%d", &passage->beg_chap, &passage->beg_verse,
         &passage->end_chap, &passage->end_verse);
  // printf("book_name prior to terminating: %s\n", book_name);
  unsigned long book_name_len = strlen(book_name);
  if (book_name[book_name_len - 1] == ' ') {
    book_name[book_name_len - 1] = '\0';
  }
  // printf("book_name: %s\n", book_name);

  // Swapping Variables to Correspond with the Values Inputted
  if (passage->end_chap != 0 && passage->end_verse == 0) {
    passage->end_verse = passage->end_chap;
    passage->end_chap = passage->beg_chap;
  }

  // Getting Bible Version Abbreviation, if inputted
  char bible_version_abbr[24];
  char language_id[10];
  int n_captured = sscanf(passage_input, "%*[^(](%23[^)]) - %9s",
                          bible_version_abbr, language_id);
  if (n_captured == 2) {
    printf("Language_id: %s\n", language_id);
  }
  if (n_captured > 0) {
    BibleVersion version_supplied = bible_version_from_abbreviation(
        bibles_arr, (n_captured == 2) ? language_id : version->language_id,
        bible_version_abbr);

    if (version_supplied.id != NULL) {
      *version = version_supplied;
      cJSON_Delete(*books_arr);
      *books_arr =
          books_get_from_bible_version(curl, result_code, version_supplied.id);
      error_if(*books_arr == NULL, "failed to parse json");
    }
  }

  // Getting Book Abbreviation from Name
  const char *book_id = book_get_id(*books_arr, book_name);
  if (book_id == NULL) {
    fprintf(stderr, "Book not found in bible version list\n");
    *passage = (PassageInfo){0};
    return false;
  }

  // Copying over string so that it no longer belongs to books_arr
  strncpy(passage->book_id, book_id, sizeof(char) * (MAX_BOOK_ID_LEN - 1));

  printf("%s %d:%d-%d:%d (%s)\n", book_name, passage->beg_chap,
         passage->beg_verse, passage->end_chap, passage->end_verse,
         version->abbr);
  return true;
}

// NOTE: caller is responsible for cleaning up the json returned and responsible
// for verifying if the passage is in the ESV API format
cJSON *passage_get_data(PassageInfo passage, CURL *curl, CURLcode *result_code,
                        BibleVersion bible_version) {
  char url[URL_BUFF_LEN];
  if (bible_version.is_esv) {
    esv_passage_url(passage, url);
  } else {
    passage_url(passage, bible_version.id, url);
  }
  if (strnlen(url, 2) == 0) {
    puts("Passage is not valid for selected bible version");
    return NULL;
  }

  CurlResponse response;
  curl_response_init(&response);

  // Performing GET Request
  curl_get_at_url(curl, result_code, &response, url,
                  (bible_version.is_esv) ? ESV_AUTH_HEADER : NULL);
  // puts("----------------------");
  // printf("string retrieved: %s\n", response.string);
  // puts("----------------------");

  // Parsing Response
  cJSON *root = cJSON_Parse(response.string);
  error_if(root == NULL, "failed to parse json");
  cJSON *data;

  // Verifying Response
  bool invalid_response = false;
  if (bible_version.is_esv) {
    cJSON *query = cJSON_GetObjectItemCaseSensitive(root, "query");
    error_if(query == NULL || !cJSON_IsString(query),
             "ESV JSON response did not contain a query");

    if (strcmp(INVALID_ESV_QUERY, query->valuestring) == 0) {
      invalid_response = true;
    }
  } else {
    cJSON *status_code = cJSON_GetObjectItemCaseSensitive(root, "statusCode");
    if (status_code != NULL) {
      invalid_response = true;
    }
  }

  if (invalid_response) {
    cJSON_Delete(root);
    fprintf(stderr, "-------------------------\n");
    fprintf(stderr, "JSON response for passage retrieval at %s was invalid\n",
            url);
    fprintf(stderr, "-------------------------\n");
    return NULL;
  }

  // Extracting Core Information
  if (bible_version.is_esv) {
    data = root;
  } else {
    data = cJSON_GetObjectItemCaseSensitive(root, "data");
    if (data == NULL) {
      printf("Passage %s %d:%d-%d:%d was invalid and did not contain data for "
             "the selected version: \n\t%s\n",
             passage.book_id, passage.beg_chap, passage.beg_verse,
             passage.end_chap, passage.end_verse, response.string);
      free(response.string);
      cJSON_Delete(root);
      return NULL;
    }
  }
  cJSON_AddBoolToObject(data, ESV_JSON_KEY, bible_version.is_esv);

  cJSON *res;
  if (root != data) {
    // Duplicate so deleting root won't delete the returned pointer
    res = cJSON_Duplicate(data, 1); // Deep copy
    cJSON_Delete(root);
  } else {
    res = data;
  }

  free(response.string);
  return res;
}

// Saving Passages
cJSON *passages_get_json(void) {
  cJSON *root;
  FILE *file = fopen(PASSAGES_FILE, "r");
  if (file == NULL) {
    puts(PASSAGES_FILE " doesn't exist yet, creating an empty one");

    // Create new object
    root = cJSON_CreateObject();
    cJSON *passages_arr = cJSON_CreateArray();
    cJSON_AddItemToObject(root, "passages", passages_arr);

    // Serialize JSON into text
    char *json_txt = cJSON_Print(root);
    error_if(json_txt == NULL,
             "failed to serialize json when creating new " PASSAGES_FILE
             " file");

    // Write JSON Text to File
    file = fopen(PASSAGES_FILE, "w");
    error_if(file == NULL, "failed to create " PASSAGES_FILE);
    fprintf(file, "%s", json_txt);
    free(json_txt);
  } else {
    puts("Parsing JSON from " PASSAGES_FILE);

    // Read from existing file
    fseek(file, 0, SEEK_END);
    long file_len = ftell(file);
    rewind(file);
    char *json_txt = malloc((file_len + 1) * sizeof(char));
    fread(json_txt, sizeof(char), file_len, file);
    json_txt[file_len] = '\0';

    // Parse json
    root = cJSON_Parse(json_txt);
    error_if(root == NULL, "failed to parse passages json");

    free(json_txt);
  }
  fclose(file);

  return root;
}

void passage_save(PassageId passage_id, char *message, char *context,
                  cJSON *passages_json) {
  if (passages_get_by_id(passages_json, passage_id) != NULL) {
    printf("Passage %s is already saved in " PASSAGES_FILE "\n", passage_id);
    return;
  }

  printf("Saving %s to " PASSAGES_FILE "\n", passage_id);

  cJSON *passages_arr =
      cJSON_GetObjectItemCaseSensitive(passages_json, "passages");
  error_if(passages_arr == NULL || !cJSON_IsArray(passages_arr),
           "passages json did not contain a \"passages\" array");

  // Creating Passage Object and Adding it to Array
  cJSON *passage_obj = passage_obj_create(passage_id, message, context);
  cJSON_AddItemToArray(passages_arr, passage_obj);

  // Serialize json As text
  char *json_txt = cJSON_Print(passages_json);
  error_if(json_txt == NULL, "failed to parse json after adding new passage");

  // Write json Text to PASSAGES_FILE
  FILE *file = fopen(PASSAGES_FILE, "w");
  error_if(file == NULL, "failed to open " PASSAGES_FILE " up for writing");
  fprintf(file, "%s", json_txt);

  // Clean Up Memory
  free(json_txt);
  fclose(file);
}

bool passage_save_input(PassageId passage_id, cJSON *passages_json) {
  if (passages_get_by_id(passages_json, passage_id) != NULL) {
    printf("Passage %s is already saved in " PASSAGES_FILE "\n", passage_id);
    return false;
  }

  char message_buff[PASSAGE_MESSAGE_BUFF_SIZE];
  char context_buff[PASSAGE_CONTEXT_BUFF_SIZE];

  // Getting Meaning
  printf("What message would you like to save for this passage?: ");
  scanf("\n");
  fgets(message_buff, PASSAGE_MESSAGE_BUFF_SIZE, stdin);
  // TODO: figure out what happens when an exceedingly large input is given and there is no \n character
  message_buff[strcspn(message_buff, "\n")] = '\0'; // Remove '\n'
  fflush(stdout);

  // Getting Context
  printf("What is the context of the passage?: ");
  fgets(context_buff, PASSAGE_CONTEXT_BUFF_SIZE, stdin);
  context_buff[strcspn(context_buff, "\n")] = '\0'; // Remove '\n'
  fflush(stdout);

  // Saving the Passage
  passage_save(passage_id, message_buff, context_buff, passages_json);
  return true;
}

bool passage_get_save(PassageId out, CURL *curl, CURLcode *result_code,
                      BibleVersion *bible_version, cJSON *bibles_arr,
                      cJSON **books_arr, cJSON *passages_json) {
  PassageInfo passage = {0};
  bool res = false;
  if (passage_info_get_from_input(&passage, curl, result_code, bible_version,
                                  bibles_arr, books_arr)) {
    passage_get_id(passage, out);
    res = passage_save_input(out, passages_json);
  }
  return res;
}

// Passage Interactions
void passage_print_reference(PassageInfo passage, cJSON *books_arr,
                             bool newline) {
  const char *book_name = book_get_name(books_arr, passage.book_id);
  if (passage.end_verse != 0 && passage.end_chap != passage.beg_chap) {
    printf("%s %d:%d-%d:%d", book_name, passage.beg_chap, passage.beg_verse,
           passage.end_chap, passage.end_verse);
  } else if (passage.end_verse != 0 && passage.beg_chap == passage.end_chap) {
    printf("%s %d:%d-%d", book_name, passage.beg_chap, passage.beg_verse,
           passage.end_verse);
  } else if (passage.end_verse == 0) {
    printf("%s %d:%d", book_name, passage.beg_chap, passage.beg_verse);
  }

  if (newline) {
    printf("\n");
  }
}

void passage_print_text(cJSON *passage_data, const char *bible) {
  puts("-------------------------------------------");
  cJSON *is_esv = cJSON_GetObjectItemCaseSensitive(passage_data, ESV_JSON_KEY);
  error_if(is_esv == NULL, ESV_JSON_KEY " key was not set in passage_data");
  error_if(!cJSON_IsBool(is_esv), ESV_JSON_KEY " was not a boolean");

  cJSON *reference =
      (cJSON_IsTrue(is_esv))
          ? cJSON_GetObjectItemCaseSensitive(passage_data, "query")
          : cJSON_GetObjectItemCaseSensitive(passage_data, "reference");
  if (cJSON_IsString(reference) && (reference->valuestring != NULL)) {
    if (bible != NULL) {
      printf("%s (%s)\n\n", reference->valuestring, bible);
    } else {
      printf("%s\n\n", reference->valuestring);
    }
  }

  if (cJSON_IsTrue(is_esv)) {
    cJSON *passages =
        cJSON_GetObjectItemCaseSensitive(passage_data, "passages");
    cJSON *passage = NULL;
    cJSON_ArrayForEach(passage, passages) {
      if (cJSON_IsString(passage) && passage->valuestring != NULL) {
        printf("%s", passage->valuestring);
      }
    }
    printf("\n");
  } else {
    cJSON *content = cJSON_GetObjectItemCaseSensitive(passage_data, "content");
    if (cJSON_IsString(content) && content->valuestring != NULL) {
      printf("%s", content->valuestring);
    }
  }
  puts("-------------------------------------------");
}

// Saved Passages
cJSON *passages_array_get(cJSON *passages_json) {
  cJSON *passages_arr =
      cJSON_GetObjectItemCaseSensitive(passages_json, "passages");
  error_if(passages_arr == NULL || !cJSON_IsArray(passages_arr),
           "passages json did not contain a valid \"passages\" array");

  return passages_arr;
}

// NOTE: pointer returned belongs to passages_json and lasts for its lifetime
cJSON *passages_get_by_id(cJSON *passages_json, PassageId req_id) {
  cJSON *passages_arr = passages_array_get(passages_json);

  cJSON *passage_obj = NULL;
  cJSON_ArrayForEach(passage_obj, passages_arr) {
    cJSON *id = passage_obj_get_field(passage_obj, PassageObjId);
    if (!strcmp(id->valuestring, req_id)) {
      return passage_obj;
    }
  }

  printf("Passage with id: %s is not already saved in " PASSAGES_FILE "\n",
         req_id);
  return NULL;
}

// NOTE: pointer returned belongs to passages_json and lasts for its lifetime
cJSON *passages_get_random_entry(cJSON *passages_json) {
  cJSON *passages_arr = passages_array_get(passages_json);

  int size = cJSON_GetArraySize(passages_arr);
  if (size == 0) {
    puts("\"passages\" array in " PASSAGES_FILE " was empty");
    return NULL;
  }

  size_t ind = (size_t)(rand() % size);
  cJSON *res = cJSON_GetArrayItem(passages_arr, ind);
  error_if(res == NULL,
           "random item in \"passages\" array in " PASSAGES_FILE " was NULL");
  error_if(!cJSON_IsObject(res),
           "random item in \"passages\" array was not an object");

  return res;
}

// JSON Passage Obj Manipulations
cJSON *passage_obj_create(PassageId id, char *message, char *context) {
  cJSON *passage_obj = cJSON_CreateObject();

  cJSON *cjson_passage_id = cJSON_CreateString(id);
  cJSON_AddItemToObject(passage_obj, PASSAGE_OBJ_FIELD_KEYS[PassageObjId],
                        cjson_passage_id);
  cJSON *cjson_passage_message = cJSON_CreateString(message);
  cJSON_AddItemToObject(passage_obj, PASSAGE_OBJ_FIELD_KEYS[PassageObjMessage],
                        cjson_passage_message);
  cJSON *cjson_passage_context = cJSON_CreateString(context);
  cJSON_AddItemToObject(passage_obj, PASSAGE_OBJ_FIELD_KEYS[PassageObjContext],
                        cjson_passage_context);

  return passage_obj;
}

// NOTE: pointer returned belongs to passage_obj and lasts for its lifetime
cJSON *passage_obj_get_field(cJSON *passage_obj, PassageObjField attr) {
  cJSON *res = cJSON_GetObjectItemCaseSensitive(passage_obj,
                                                PASSAGE_OBJ_FIELD_KEYS[attr]);
  error_if(res == NULL,
           "passage object given did not contain the requested field");
  error_if(!cJSON_IsString(res), "passage object field was not a string");
  return res;
}
