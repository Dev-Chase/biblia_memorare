#include "input.h"
#include <stdio.h>
#include <string.h>

// Input Option Definitions
void test_option_print_desc(void) { puts("test - This is a test option"); }

void test_option_fn(InputOption *_) { puts("Test Option Executed!"); }

bool test_option_input_check(char input_buff[static INPUT_BUFF_LEN]) {
  return (strcmp(input_buff, "test") == 0);
}

static const InputOption TEST_OPTION = {
    .exec = test_option_fn,
    .print_desc = test_option_print_desc,
    .input_check = test_option_input_check,
    .n_sub_options = 1,
    .sub_options = (const InputOption *[]){&GLOBAL_INPUT_OPTION},
};

// Global Option
void global_option_print_desc(void) {
  puts("root/global/home - Go back to application home");
}

void global_option_fn(InputOption *_) { puts("You're home!"); }

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

// TODO: add it so info option is always available
// Directions
void input_print_options_list(
    size_t n_sub_options,
    const InputOption *input_options[static n_sub_options]) {
  for (size_t i = 0; i < n_sub_options; i++) {
    input_options[i]->print_desc();
  }
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

void input_process(InputOption *current_option,
                   char input_buff[static INPUT_BUFF_LEN]) {
  size_t i = 0;
  for (; i < current_option->n_sub_options; i++) {
    if (current_option->sub_options[i]->input_check(input_buff)) {
      break;
    }
  }

  if (i == current_option->n_sub_options) {
    printf("%s is not a valid option, type info to see your options\n",
           input_buff);
    return;
  }

  const InputOption *selected_option = current_option->sub_options[i];
  selected_option->exec(current_option);
  input_switch_option(current_option, selected_option);
}
