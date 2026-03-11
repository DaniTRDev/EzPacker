/**
 * @file Output.h
 * @brief ImGui view that displays compiler errors and warnings collected
 *        by the ErrorCollector during tokenization, parsing, and semantic analysis.
 */
#ifndef EZPACKER_OUTPUT_H
#define EZPACKER_OUTPUT_H

#include "EzFrontendDebugGUICommon.h"
#include "IView.h"

class Output : public IView
{
  public:
    /**
     * Creates the error view with the given error collector.
     * @param errorCollector
     */
    explicit Output(const std::shared_ptr<ErrorCollector> &errorCollector);
    /**
     * Returns "Output".
     * @return const char*
     */
    const char *getName() override;

    /**
     * Renders the view with ImGui.
     */
    void render() override;

  private:
    std::shared_ptr<ErrorCollector> m_errorCollector;
};

#endif // EZPACKER_OUTPUT_H
