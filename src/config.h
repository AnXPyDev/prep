#define PREP_CONFIG_INCLUDED

#include <wchar.h>

const wchar_t escape_wc = L'\\';
const wchar_t quote_open = L'`';
const wchar_t quote_close = L'\'';
const wchar_t quote_break = L'#';
const wchar_t quote_force_break = L'$';
const wchar_t quote_reset = L'%';

const wchar_t macro_argument_list_open = L'(';
const wchar_t macro_argument_list_close = L')';
const wchar_t macro_argument_list_delimiter = L',';

const wchar_t *token_inclusive_chars = L"-_";
