#ifndef EZDSLLEXER_COMMON_H
#define EZDSLLEXER_COMMON_H

#include <lexy/action/parse.hpp>  // lexy::parse
#include <lexy/callback/bind.hpp> // parse_state
#include <lexy/callback.hpp>      // value callbacks
#include <lexy/dsl.hpp>           // lexy::dsl::*
#include <lexy/input/string_input.hpp>
#include <lexy_ext/report_error.hpp> // lexy_ext::report_error

#include <charconv>
#include <filesystem>
#include <format>
#include <fstream>
#include <memory_resource>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

#endif // EZDSLLEXER_COMMON_H