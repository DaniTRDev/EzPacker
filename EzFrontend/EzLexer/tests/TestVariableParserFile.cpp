#include "BasicParserTest.h"

TEST(VariableParseFileTest, TestFile)
{
    std::string fileContent = R"(
# Simple variables
.variable globalTimer: .i32 0;
.variable frameCount: .i32 0;
.variable errorCount: .i32 0;

# Vectors
.variable identityMatrix: .float 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0;
.variable floatMatrix: .float 2.0, 0.0, 0.0, 0.0, 4.343, 0.0, 0.0, 8.0462, 1.0;
.variable rgbColors: .i32 255, 0, 0, 0, 255, 0, 0, 0, 255;
.variable primeNumbers: .i32 2, 3, 5, 7, 11, 13, 17, 19;
.variable appConfig: .string "debug", "1", "timeout", "3000";

# Strings
.variable welcomeMessage: .i8 "Welcome to the system!\nPress any key to continue";
.variable filePath: .i8 "/usr/local/bin/app";
.variable jsonTemplate: .i8 "{\"id\":0,\"value\":\"\"}";)";

    auto result = BasicParserTest::testRule(false, Rules::ManyOf(NodeParsers::Variable()), fileContent);
    BasicParserTest::expectChildCount(11, result);

    for (auto &var : result->getChildren())
    {
        auto varName = var->getChild(0)->getContent();
        auto varTypeName = var->getChild(1)->getContent();

        std::cout << "Recognised " << varName << "with type: " << varTypeName << std::endl;
    }
}