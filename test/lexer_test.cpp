#include "../src/Lexer.h"

#include <string>
#include <vector>

#include <gtest/gtest.h>

#define EXPECTED_TOKENS_COUNT 12

static std::string sourceCode = "fn main(): int {\n"
                                "  return 0;\n"
                                "}";

class LexerTest : public ::testing::Test
{
protected:
  Lexer *lexer;

  std::vector<Token> ListTokens()
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
    return tokens;
  }

  void SetUp() override { lexer = new Lexer(sourceCode, "test"); }
  void TearDown() override { delete lexer; }
};

TEST_F(LexerTest, tokensCount) { ASSERT_EQ(ListTokens().size(), EXPECTED_TOKENS_COUNT); }

TEST_F(LexerTest, tokenTypes)
{
  std::vector<Token> tokens                 = ListTokens();
  std::vector<TokenType> expectedTokenTypes = {
      TokenType::Fn,      TokenType::Identifier, TokenType::LeftParent, TokenType::RightParent,
      TokenType::Colon,   TokenType::TypeInt,    TokenType::LeftBrace,  TokenType::Return,
      TokenType::Integer, TokenType::Semicolon,  TokenType::RightBrace, TokenType::Eof,
  };

  for (unsigned long i = 0; i < EXPECTED_TOKENS_COUNT; ++i)
  {
    ASSERT_EQ(expectedTokenTypes.at(i), tokens.at(i).Type);
  }
}

TEST_F(LexerTest, tokenSpans)
{
  std::vector<Token> tokens                 = ListTokens();
  std::vector<TokenSpan> expectedTokenSpans = {
      TokenSpan(1, 1, 0, 1),    TokenSpan(1, 4, 3, 6),    TokenSpan(1, 8, 7, 7),    TokenSpan(1, 9, 8, 8),
      TokenSpan(1, 10, 9, 9),   TokenSpan(1, 12, 11, 13), TokenSpan(1, 16, 15, 15), TokenSpan(2, 3, 19, 24),
      TokenSpan(2, 10, 26, 26), TokenSpan(2, 11, 27, 27), TokenSpan(3, 1, 29, 29),  TokenSpan(3, 2, 30, 30),
  };

  for (unsigned long i = 0; i < EXPECTED_TOKENS_COUNT; ++i)
  {
    ASSERT_EQ(expectedTokenSpans.at(i), tokens.at(i).Span);
  }
}

TEST_F(LexerTest, tokenLabels)
{
  std::vector<Token> tokens                    = ListTokens();
  std::vector<std::string> expectedTokenLabels = {"fn", "main", "(", ")", ":", "int", "{", "return", "0", ";", "}"};

  for (unsigned long i = 0; i < (EXPECTED_TOKENS_COUNT - 1); ++i)
  {
    ASSERT_EQ(expectedTokenLabels.at(i), tokens.at(i).Data);
  }
}
