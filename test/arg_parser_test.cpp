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

TEST_CASE("can parse tuples")
{
    {
        std::istringstream iss("{3}");
        using type = std::tuple<int>;
        CHECK(parse<type>(iss) == type{3});
    }
    
    {
        std::istringstream iss("{500, 6123}");
        using type = std::tuple<int, int>;
        CHECK(parse<type>(iss) == type{500, 6123});
    }
    
    {
        std::istringstream iss("{0, 1, abc}");
        using type = std::tuple<int, int, std::string>;
        CHECK(parse<type>(iss) == type{0, 1, "abc"});
    }
    
    {
        std::istringstream iss(R"({"one, two", 1, 6.28, X}")");
        using type = std::tuple<std::string, int, double, char>;
        auto tuple = parse<type>(iss);
        
        CHECK(std::get<0>(tuple) == "one, two");
        CHECK(std::get<1>(tuple) == 1);
        CHECK(std::get<2>(tuple) == Approx(6.28));
        CHECK(std::get<3>(tuple) == 'X');
    }
}

TEST_CASE("can parse pairs")
{
    {
        std::istringstream iss("{1, 2}");
        using type = std::pair<int, int>;
        CHECK(parse<type>(iss) == type{1, 2});
    }
}

TEST_CASE("can parse sequence containers")
{
    SECTION("arrays")
    {
        {
            using type = std::array<int, 1>;
            std::istringstream iss("{5}");
            CHECK(parse<type>(iss) == type{5});
        }
        
        {
            using type = std::array<int, 5>;
            std::istringstream iss("{6, 1, 3, 2, -7}");
            CHECK(parse<type>(iss) == type{6, 1, 3, 2, -7});
        }
        
        {
            using type = std::array<int, 2>;
            std::istringstream iss("{7}");
            CHECK_THROWS_AS(parse<type>(iss), parsing_error);
            
            iss = std::istringstream("{1, 6, 7}");
            CHECK_THROWS_AS(parse<type>(iss), parsing_error);
        }

    }
    
    SECTION("deques")
    {
        using type = std::deque<int>;
        
        std::istringstream iss("{5, 6, 100, 828495}");
        CHECK(parse<type>(iss) == type{5, 6, 100, 828495});
    }
    
    SECTION("forward_lists")
    {
        using type = std::forward_list<int>;
        
        std::istringstream iss("{5, 6, 100, 828495}");
        CHECK(parse<type>(iss) == type{5, 6, 100, 828495});
    }
    
    SECTION("lists")
    {
        using type = std::list<int>;
        
        std::istringstream iss("{5, 6, 100, 828495}");
        CHECK(parse<type>(iss) == type{5, 6, 100, 828495});
    }
    
}

TEST_CASE("can parse associative containers")
{
    SECTION("sets")
    {
        using type = std::set<int>;
        std::istringstream iss("{2}");
        CHECK(parse<type>(iss) == type{2});
        
        iss = std::istringstream("{-5, 0, 5, 23, -5, 3}");
        CHECK(parse<type>(iss) == type{-5, 0, 5, 23, 3});
    }
    
    SECTION("maps")
    {
        using type = std::map<int, int>;
        
        std::istringstream iss("{}");
        CHECK(parse<type>(iss).size() == 0);
        
        iss = std::istringstream("{{5, 6}}");
        CHECK(parse<type>(iss) == type{{5, 6}});
        
        iss = std::istringstream("{{0, 1}, {2, 3}, {4, 5}}");
        CHECK(parse<type>(iss) == type{{0, 1}, {2, 3}, {4, 5}});
    }
    
    SECTION("multisets")
    {
        using type = std::multiset<int>;
        
        std::istringstream iss("{2}");
        auto multiset = parse<type>(iss);
        CHECK(multiset.size() == 1);
        CHECK(multiset.count(2) == 1);
        
        iss = std::istringstream("{1, 2, 2, 2, 3, 3}");
        multiset = parse<type>(iss);
        CHECK(multiset.size() == 6);
        CHECK(multiset.count(1) == 1);
        CHECK(multiset.count(2) == 3);
        CHECK(multiset.count(3) == 2);
    }
    
    SECTION("multimaps")
    {
        using type = std::multimap<int, int>;
        
        std::istringstream iss("{{1, 2}}");
        auto multimap = parse<type>(iss);
        CHECK(multimap.size() == 1);
        CHECK(multimap.count(1) == 1);
        
        iss = std::istringstream("{{1, 4}, {2, 5}, {2, 6}, {2, 7}, {3, 8}, {3, 9}}");
        multimap = parse<type>(iss);
        CHECK(multimap.size() == 6);
        CHECK(multimap.count(1) == 1);
        CHECK(multimap.count(2) == 3);
        CHECK(multimap.count(3) == 2);
    }
}
