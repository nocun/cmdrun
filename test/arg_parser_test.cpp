#include <catch2/catch.hpp>
#include "cmdrun.hpp"

#define ARGV_SIZE(argv) (sizeof(argv)/sizeof(*argv))

using namespace cmdrun::detail;

TEST_CASE("can parse empty command line")
{
    const char program[] = "test";
    const char* argv[] = { program };
    
    auto args = parse_args(ARGV_SIZE(argv), argv);
    
    CHECK(args.command == "");
    CHECK(args.params.empty());
}

TEST_CASE("can parse single command")
{
    const char program[] = "test";
    const char* argv[] = { program, "my_command" };
    
    auto args = parse_args(ARGV_SIZE(argv), argv);
    
    CHECK(args.command == "my_command");
    CHECK(args.params.empty());
}

TEST_CASE("can parse command with parameters")
{
    const char program[] = "test";
    const char* argv[] = { program, "my_command", "param1", "param2" };
    
    auto args = parse_args(ARGV_SIZE(argv), argv);
    
    CHECK(args.command == "my_command");
    CHECK(args.params.size() == 2);
    CHECK(args.params[0] == "param1");
    CHECK(args.params[1] == "param2");
}

using namespace cmdrun::detail;

TEST_CASE("can parse strings")
{
    SECTION("can parse an empty string")
    {
        std::istringstream iss("");
        CHECK(parse<std::string>(iss) == "");
    }
    
    SECTION("can parse a single word")
    {
        std::istringstream iss("word");
        CHECK(parse<std::string>(iss) == "word");
    }
    
    SECTION("can parse a single word in quotes")
    {
        std::istringstream iss(R"("word")");
        CHECK(parse<std::string>(iss) == "word");
    }
    
    SECTION("can parse multiple words")
    {
        SECTION("one by one")
        {
            std::istringstream iss("multiple words");
            CHECK(parse<std::string>(iss) == "multiple");
            CHECK(parse<std::string>(iss) == "words");
        }
        
        SECTION("one by one in quotes")
        {
            std::istringstream iss(R"("multiple" "words")");
            CHECK(parse<std::string>(iss) == "multiple");
            CHECK(parse<std::string>(iss) == "words");
        }
        
        SECTION("as a single string")
        {
            std::istringstream iss(R"("multiple words")");
            CHECK(parse<std::string>(iss) == "multiple words");
        }
    }
    
    SECTION("escaped quotation symbol does not end parsing")
    {
        std::istringstream iss(R"("need to \"quote\" something")");
        CHECK(parse<std::string>(iss) == R"(need to "quote" something)");
    }
    
    SECTION("escaping only work on the quotation symbol")
    {
        std::istringstream iss(R"("this is a \"random\" string c:\abc \\ def")");
        CHECK(parse<std::string>(iss) == R"(this is a "random" string c:\abc \\ def)");
    }
}

