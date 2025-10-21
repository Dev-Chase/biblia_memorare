#include "input.h"
#include "passage.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const InputOption GLOBAL_INPUT_OPTION;
static const InputOption GET_PASSAGE_OPTION;
static const InputOption SAVE_PASSAGE_OPTION;

// Input Option Specifics
// Getting a Passage from the Bible
void get_passage_option_print_desc(void) {
  puts("get/passage get - Get a Passage from the Bible");
}

// TODO: figure out why a passage with a range of verses is not being retrieved, but only the first verse when going here after saving
bool get_passage_option_fn(InputOption *current_opt, AppEnv env) {
  PassageInfo passage = {0};
  cJSON *passage_data = NULL;

  if (current_opt->data.type == SavedPassageId) {
    passage_get_info_from_id(current_opt->data.value.passage_id, &passage);
  } else {
    if (!passage_info_get_from_input(&passage, env.curl, env.curl_code,
                                     env.bible_version, env.bibles_arr,
                                     env.books_arr)) {
      return false;
    }
  }

  passage_data =
      passage_get_data(passage, env.curl, env.curl_code, *env.bible_version);
  if (passage_data != NULL) {
    // Printing Passage Text
    passage_print_text(passage_data, env.bible_version->abbr);

    // Saving PassageID to current_opt
    current_opt->data.type = RetrievedPassageId;
    passage_get_id(passage, current_opt->data.value.passage_id);

    cJSON_Delete(passage_data);
    return true;
  }

  current_opt->data.type = NoData;
  return false;
}

bool get_passage_option_input_check(char input_buff[static INPUT_BUFF_LEN]) {
  return (strcmp(input_buff, "get") == 0) ||
         (strcmp(input_buff, "passage get") == 0);
}

static const InputOption GET_PASSAGE_OPTION = {
    .exec = get_passage_option_fn,
    .print_desc = get_passage_option_print_desc,
    .input_check = get_passage_option_input_check,
    .n_sub_options = 3,
    .sub_options =
        (const InputOption *[]){&GLOBAL_INPUT_OPTION, &GET_PASSAGE_OPTION,
                                &SAVE_PASSAGE_OPTION},
    .data = {0}};

// Saving a Passage ID
void save_passage_option_print_desc(void) {
  puts("save/passage save - Save a Passage");
}

bool save_passage_option_fn(InputOption *current_opt, AppEnv env) {
  if (current_opt->data.type == RetrievedPassageId) {
    if (passage_save_input(current_opt->data.value.passage_id,
                           env.saved_passages_json)) {
      current_opt->data.type = SavedPassageId;
    }
    return false;
  }

  if (passage_get_save(current_opt->data.value.passage_id, env.curl,
                       env.curl_code, env.bible_version, env.bibles_arr,
                       env.books_arr, env.saved_passages_json)) {
    current_opt->data.type = SavedPassageId;
  } else {
    current_opt->data.type = NoData;
  }

  return true;
}

bool save_passage_option_input_check(char input_buff[static INPUT_BUFF_LEN]) {
  return (strcmp(input_buff, "save") == 0) ||
         (strcmp(input_buff, "passage save") == 0);
}

static const InputOption SAVE_PASSAGE_OPTION = {
    .exec = save_passage_option_fn,
    .print_desc = save_passage_option_print_desc,
    .input_check = save_passage_option_input_check,
    .n_sub_options = 2,
    .sub_options =
        (const InputOption *[]){&GLOBAL_INPUT_OPTION, &GET_PASSAGE_OPTION},
    .data = {0}};

// Global Option
void global_option_print_desc(void) {
  puts("root/global/home - Go back to application home");
}

bool global_option_fn(InputOption *current_opt, AppEnv _) {
  current_opt->data.type = NoData;
  return true;
}

bool global_option_input_check(char input_buff[static INPUT_BUFF_LEN]) {
  return (strcmp(input_buff, "root") == 0) ||
         (strcmp(input_buff, "global") == 0) ||
         (strcmp(input_buff, "home") == 0);
}

const InputOption GLOBAL_INPUT_OPTION = {
    .exec = global_option_fn,
    .print_desc = global_option_print_desc,
    .input_check = global_option_input_check,
    .n_sub_options = 2,
    .sub_options =
        (const InputOption *[]){&GET_PASSAGE_OPTION, &SAVE_PASSAGE_OPTION},
    .data = {0}};

void input_show_options_desc(void) {
  printf("info/help/list - List Available Options\n");
}

// Directions
void input_print_options_list(
    size_t n_sub_options,
    const InputOption *input_options[static n_sub_options]) {
  puts("---------------------");
  puts("Available Options:");
  input_show_options_desc();
  for (size_t i = 0; i < n_sub_options; i++) {
    input_options[i]->print_desc();
  }
  printf("exit - Exit Program\n");
  puts("---------------------");
}

// Getting and Processing Input
void input_get(const char *message, char input_buff[static INPUT_BUFF_LEN]) {
  printf("%s", message);
  fflush(stdout);

  fgets(input_buff, INPUT_BUFF_LEN, stdin);
  size_t input_buff_strlen = strnlen(input_buff, INPUT_BUFF_LEN);
  if (input_buff[input_buff_strlen - 1] == '\n') {
    input_buff[input_buff_strlen - 1] = '\0';
  }
}

// Handling Input
// NOTE: only run after new_opt has been executed
void input_switch_option(InputOption *current_opt, const InputOption *new_opt) {
  InputOptionData copied_data = current_opt->data;
  *current_opt = *new_opt;
  current_opt->data = copied_data;
}

bool input_info_req_check(char input_buff[static INPUT_BUFF_LEN]) {
  return (strcmp(input_buff, "info") == 0) ||
         (strcmp(input_buff, "help") == 0) || (strcmp(input_buff, "list") == 0);
}

bool exit_req_check(char input_buff[static INPUT_BUFF_LEN]) {
  return (strcmp(input_buff, "exit") == 0);
}

void input_process(InputOption *current_option,
                   char input_buff[static INPUT_BUFF_LEN], AppEnv env) {
  // Print Available Options if Requested
  if (input_info_req_check(input_buff)) {
    input_print_options_list(current_option->n_sub_options,
                             current_option->sub_options);
    return;
  }

  if (exit_req_check(input_buff)) {
    puts("Exiting program");
    exit(EXIT_SUCCESS);
    return;
  }

  size_t i = 0;
  for (; i < current_option->n_sub_options; i++) {
    if (current_option->sub_options[i]->input_check(input_buff)) {
      break;
    }
  }

  if (i == current_option->n_sub_options) {
    printf("%s is not a valid option, enter 'info' or 'help' or 'list' to see "
           "available "
           "options\n",
           input_buff);
    return;
  }

  const InputOption *selected_option = current_option->sub_options[i];
  if (selected_option->exec(current_option, env)) {
    input_switch_option(current_option, selected_option);
  }
}
