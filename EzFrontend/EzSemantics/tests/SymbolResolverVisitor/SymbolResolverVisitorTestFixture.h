#ifndef EZPACKER_SYMBOLRESOLVERVISITORTESTFIXTURE_H
#define EZPACKER_SYMBOLRESOLVERVISITORTESTFIXTURE_H

#include "EzSemantics.h"
#include <gtest/gtest.h>

class SymbolResolverVisitorTestFixture : public ::testing::Test
{
  public:
    /**
     * Creates the test fixture by instantiating the scope manager.
     */
    void SetUp() override;
    
    /**
     * Destroys the instance of the scope manager.
     */
    void TearDown() override;
    
  private:
};

#endif // EZPACKER_SYMBOLRESOLVERVISITORTESTFIXTURE_H
