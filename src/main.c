#include "bibles.h"
#include "books.h"
#include "constants.h"
#include "input.h"
#include "passage.h"
#include <cjson/cJSON.h>
#include <curl/curl.h>
#include <global.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Old Testing
// const char *url = "https://bible-api.com/john%203:16-20?translation=kjv";
// const char *url =
//     "https://api.scripture.api.bible/v1/bibles/" WEB_BIBLE_ID "/books";
// const char *url = "https://api.scripture.api.bible/v1/bibles/"
//                   "de4e12af7f28f599-02/verses/JHN.3.16";
// const char *url = "https://api.scripture.api.bible/v1/bibles";

int main(void) {
  srand(time(NULL));
  puts("C Web Testing - GET Requests from API's");

  // Initializing Curl
  CURL *curl;
  CURLcode curl_res;
  curl = curl_easy_init();
  error_if(curl == NULL, "curl failed to initalize");

  // Retrieving Default Information
  cJSON *bibles_arr = get_bible_versions(curl, &curl_res);
  BibleVersion bible_version = bible_version_from_abbreviation(
      bibles_arr, DEFAULT_LANGUAGE_ID, DEFAULT_BIBLE_ABBR);
  error_if(bible_version.id == NULL, "default bible ( " DEFAULT_BIBLE_ABBR
                                     " ) is not present in " BIBLES_FILE);
  cJSON *books_arr =
      books_get_from_bible_version(curl, &curl_res, bible_version.id);
  cJSON *passages_json = passages_get_json();

  // Creating Application Environment
  AppEnv app_env = {
      .curl = curl,
      .curl_code = &curl_res,
      .bibles_arr = bibles_arr,
      .books_arr = &books_arr,
      .bible_version = &bible_version,
      .saved_passages_json = passages_json,
  };

  // Input Options:
  //  1. Show Input Options
  //  2. Retrieve a Passage from the Bible
  //    a) Save passage
  //    g) Get Saved Passage Object
  //  3. Search for a saved passage
  //    Options if one is found:
  //    a) show specified meaning
  //    b) show specified context
  //    b) show passage text (Redirect to option for retrieving a passage)
  //  4. Retrieve a Random Passage from the Saved Passages
  //    (options same as in previous)
  //  5. Set a Bible Translation
  //  6. Get a List of the Books in the Current Version/Translation
  //    Get a list of the books in a specified version?
  //      e.g. optional parameter that, if empty, uses current version
  //  7.

  InputOption current_option = GLOBAL_INPUT_OPTION;
  current_option.exec(&current_option, app_env);
  current_option.data.input_buff[0] = '\0';

  input_print_options_list(current_option.n_sub_options,
                           current_option.sub_options);
  while (true) {
    // Get Input
    input_get("Input Here: ", INPUT_BUFF_LEN, current_option.data.input_buff);

    // Process Input
    input_process(&current_option, app_env);

    puts("-----------------------------------");
  }

  // Cleaning Up Curl
  cJSON_Delete(books_arr);
  cJSON_Delete(bibles_arr);
  cJSON_Delete(passages_json);
  curl_easy_cleanup(curl);
  return EXIT_SUCCESS;
}

void error_if(bool condition, const char *str) {
  if (condition) {
    fprintf(stderr, "Error: %s\n", str);
    exit(EXIT_FAILURE);
  }
}
