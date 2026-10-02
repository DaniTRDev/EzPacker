#ifndef EZLINKER_COMMON_H
#define EZLINKER_COMMON_H

#include <string>
#include <string_view>
#include <vector>
#include <filesystem>
#include <memory>
#include <optional>
#include <cstdint>
#include <iostream>

#if defined(_WIN32)
  #if defined(EZLINKER_EXPORTS)
    #define EZLINKER_API __declspec(dllexport)
  #else
    #define EZLINKER_API
  #endif
#else
  #define EZLINKER_API __attribute__((visibility("default")))
#endif

#endif // EZLINKER_COMMON_H
