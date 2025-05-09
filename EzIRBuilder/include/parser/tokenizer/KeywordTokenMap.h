#ifndef EZPACKER_KEYWORDTOKENMAP_H
#define EZPACKER_KEYWORDTOKENMAP_H

#include "EzIRBuilderCommon.h"

inline std::map<std::string, IRTokenType> g_keywordTokenMap = {{".arch", IRTokenType::Arch},
                                                               {".end", IRTokenType::End},
                                                               {".label", IRTokenType::Label},
                                                               {".module", IRTokenType::Module},
                                                               {".string", IRTokenType::StringKeyword},
                                                               {".variable", IRTokenType::Variable},
                                                               {".vector", IRTokenType::Vector}};

#endif // EZPACKER_KEYWORDTOKENMAP_H
