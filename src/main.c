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

// TODO: consider adding topics or associated passages together (e.g. Genesis
// 50:20 and Romans 8:28) --------------

// TODO: add these notable passages to PASSAGES_FILE
// Exodus 20 + 33
// Numbers 6:24-26
// Deuteronomy 6:4-6, 8:17-18 --------------
// Judges 17:6, 21:25 (In Comparison with Deuteronomy 12:8)
// 1 Samuel 16:7 (read around it too)
// David's Repentance in 1 or 2 Samuel
// Job 1:21
// Job 28:23-28
// Psalms
//  16?
//  17
//  23
//  32
//  33
//  34
//  46
//  51
//  62
//  91
//  94
//  Moses Psalms
// Proverbs
//   3 (especially 5-8)
//   12:15
//   (18 or 19)?
//   26:4-5
//   26:12
//   30 (son reference)
//   31 (end)
// Ecclesiastes
//   1:1-2
//   3:20
// Isaiah
//  5:21 (Connection to Prov 3 & Prov 26:12)
//  40:6-8
//  53
// Jonah

/** Gospels **/
// Matthew
// NOTE: there is so much to note that I am going to leave it as such
// for now. This application was meant to help me write this down, but I find
// myself writing it down before writing it down. It would be better to get the
// ones that I want to remember written down, then read through the gospels and
// take notes as I go
//
//  3:1-3
//  3:8-10
//  3:16-17
//  4:1-11 (consider adding a way to exclude certain passages that are too large
//  from quizzes)
//    4:3-4 - First Temptation: Man shall not live by bread alone
//    4:5-7 - Second Temptation: Do not tempt the Lord your God
//    4:8-10 - Third Temptation: Worship is due to God alone
//  4:18-22 - Calling Simon, Andrew, James, and John
//  5:1-12 (already saved in file)
//  5:13
//  5:14-16
//  (Consider 5:17-20)
//  5:21-26 - About Anger
//  5:27-30 - Adultery + Graveness of Sin (Ties then into divorce)
//    5:31-32 - Divorce
//  5:33-37 - Lying
//  5:38-42 - Retaliation
//  5:43-48 - Love for Enemies
//  6:1-4 - Acting in Secret for God's sake and not for Recognition
//  6:6-7
//  6:7-15 - Our Father + Warning About Forgiveness
//    7-13 - Our Father
//    14-15 - Warning about Forgiveness
//  6:17-18 (Need to save or no? - Consider saving at a later time or adding
//  associated difficulty or depth levels to passages) 6:19-21 - Treasures in
//  Heaven 6:22-23 (To study) 6:24 - Serving Two Masters 6:25-34 - Worrying
//  7:1-5 - Hypocrisy
//  7:7-12 - Ask, Seek, Knock
//  7:13-14 - Narrow Gate
//  7:15-20 - False Prophets
//  7:21-23 - Self-Deception (Important)
//  7:24-27 - Doing and not simply hearing
//  8:5-13
//    8:7-8 - "I am not worthy that you should come under my roof"
//  8:16-17
//  8:19-22 (To study)
//  8:23-27 - Calming the storm
//  8:29-32 - Authority of Demons
//  9:1-8 (Link to Mark 2)
//  9:9 - Be ready like Matthew to abandon everything
//  9:10-13
//    9:12-13 especially
//  9:14-15 - We are still commanded to fast
//  9:24-26
//  9:36-38
//  18

// Mark
//  2
//  8
//  10

// Luke
//  18
//  23:34

// John
// To-add: I am statements
//  3:16
//  3:30
//  6 (eucharist)
//  8
//  14:27
//  16
//    statements
//    16:33

/** Acts & Epistles **/
// Romans
//  8
//    (beginning)
//    26
//    28
//    31
//    end
//  5 (on perseverance)
//  1 (on God's attributes)
//  7
//  4 (new Adam)
//  12:14-21
//  14
//    11
//    19
// 1 Corinthians 3 on Testing Through Fire and possible connections to purgatory
// seeing as purgation is told to be waiting 1 Corinthians 11 on the Eucharist
// Galatians 6:9-11
// James 2
// James 4
// 1 Peter 3
// 1 Peter 5

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

    puts("-------------------------------------------");
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
