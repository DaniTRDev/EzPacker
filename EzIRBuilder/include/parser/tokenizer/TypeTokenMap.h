#ifndef EZPACKER_TYPETOKENMAP_H
#define EZPACKER_TYPETOKENMAP_H

#include "EzIRBuilderCommon.h"

inline std::map<std::string, IRTokenType> g_typeTokenMap = {{".i8", IRTokenType::TypeI8},
                                                            {".i16", IRTokenType::TypeI16},
                                                            {".i32", IRTokenType::TypeI32},
                                                            {".i64", IRTokenType::TypeI64},
                                                            {".ptr", IRTokenType::TypePtr}};

#endif // EZPACKER_TYPETOKENMAP_H
