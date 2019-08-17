#include <catch2/catch.hpp>
#include "cmdrun.hpp"

using namespace cmdrun;

TEST_CASE("can parse empty command")
{
    auto cp = command_parser();
    auto params = cp.parse("");
    REQUIRE(params.has_value() == false);
}

TEST_CASE("can parse a no parameter command")
{
    auto cp = command_parser(command{"do_something"});
    auto result = cp.parse("do_something");
    REQUIRE(result.has_value());
    REQUIRE(result->command == "do_something");
}

TEST_CASE("can parse only commands that were specified")
{
    auto cp = command_parser(command{"my_command"});
    auto result = cp.parse("do_something");
    REQUIRE(result.has_value() == false);
}

TEST_CASE("can parse multiple viable commands")
{
    auto viable_commands = std::vector<std::string>{"do_this", "do_that", "print_status"};
    auto cp = command_parser({
        command{viable_commands[0]},
        command{viable_commands[1]},
        command{viable_commands[2]}
    });
    
    for (const auto& cmd : viable_commands) {
        auto result = cp.parse({cmd});
        REQUIRE(result.has_value());
        CHECK(result->command == cmd);
    }
    
    auto result = cp.parse("bad_command");
    REQUIRE(result.has_value() == false);
}

TEST_CASE("can parse command with a named parameter")
{
    auto cp = command_parser(command{"cmd", {"param1"}});
    auto result = cp.parse("cmd", {"val1"});
    
    REQUIRE(result.has_value());
    CHECK(result->command == "cmd");
    REQUIRE(result->params.size() == 1);
    CHECK(result->params["param1"] == "val1");
}

TEST_CASE("parameters that were not specified during parsing will be defaulted")
{
    auto cp = command_parser(command{"cmd", {"param1", "param2", "param3"}});
    auto result = cp.parse("cmd", {"val1"});
    
    REQUIRE(result.has_value());
    CHECK(result->command == "cmd");
    REQUIRE(result->params.size() == 3);
    CHECK(result->params["param1"] == "val1");
    CHECK(result->params["param2"] == "");
    CHECK(result->params["param3"] == "");
}

TEST_CASE("excessive parameters will be ignored")
{
    auto cp = command_parser(command{"cmd", {"param1"}});
    auto result = cp.parse("cmd", {"val1", "val2", "val3"});
    
    REQUIRE(result.has_value());
    CHECK(result->command == "cmd");
    REQUIRE(result->params.size() == 1);
    CHECK(result->params["param1"] == "val1");
}
