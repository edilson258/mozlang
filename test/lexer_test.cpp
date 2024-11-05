#include "../src/Lexer.h"

#include <string>
#include <vector>

#include <gtest/gtest.h>

static std::string sourceCode = "fn main(): int {\n"
                                " return 0;\n"
                                "}";

class LexerTest : public ::testing::Test
{
protected:
  Lexer *lexer;

  void SetUp() override { lexer = new Lexer(sourceCode, "test"); }
  void TearDown() override { delete lexer; }
};

TEST_F(LexerTest, tokensCount)
{
  std::vector<Token> tokens;
  while (1)
  {
    Token token = lexer->GetNextToken();
    tokens.push_back(token);
    if (TokenType::Eof == token.Type)
    {
      break;
    }
  }

  EXPECT_EQ(tokens.size(), 12);
}

TEST_F(LexerTest, tokenTypes)
{
  std::vector<TokenType> tokenTypes = {
      TokenType::Fn,      TokenType::Identifier, TokenType::LeftParent, TokenType::RightParent,
      TokenType::Colon,   TokenType::TypeInt,    TokenType::LeftBrace,  TokenType::Return,
      TokenType::Integer, TokenType::Semicolon,  TokenType::RightBrace, TokenType::Eof,
  };

  for (TokenType &tokenType : tokenTypes)
  {
    EXPECT_EQ(tokenType, lexer->GetNextToken().Type);
  }
}
