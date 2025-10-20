#ifndef CONSTANTS_H
#define CONSTANTS_H

// ESV Definitions
#define ESV_API_KEY "a06ee3811b592c385d53d189e0554e022004ccec"
#define ESV_AUTH_HEADER "Authorization: Token " ESV_API_KEY
#define ESV_ABBR "ESV"
#define ESV_NAME "English Standard Version"
#define ESV_JSON_KEY "is_esv"

#define URL_BUFF_LEN 1024
#define BIBLE_API_KEY "09df334a845ba14eb6a6617dfd187607"
#define DEFAULT_AUTH_HEADER "api-key: " BIBLE_API_KEY
#define BIBLE_ID_BUFF_LEN 32
#define DEFAULT_BIBLE_ABBR "ESV"
#define ENGLISH_LANGUAGE_ID "eng"
#define DEFAULT_LANGUAGE_ID ENGLISH_LANGUAGE_ID
#define WEB_BIBLE_ID "9879dbb7cfe39e4d-02"
#define BIBLES_FILE "./info/bibles.json"
#define BOOKS_FILE "./info/books.json"
#define PASSAGES_FILE "./info/passages.json"

#endif
