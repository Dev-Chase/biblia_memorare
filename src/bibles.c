#include "bibles.h"
#include "constants.h"
#include "curl_handle.h"
#include "global.h"
#include <stdlib.h>
#include <string.h>

cJSON *get_bible_versions(CURL *curl, CURLcode *result_code) {
  CurlResponse response;

  // Verify if information is already saved
  FILE *file = fopen(BIBLES_FILE, "r");
  if (file == NULL) {
    puts("Fetching Data, since " BIBLES_FILE " doesn't exist yet");
    file = fopen(BIBLES_FILE, "w");
    error_if(file == NULL, "failed to create " BIBLES_FILE);

    curl_response_init(&response);
    curl_get_at_url(curl, result_code, &response, BIBLES_URL, NULL);
    fprintf(file, "%s", response.string);
  } else {
    puts("Reading data from " BIBLES_FILE);

    // Reading Information from File into response.string
    fseek(file, 0, SEEK_END);
    long file_len = ftell(file);
    rewind(file);
    response.string = malloc((file_len + 1) * sizeof(char));
    fread(response.string, sizeof(char), file_len, file);
    response.string[file_len] = '\0';
  }

  // Parsing Information
  cJSON *root = cJSON_Parse(response.string);
  error_if(root == NULL, "error parsing JSON");
  cJSON *bibles_arr = cJSON_GetObjectItemCaseSensitive(root, "data");
  error_if(!cJSON_IsArray(bibles_arr) || bibles_arr == NULL,
           "no data field in bibles.json");

  // Duplicate so deleting root won't delete the returned pointer
  cJSON *bibles_arr_copy = cJSON_Duplicate(bibles_arr, 1); // Deep copy
  cJSON_Delete(root);

  fclose(file);
  free(response.string);
  return bibles_arr_copy;
}

// NOTE: caller is responsible for retrieving related information on the version
// pointers in returned struct belong to bibles_arr unless it is the ESV
BibleVersion bible_version_from_abbreviation(cJSON *bibles_arr,
                                             const char *language_id,
                                             char *input) {
  // Check if ESV
  if (strcmp(input, ESV_ABBR) == 0) {
    return (BibleVersion){
        .id = WEB_BIBLE_ID, // NOTE: used for retrieving books
        .language_id = ENGLISH_LANGUAGE_ID,
        .abbr = ESV_ABBR,
        .name = ESV_NAME,
        .is_esv = true,
    };
  }

  // All Other Versions
  BibleVersion res = {0};
  cJSON *item = NULL;
  cJSON_ArrayForEach(item, bibles_arr) {
    cJSON *id = cJSON_GetObjectItemCaseSensitive(item, "id");
    cJSON *language = cJSON_GetObjectItemCaseSensitive(item, "language");
    cJSON *json_language_id = cJSON_GetObjectItemCaseSensitive(language, "id");
    cJSON *name = cJSON_GetObjectItemCaseSensitive(item, "nameLocal");
    cJSON *abbr = cJSON_GetObjectItemCaseSensitive(item, "abbreviation");
    cJSON *abbr_local =
        cJSON_GetObjectItemCaseSensitive(item, "abbreviationLocal");
    error_if(!cJSON_IsString(name) || !cJSON_IsString(abbr) ||
                 !cJSON_IsString(abbr_local) ||
                 !cJSON_IsString(json_language_id) || !cJSON_IsString(id),
             "bibles_arr is not in proper format");
    error_if(name->valuestring == NULL || abbr->valuestring == NULL ||
                 abbr_local->valuestring == NULL ||
                 json_language_id->valuestring == NULL ||
                 id->valuestring == NULL,
             "invalid strings in bibles_arr");
    if (strcmp(abbr->valuestring, input) == 0 ||
        strcmp(abbr_local->valuestring, input) == 0) {
      res = (BibleVersion){
          .id = id->valuestring,
          .language_id = json_language_id->valuestring,
          .name = name->valuestring,
          .abbr = abbr_local->valuestring,
          .is_esv = false,
      };

      if (strcmp(json_language_id->valuestring, language_id) == 0) {
        break;
      }
    }
  }

  if (res.id == NULL) {
    printf("Couldn't find %s (in %s or otherwise) in bibles\n", input,
           language_id);
  } /* else {
    printf(
        "Bible version is the %s (%s), which has an ID of %s and is in %s\n",
        name->valuestring, abbr->valuestring, id->valuestring,
        json_language_id->valuestring);
  } */

  return res;
}
