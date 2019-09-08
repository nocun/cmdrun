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
    
    SECTION("multi-word string must end with a quotation mark")
    {
        std::istringstream iss(R"("hello world)");
        REQUIRE_THROWS_AS(parse<std::string>(iss), parsing_error);
    }
}


TEST_CASE("can parse vector<int>")
{
    using type = std::vector<int>;
    
    SECTION("can parse empty vectors")
    {
        std::istringstream iss("{}");
        CHECK(parse<type>(iss) == type{});
        
        iss = std::istringstream("  { }");
        CHECK(parse<type>(iss) == type{});
        
        iss = std::istringstream(" { } 3.14");
        CHECK(parse<type>(iss) == std::vector<int>{});
        CHECK(parse<double>(iss) == Approx(3.14));
    }
    
    SECTION("bad vector format result in an exception")
    {
        std::istringstream iss("5");
        REQUIRE_THROWS_AS(parse<type>(iss), parsing_error);
        
        iss = std::istringstream("");
        REQUIRE_THROWS_AS(parse<type>(iss), parsing_error);
        
        iss = std::istringstream("{");
        REQUIRE_THROWS_AS(parse<type>(iss), parsing_error);
        
        iss = std::istringstream("{ , 3 }");
        REQUIRE_THROWS_AS(parse<type>(iss), parsing_error);
        
        iss = std::istringstream(" { 1, 2, 3 ");
        REQUIRE_THROWS_AS(parse<type>(iss), parsing_error);
        
        iss = std::istringstream(" { 1, 2, 3, ");
        REQUIRE_THROWS_AS(parse<type>(iss), parsing_error);
        
        iss = std::istringstream("}{");
        REQUIRE_THROWS_AS(parse<type>(iss), parsing_error);
    }
    
    SECTION("can parse vectors with many elements")
    {
        std::istringstream iss("{3}");
        CHECK(parse<type>(iss) == type{3});
        
        iss = std::istringstream("{1,2}");
        CHECK(parse<type>(iss) == type{1, 2});
        
        iss = std::istringstream("{ -3, 5, 123 , 7     , 999 }");
        CHECK(parse<type>(iss) == type{-3, 5, 123, 7, 999});
    }
    
    SECTION("can parse multiple vectors from a single stream")
    {
        std::istringstream iss("  { }");
        CHECK(parse<type>(iss) == type{});
        CHECK_THROWS_AS(parse<type>(iss), parsing_error);
        
        iss = std::istringstream("{}  {  }");
        CHECK(parse<type>(iss) == type{});
        CHECK(parse<type>(iss) == type{});
        CHECK_THROWS_AS(parse<type>(iss), parsing_error);
        
        iss = std::istringstream("{5, 6 ,8 , 9} {}");
        CHECK(parse<type>(iss) == type{5, 6, 8, 9});
        CHECK(parse<type>(iss) == type{});
        CHECK_THROWS_AS(parse<type>(iss), parsing_error);
        
        iss = std::istringstream("{ 234234 , 165123, 75552, -3425289, 55555} {-123123, 983223, 0 , 123591}    { 700 }");
        CHECK(parse<type>(iss) == type{234234, 165123, 75552, -3425289, 55555});
        CHECK(parse<type>(iss) == type{-123123, 983223, 0, 123591});
        CHECK(parse<type>(iss) == type{700});
        CHECK_THROWS_AS(parse<type>(iss), parsing_error);
    }
}

TEST_CASE("can parse vector<string>")
{
    using type = std::vector<std::string>;

    SECTION("can parse empty vectors")
    {
        std::istringstream iss("{}");
        CHECK(parse<type>(iss) == type{});
        
        iss = std::istringstream("  { }");
        CHECK(parse<type>(iss) == type{});
    }
    
    SECTION("can parse vectors with unquoted strings")
    {
        std::istringstream iss("{ hello, world}");
        CHECK(parse<type>(iss) == type{"hello", "world"});
        
        iss = std::istringstream("  { a , b    ,  cdef  , 4 }");
        CHECK(parse<type>(iss) == type{"a", "b", "cdef", "4"});
    }
    
    SECTION("can parse vectors with quoted strings")
    {
        std::istringstream iss(R"({"abc"})");
        CHECK(parse<type>(iss) == type{"abc"});
        
        iss = std::istringstream(R"({ "" , one, "two three", " 4 "})");
        CHECK(parse<type>(iss) == type{"", "one", "two three", " 4 "});
        
        iss = std::istringstream(R"({including , "vector-specific ,", "} characters", "in the string"})");
        CHECK(parse<type>(iss) == type{"including", "vector-specific ,", "} characters", "in the string"});
    }
}
