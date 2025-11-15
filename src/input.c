#include "input.h"
#include "constants.h"
#include "global.h"
#include "passage.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const InputOption GLOBAL_INPUT_OPTION;
static const InputOption GET_PASSAGE_OPTION;
static const InputOption SAVE_PASSAGE_OPTION;
static const InputOption GET_SAVED_PASSAGE_OPTION;
static const InputOption RANDOM_SAVED_PASSAGE_OPTION;
static const InputOption SAVED_PASSAGE_INFO_OPTION;
static const InputOption EDIT_SAVED_PASSAGE_OPTION;

// Input Option Specifics
// Getting a Passage from the Bible
void get_passage_option_print_desc(void) {
  puts("get/passage get - Get a Passage from the Bible");
}

bool get_passage_option_fn(InputOption *current_opt, AppEnv env) {
  PassageInfo passage = {0};
  cJSON *passage_data = NULL;
  bool replace_current_opt = true;

  if (current_opt->data.type == SavedPassage) {
    passage_get_info_from_id(current_opt->data.value.passage_id, &passage);
    replace_current_opt = false;
  } else {
    if (!passage_info_get_from_input(
            "What passage are you searching for?: ", &passage, env.curl,
            env.curl_code, env.bible_version, env.bibles_arr, env.books_arr)) {
      return false;
    }
  }

  passage_data =
      passage_get_data(passage, env.curl, env.curl_code, *env.bible_version);
  if (passage_data != NULL) {
    // Printing Passage Text
    passage_print_text(passage_data, env.bible_version->abbr);

    // Saving PassageID to current_opt if going to Switch Option
    if (replace_current_opt) {
      current_opt->data.type = RetrievedPassageId;
      passage_get_id(passage, current_opt->data.value.passage_id);
    }

    cJSON_Delete(passage_data);
    return replace_current_opt;
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
    .n_sub_options = 4,
    .sub_options =
        (const InputOption *[]){&GLOBAL_INPUT_OPTION, &GET_PASSAGE_OPTION,
                                &SAVE_PASSAGE_OPTION,
                                &GET_SAVED_PASSAGE_OPTION},
    .data = {0}};

// Saving a Passage ID
void save_passage_option_print_desc(void) {
  puts("save/passage save - Save a Passage");
}

bool save_passage_option_fn(InputOption *current_opt, AppEnv env) {
  // Set current_opt->data.value.passage_id if not just gotten through
  // retrieving a passage
  if (current_opt->data.type != RetrievedPassageId) {
    puts("Getting a Passage Id to save since none was provided.");
    PassageInfo passage;
    if (!passage_info_get_from_input(
            "Which passage do you want to save?: ", &passage, env.curl,
            env.curl_code, env.bible_version, env.bibles_arr, env.books_arr)) {
      return false;
    }

    passage_get_id(passage, current_opt->data.value.passage_id);
  }

  passage_save_input(current_opt->data.value.passage_id,
                     env.saved_passages_json);
  // NOTE: no error handling is done because the only error that should be
  // present here is a passage already being saved, in which case a
  // SavedPassage can still be set (might have to change if passage_save_input
  // can error in any other meaningful way)
  current_opt->data.type = SavedPassage;
  // NOTE: pointer lasts for lifetime of env.saved_passages_json
  current_opt->data.value.saved_passage_obj = passages_get_by_id(
      env.saved_passages_json, current_opt->data.value.passage_id);
  // Only NULL if it was not saved or is not already saved, which should never
  // happen
  error_if(current_opt->data.value.saved_passage_obj == NULL,
           "error saving passage: could not be retrieved after having "
           "been saved");

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
    .n_sub_options = 4,
    .sub_options =
        (const InputOption *[]){&GLOBAL_INPUT_OPTION, &GET_PASSAGE_OPTION,
                                &SAVED_PASSAGE_INFO_OPTION,
                                &EDIT_SAVED_PASSAGE_OPTION},
    .data = {0}};

// Getting a Saved Passage
void get_saved_passage_option_print_desc(void) {
  puts("saved/get saved - Get a saved passage");
}

bool get_saved_passage_option_fn(InputOption *current_opt, AppEnv env) {
  PassageInfo passage;
  PassageId passage_id;
  if (current_opt->data.type == RetrievedPassageId) {
    // NOTE: no need for bounds checking since both are of type PassageId
    // (char[] of the same length) and should both be null-terminated
    strcpy(passage_id, current_opt->data.value.passage_id);
    passage_get_info_from_id(passage_id, &passage);
  } else {
    if (!passage_info_get_from_input(
            "What saved passage are you looking for?: ", &passage, env.curl,
            env.curl_code, env.bible_version, env.bibles_arr, env.books_arr)) {
      return false;
    }

    passage_get_id(passage, passage_id);
  }

  cJSON *passage_obj = passages_get_by_id(env.saved_passages_json, passage_id);
  if (passage_obj == NULL) {
    passage_print_reference(passage, *env.books_arr, false);
    printf(" is not saved in " PASSAGES_FILE "\n");
    return false;
  }

  printf("Found ");
  passage_print_reference(passage, *env.books_arr, false);
  printf(" in " PASSAGES_FILE "!\n");
  current_opt->data.type = SavedPassage;
  current_opt->data.value.saved_passage_obj = passage_obj;
  // NOTE: no need for bounds checking since both are of type PassageId (char[]
  // of the same length) and should be null-terminated
  strcpy(current_opt->data.value.passage_id, passage_id);

  return true;
}

bool get_saved_passage_option_input_check(
    char input_buff[static INPUT_BUFF_LEN]) {
  return (strcmp(input_buff, "saved") == 0) ||
         (strcmp(input_buff, "get saved") == 0);
}

static const InputOption GET_SAVED_PASSAGE_OPTION = {
    .exec = get_saved_passage_option_fn,
    .print_desc = get_saved_passage_option_print_desc,
    .input_check = get_saved_passage_option_input_check,
    .n_sub_options = 5,
    .sub_options =
        (const InputOption *[]){&GLOBAL_INPUT_OPTION, &GET_PASSAGE_OPTION,
                                &GET_SAVED_PASSAGE_OPTION,
                                &SAVED_PASSAGE_INFO_OPTION,
                                &EDIT_SAVED_PASSAGE_OPTION},
    .data = {0}};

// Getting a Random Saved Passage
void random_saved_passage_option_print_desc(void) {
  puts("random/get random - Get a random saved passage");
}

bool random_saved_passage_option_fn(InputOption *current_opt, AppEnv env) {
  cJSON *passage_obj = passages_get_random_entry(env.saved_passages_json);
  error_if(passage_obj == NULL, "Failed to get a random entry!");

  printf("Successfully Retrieved a Random Passage\n");
  current_opt->data.type = SavedPassage;
  current_opt->data.value.saved_passage_obj = passage_obj;

  // Getting Passage ID
  cJSON *passage_obj_id = passage_obj_get_field(passage_obj, PassageObjId);

  // NOTE: no need for bounds checking since both are of type PassageId (char[]
  // of the same length) and should be null-terminated
  strncpy(current_opt->data.value.passage_id, passage_obj_id->valuestring,
          MAX_PASSAGE_ID_LEN - 1);
  // NOTE: bounds checking is done to prevent overflows, but it does not
  // guarantee that the saved passage id is valid

  return true;
}

bool random_saved_passage_option_input_check(
    char input_buff[static INPUT_BUFF_LEN]) {
  return (strcmp(input_buff, "random") == 0) ||
         (strcmp(input_buff, "get random") == 0);
}

static const InputOption RANDOM_SAVED_PASSAGE_OPTION = {
    .exec = random_saved_passage_option_fn,
    .print_desc = random_saved_passage_option_print_desc,
    .input_check = random_saved_passage_option_input_check,
    .n_sub_options = 5,
    .sub_options =
        (const InputOption *[]){&GLOBAL_INPUT_OPTION, &GET_PASSAGE_OPTION,
                                &GET_SAVED_PASSAGE_OPTION,
                                &SAVED_PASSAGE_INFO_OPTION,
                                &EDIT_SAVED_PASSAGE_OPTION},
    .data = {0}};

// Getting a Saved Passage's Information
void saved_passage_info_option_print_desc(void) {
  puts("show/get info/get field - Get the passage's saved information");
}

bool saved_passage_info_option_fn(InputOption *current_opt, AppEnv env) {
  error_if(current_opt->data.type != SavedPassage,
           "Attempted to Get a Saved Passage's information when there is no "
           "Saved Passage given");
  error_if(current_opt->data.value.saved_passage_obj == NULL,
           "Attempted to Get Information from a NULL Saved Passage");

  char input_buff[INPUT_BUFF_LEN] = "\0";
  const char *input_start = "show";
  size_t input_start_len = strlen(input_start);
  if (strncmp(current_opt->data.input_buff, input_start, input_start_len) ==
          0 &&
      current_opt->data.input_buff[input_start_len] == ' ') {
    strncpy(input_buff, &current_opt->data.input_buff[input_start_len + 1],
            INPUT_BUFF_LEN - 1);
  } else {
    input_get("What field would you like to see (id, message, or context)?: ",
              INPUT_BUFF_LEN, input_buff);
  }

  PassageObjField req_field;
  if (strcmp(input_buff, "id") == 0) {
    req_field = PassageObjId;
  } else if (strcmp(input_buff, "message") == 0) {
    req_field = PassageObjMessage;
  } else if (strcmp(input_buff, "context") == 0) {
    req_field = PassageObjContext;
  } else {
    fprintf(stderr, "%s is not a valid field for a saved passage\n",
            input_buff);
    return false;
  }

  cJSON *field = passage_obj_get_field(
      current_opt->data.value.saved_passage_obj, req_field);
  printf("Here is the passage's saved %s:\n", input_buff);
  printf("%s\n", field->valuestring);
  if (req_field == PassageObjId) {
    if (strcmp(field->valuestring, current_opt->data.value.passage_id) != 0) {
      printf("This is what the ID should be: %s\n",
             current_opt->data.value.passage_id);
      error_if(true, "That's weird! The Saved Passage's ID doesn't match the "
                     "one stored in code!\n");
    }

    printf("It's reference is: ");
    PassageInfo passage_info;
    passage_get_info_from_id(field->valuestring, &passage_info);
    passage_print_reference(passage_info, *env.books_arr, false);
    printf("\n");
  }

  return false;
}

bool saved_passage_info_option_input_check(
    char input_buff[static INPUT_BUFF_LEN]) {
  return (strncmp(input_buff, "show", strlen("show")) == 0) ||
         (strcmp(input_buff, "get info") == 0) ||
         (strcmp(input_buff, "get field") == 0);
}

static const InputOption SAVED_PASSAGE_INFO_OPTION = {
    .exec = saved_passage_info_option_fn,
    .print_desc = saved_passage_info_option_print_desc,
    .input_check = saved_passage_info_option_input_check,
    // NOTE: sub_options should not be accessible here anyway
    .n_sub_options = 1,
    .sub_options = (const InputOption *[]){&GLOBAL_INPUT_OPTION},
    .data = {0}};

// Editing a Saved Passage
void edit_saved_passage_option_print_desc(void) {
  puts("edit/edit info/edit field - Edit the passage's saved information");
}

// NOTE: edited fields must be present in the PASSAGES_FILE file
bool edit_saved_passage_option_fn(InputOption *current_opt, AppEnv env) {
  error_if(current_opt->data.type != SavedPassage,
           "Attempted to Edit a Saved Passage's information when there is no "
           "Saved Passage given");
  error_if(current_opt->data.value.saved_passage_obj == NULL,
           "Attempted to Edit a NULL Saved Passage");

  char input_buff[INPUT_BUFF_LEN] = "\0";
  const char *input_start = "edit";
  size_t input_start_len = strlen(input_start);
  if (strncmp(current_opt->data.input_buff, input_start, input_start_len) ==
          0 &&
      current_opt->data.input_buff[input_start_len] == ' ') {
    strncpy(input_buff, &current_opt->data.input_buff[input_start_len + 1],
            INPUT_BUFF_LEN - 1);
  } else {
    input_get("What field would you like to edit (id, message, or context): ",
              INPUT_BUFF_LEN, input_buff);
  }

  PassageObjField req_field;
  size_t field_input_buff_size = 0;
  if (strcmp(input_buff, "id") == 0) {
    req_field = PassageObjId;
    field_input_buff_size = PASSAGE_INPUT_BUFF_SIZE;
  } else if (strcmp(input_buff, "message") == 0) {
    req_field = PassageObjMessage;
    field_input_buff_size = PASSAGE_MESSAGE_BUFF_SIZE;
  } else if (strcmp(input_buff, "context") == 0) {
    req_field = PassageObjContext;
    field_input_buff_size = PASSAGE_CONTEXT_BUFF_SIZE;
  } else {
    fprintf(stderr, "%s is not a valid field for a saved passage\n",
            input_buff);
    return false;
  }

  char *field_input_buff = (char *)malloc(field_input_buff_size * sizeof(char));
  error_if(field_input_buff == NULL, "failed to malloc field editing buffer");

  // Getting Input
  printf("What would you like to change the passage's %s to%s", input_buff,
         (req_field == PassageObjId) ? " (enter the passage in regularly)"
                                     : "");
  input_get("?: ", field_input_buff_size, field_input_buff);

  // Changing Field in Object
  if (req_field == PassageObjId) {
    PassageInfo passage = {0};
    PassageId passage_id;
    if (!passage_info_get_from_string(field_input_buff, &passage, env.curl,
                                      env.curl_code, env.bible_version,
                                      env.bibles_arr, env.books_arr)) {
      printf("Failed to parse given passage\n");
      free(field_input_buff);
      return false;
    }

    passage_get_id(passage, passage_id);
    cJSON_ReplaceItemInObject(current_opt->data.value.saved_passage_obj,
                              input_buff, cJSON_CreateString(passage_id));

    // Changing current_opt passage_id to the new one
    strncpy(current_opt->data.value.passage_id, passage_id,
            MAX_PASSAGE_ID_LEN - 1);
  } else {
    cJSON_ReplaceItemInObject(current_opt->data.value.saved_passage_obj,
                              input_buff, cJSON_CreateString(field_input_buff));
  }

  // Writing Changes to PASSAGES_FILE
  // Serialize new json As text
  char *json_txt = cJSON_Print(env.saved_passages_json);
  error_if(json_txt == NULL, "failed to parse json after adding new passage");

  // Write json Text to PASSAGES_FILE
  FILE *file = fopen(PASSAGES_FILE, "w");
  error_if(file == NULL, "failed to open " PASSAGES_FILE " up for writing");
  fprintf(file, "%s", json_txt);

  // Clean Up Memory
  free(json_txt);
  fclose(file);
  free(field_input_buff);

  return false;
}

bool edit_saved_passage_option_input_check(
    char input_buff[static INPUT_BUFF_LEN]) {
  return (strncmp(input_buff, "edit", strlen("edit")) == 0) ||
         (strcmp(input_buff, "edit info") == 0) ||
         (strcmp(input_buff, "edit field") == 0);
}

static const InputOption EDIT_SAVED_PASSAGE_OPTION = {
    .exec = edit_saved_passage_option_fn,
    .print_desc = edit_saved_passage_option_print_desc,
    .input_check = edit_saved_passage_option_input_check,
    // NOTE: sub_options should not be accessible here anyway
    .n_sub_options = 1,
    .sub_options = (const InputOption *[]){&GLOBAL_INPUT_OPTION},
    .data = {0}};

// TODO: add quiz option
//   - include getting a passage's content without getting its reference then
//   guessing it and vice versa (verifying if the reference is right, but
//   perhaps not the wording of the passage unless a library is found to compare
//   text
//   - include guessing the message & context
// TODO: add deleting entry option

// Global/Home Option
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
    .n_sub_options = 4,
    .sub_options =
        (const InputOption *[]){&GET_PASSAGE_OPTION, &SAVE_PASSAGE_OPTION,
                                &RANDOM_SAVED_PASSAGE_OPTION,
                                &GET_SAVED_PASSAGE_OPTION},
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
  printf("current - See Current Option\n");
  printf("clear - Clear console\n");
  printf("exit - Exit Program\n");
  puts("---------------------");
}

// Getting and Processing Input
// NOTE: does not append ": " to message
void input_get(const char *message, size_t buff_len, char *input_buff) {
  printf("%s", message);
  fflush(stdout);
  fgets(input_buff, buff_len, stdin);

  // Remove trailing '\n'
  size_t input_buff_strlen = strnlen(input_buff, buff_len);
  if (input_buff[input_buff_strlen - 1] == '\n') {
    input_buff[input_buff_strlen - 1] = '\0';
  }

  fflush(stdout);
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

bool current_opt_req_check(char input_buff[static INPUT_BUFF_LEN]) {
  return (strcmp(input_buff, "current") == 0);
}

bool clear_req_check(char input_buff[static INPUT_BUFF_LEN]) {
  return (strcmp(input_buff, "clear") == 0);
}

bool exit_req_check(char input_buff[static INPUT_BUFF_LEN]) {
  return (strcmp(input_buff, "exit") == 0);
}

void input_process(InputOption *current_option, AppEnv env) {
  // Print Available Options if Requested
  if (input_info_req_check(current_option->data.input_buff)) {
    input_print_options_list(current_option->n_sub_options,
                             current_option->sub_options);
    return;
  }

  if (current_opt_req_check(current_option->data.input_buff)) {
    printf("Your Current Option is:\n");
    current_option->print_desc();
    return;
  }

  if (clear_req_check(current_option->data.input_buff)) {
    // Clear the Console
    printf("\033[2J\033[H");
    return;
  }

  if (exit_req_check(current_option->data.input_buff)) {
    puts("Exiting program");
    exit(EXIT_SUCCESS);
    return;
  }

  size_t i = 0;
  for (; i < current_option->n_sub_options; i++) {
    if (current_option->sub_options[i]->input_check(
            current_option->data.input_buff)) {
      break;
    }
  }

  if (i == current_option->n_sub_options) {
    printf("%s is not a valid option, enter 'info' or 'help' or 'list' to see "
           "available "
           "options\n",
           current_option->data.input_buff);
    return;
  }

  const InputOption *selected_option = current_option->sub_options[i];
  if (selected_option->exec(current_option, env)) {
    input_switch_option(current_option, selected_option);
  }
}
