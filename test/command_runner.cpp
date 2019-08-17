#include <catch2/catch.hpp>
#include "cmdrun.hpp"

using namespace cmdrun;

TEST_CASE("can run with no arguments")
{
    auto cp = command_runner();
    cp.run("");
}

TEST_CASE("can run a simple command")
{
    bool executed = false;
    auto cp = command_runner(command{"cmd", [&](){ executed = true; }});
    cp.run("cmd");
    
    REQUIRE(executed == true);
}

TEST_CASE("can run command with an argument")
{
    SECTION("integer argument")
    {
        int arg = 0;
        auto cp = command_runner(command{"cmd", [&](int a){ arg = a; }});
        cp.run("cmd 5");
        
        CHECK(arg == 5);
    }
    
    SECTION("string argument")
    {
        std::string arg;
        auto cp = command_runner(command{"cmd", [&](std::string a){ arg = a; }});
        cp.run("cmd abc");
        
        CHECK(arg == "abc");
    }
}

TEST_CASE("can run command with multiple arguments")
{
    int arg_a = 0;
    std::string arg_b;
    float arg_c = 0.0f;
    
    auto cp = command_runner(command{"cmd",
        [&](int a, std::string b, float c) {
            arg_a = a;
            arg_b = b;
            arg_c = c;
        }});
    
    cp.run("cmd 123 abc 3.5f");
    
    CHECK(arg_a == 123);
    CHECK(arg_b == "abc");
    CHECK(arg_c == Approx(3.5f));
}


TEST_CASE("can parse multi-word string arguments with whitespace characters")
{
    std::string arg;
    
    auto cp = command_runner(command{"cmd",
        [&](std::string a) {
            arg = a;
        }});
    
    
    SECTION("with whitespace characters")
    {
        std::string cmd_param = "hi there  !\t!\n what's\r up?";
        cp.run("cmd \"" + cmd_param + "\"");
        CHECK(arg == cmd_param);
    }

    SECTION("with escape sequences")
    {
        SECTION("escapting a quotation symbol")
        {
            std::string cmd_param = "a b\\\"c\\\"d";
            cp.run("cmd \"" + cmd_param + "\"");
            CHECK(arg == "a b\"c\"d");
        }
        
        SECTION("escaping other sequences")
        {
            std::string cmd_param = "escape sequences \\\\ \\? \\@";
            cp.run("cmd \"" + cmd_param + "\"");
            CHECK(arg == cmd_param);
        }
    }
}

