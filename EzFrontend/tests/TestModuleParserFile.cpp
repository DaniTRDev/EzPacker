#include "BasicParserTest.h"

TEST(ModuleParserTestFile, TestFile)
{
    std::ifstream file("testData/TestModuleCode.txt");
    EXPECT_TRUE(file.is_open());
    
    std::string fileContent;
    std::string line = "";
    while (!file.eof())
    {
        std::getline(file, line);
        
        fileContent += line;
        fileContent += '\n';

        line = "";
    }

    auto nodes = BasicParserTest::testRule(
        false, grammar::combinators::manyOf(1, UINT64_MAX, grammar::module()), fileContent);
    EXPECT_EQ(nodes->getChildren().size(), 5);
    
    for(auto &var : nodes->getChildren())
    {
        auto varIdentifier = std::dynamic_pointer_cast<IdentifierNode>(var->getChildren()[1]);
        auto varName = std::dynamic_pointer_cast<TokenTypeNode>(varIdentifier->getChildren()[0]);
        
        std::cout << "Recognised " << varName->getContent() << std::endl;
    }
}