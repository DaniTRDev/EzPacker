#ifndef EZDSLLEXER_COMMON_H
#define EZDSLLEXER_COMMON_H

// Shared std/StringUtils base; the Lexer module adds its parser-specific extras below.
#include "EzCommonStd.h"

#include <lexy/action/parse.hpp>  // lexy::parse
#include <lexy/callback/bind.hpp> // parse_state
#include <lexy/callback.hpp>      // value callbacks
#include <lexy/dsl.hpp>           // lexy::dsl::*
#include <lexy/input/string_input.hpp>
#include <lexy_ext/report_error.hpp> // lexy_ext::report_error

#endif // EZDSLLEXER_COMMON_H
