#include "input.h"
#include "passage.h"
#include <stdio.h>
#include <string.h>

// Input Option Specifics
// Getting a Passage from the Bible
void get_passage_option_print_desc(void) {
  puts("passage/passage get - Retrieve a Passage from the Bible");
}

bool get_passage_option_fn(InputOption *current_opt, AppEnv env) {
  PassageInfo passage = {0};
  cJSON *passage_data = NULL;
  if (passage_info_get_from_input(&passage, env.curl, env.curl_code,
                                  env.bible_version, env.bibles_arr,
                                  env.books_arr)) {
    passage_data =
        passage_get_data(passage, env.curl, env.curl_code, *env.bible_version);
  }

  if (passage_data != NULL) {
    passage_print_text(passage_data, env.bible_version->abbr);
    // passage_save_input(passage, env.saved_passages_json);
    // puts("---------------------------------------------------");
    // passage_print_reference(passage, *env.books_arr, true);
    // TODO: figure out when passage_data should be deleted (I think it should
    // instead be put into current_opt->data, but I would need to be careful to
    // watch out for memory leaks)
    cJSON_Delete(passage_data);
    return true;
  }

  return false;
}

bool get_passage_option_input_check(char input_buff[static INPUT_BUFF_LEN]) {
  return (strcmp(input_buff, "passage") == 0) ||
         (strcmp(input_buff, "passage get") == 0);
}

static const InputOption GET_PASSAGE_OPTION = {
    .exec = get_passage_option_fn,
    .print_desc = get_passage_option_print_desc,
    .input_check = get_passage_option_input_check,
    .n_sub_options = 1,
    .sub_options = (const InputOption *[]){&GLOBAL_INPUT_OPTION},
};

// Testing
void test_option_print_desc(void) { puts("test - This is a test option"); }

bool test_option_fn() {
  puts("Test Option Executed!");
  return true;
}

bool test_option_input_check(char input_buff[static INPUT_BUFF_LEN]) {
  return (strcmp(input_buff, "test") == 0);
}

static const InputOption TEST_OPTION = {
    .exec = test_option_fn,
    .print_desc = test_option_print_desc,
    .input_check = test_option_input_check,
    .n_sub_options = 2,
    .sub_options =
        (const InputOption *[]){&GLOBAL_INPUT_OPTION, &GET_PASSAGE_OPTION},
};

// Global Option
void global_option_print_desc(void) {
  puts("root/global/home - Go back to application home");
}

bool global_option_fn() {
  puts("You're home!");
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
    .n_sub_options = 1,
    .sub_options = (const InputOption *[]){&TEST_OPTION},
};

void input_show_options_desc(void) {
  printf("info/help - See Available Options\n");
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
  return (strcmp(input_buff, "info") == 0) || (strcmp(input_buff, "help") == 0);
}

void input_process(InputOption *current_option,
                   char input_buff[static INPUT_BUFF_LEN], AppEnv env) {
  // Print Available Options if Requested
  if (input_info_req_check(input_buff)) {
    input_print_options_list(current_option->n_sub_options,
                             current_option->sub_options);
    return;
  }

  size_t i = 0;
  for (; i < current_option->n_sub_options; i++) {
    if (current_option->sub_options[i]->input_check(input_buff)) {
      break;
    }
  }

  if (i == current_option->n_sub_options) {
    printf("%s is not a valid option, enter 'info' or 'help' to see available "
           "options\n",
           input_buff);
    return;
  }

  const InputOption *selected_option = current_option->sub_options[i];
  if (selected_option->exec(current_option, env)) {
    input_switch_option(current_option, selected_option);
  }
}
