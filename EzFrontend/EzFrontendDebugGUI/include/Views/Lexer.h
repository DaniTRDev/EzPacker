/**
 * @file Lexer.h
 * @brief ImGui view that displays debug information about the EzLexer
 *        token stream (token types, source locations, content).
 */
#ifndef EZPACKER_LEXER_H
#define EZPACKER_LEXER_H

#include "EzFrontendDebugGUICommon.h"
#include "IView.h"

class Lexer : public IView
{
  public:
    /**
     * Overrides the render function to show debug information about EzLexer.
     */
    void render() override;

  private:
};

#endif // EZPACKER_LEXER_H
