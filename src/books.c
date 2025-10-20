#include "books.h"
#include "constants.h"
#include "curl_handle.h"
#include "global.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BOOKS_URL_BUFF_LEN 200
#define SCANF_BUFF_LEN 100

cJSON *books_parse_json(char *data) {
  cJSON *root = cJSON_Parse(data);
  error_if(root == NULL, "error parsing JSON");

  cJSON *books_arr = cJSON_GetObjectItemCaseSensitive(root, "data");
  error_if(!cJSON_IsArray(books_arr) || books_arr == NULL,
           "no data field in books.json");

  // Duplicate so deleting root won't delete the returned pointer
  cJSON *books_arr_copy = cJSON_Duplicate(books_arr, 1); // Deep copy
  cJSON_Delete(root);

  return books_arr_copy;
}

cJSON *books_get_from_bible_version(CURL *curl, CURLcode *result_code,
                                    const char *bible_id) {
  error_if(strlen(bible_id) == 0, "invalid bible_id");

  // Verify if information already saved
  FILE *file = fopen(BOOKS_FILE, "r");
  if (file != NULL) {
    char scan_buff[SCANF_BUFF_LEN];
    size_t n_read =
        fread(scan_buff, sizeof(scan_buff[0]), SCANF_BUFF_LEN - 1, file);
    scan_buff[n_read] = '\0';

    char *substr_pos = strstr(scan_buff, bible_id);
    if (substr_pos != NULL) {
      puts("Reading books data from " BOOKS_FILE);

      // Reading Informaiton from File
      fseek(file, 0, SEEK_END);
      long file_len = ftell(file);
      rewind(file);
      char *books_txt = malloc((file_len + 1) * sizeof(char));
      fread(books_txt, sizeof(char), file_len, file);
      books_txt[file_len] = '\0';

      // Parsing Information
      cJSON *res = books_parse_json(books_txt);
      free(books_txt);

      fclose(file);
      return res;
    }

    fclose(file);
  }

  // Get Books Information from API
  CurlResponse response;
  curl_response_init(&response);
  char url[BOOKS_URL_BUFF_LEN];
  snprintf(url, BOOKS_URL_BUFF_LEN * sizeof(char),
           "https://api.scripture.api.bible/v1/bibles/%s/books", bible_id);

  curl_get_at_url(curl, result_code, &response, url, NULL);

  // Write Books to info/books.json
  file = fopen(BOOKS_FILE, "w");
  error_if(file == NULL, "failed to open " BOOKS_FILE " in write in w mode");
  fprintf(file, "%s", response.string);

  // Parsing Json
  cJSON *res = books_parse_json(response.string);

  // Dealing with dynamic memory
  free(response.string);
  fclose(file);

  return res;
}

// NOTE: pointer returned belongs to books_arr
const char *book_get_name(cJSON *books_arr, char *input) {
  cJSON *item = NULL;
  cJSON_ArrayForEach(item, books_arr) {
    cJSON *name = cJSON_GetObjectItemCaseSensitive(item, "name");
    // cJSON *name = cJSON_GetObjectItemCaseSensitive(item, "name");
    cJSON *id = cJSON_GetObjectItemCaseSensitive(item, "id");
    error_if(!cJSON_IsString(name) || !cJSON_IsString(id),
             "books_arr is not in proper format");
    error_if(name->valuestring == NULL || id->valuestring == NULL,
             "missing string in books_arr");
    if (strcmp(id->valuestring, input) == 0) {
      return name->valuestring;
    }
  }

  return NULL;
}

// NOTE: pointer returned belongs to books_arr
const char *book_get_id(cJSON *books_arr, char *input) {
  /*
[
{"id":"GEN", "bibleId":"9879...", "abbreviation":"Genesis", "name":"Genesis",
"nameLong":"The First Book of Moses, Commonly Called Genesis"},
]
*/
  cJSON *item = NULL;
  cJSON_ArrayForEach(item, books_arr) {
    cJSON *name = cJSON_GetObjectItemCaseSensitive(item, "name");
    // cJSON *name = cJSON_GetObjectItemCaseSensitive(item, "name");
    cJSON *id = cJSON_GetObjectItemCaseSensitive(item, "id");
    error_if(!cJSON_IsString(name) || !cJSON_IsString(id),
             "books_arr is not in proper format");
    error_if(name->valuestring == NULL || id->valuestring == NULL,
             "missing string in books_arr");
    if (strcmp(name->valuestring, input) == 0) {
      return id->valuestring;
    }
  }

  return NULL;
}
