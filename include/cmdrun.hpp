#pragma once

#include <algorithm>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <functional>
#include <cassert>
#include <iostream>
#include <stdexcept>


namespace cmdrun {

using param_list = std::vector<std::string>;

struct command_param_values
{
    std::string command;
    param_list params;
};

struct command_param_map {
    std::string command;
    std::map<std::string, std::string> params;
};

using command_callback = std::function<void(std::istream&)>;

namespace detail {

struct parsing_error : std::runtime_error
{
    parsing_error(const std::string& message, const std::string& command_ = "", int error_pos_ = -1):
        std::runtime_error(message), command{command_}, error_pos{error_pos_} {}
    
    std::string command;
    int error_pos;
};

inline std::string parse_multiword_string(std::istream& is)
{
    is.unsetf(std::ios_base::skipws);
    assert(is.peek() == '"');
    is.get();
    
    std::string str;
    char c = '\0';
    bool escape_symbol = false;
    
    while (std::operator>>(is, c)) {
        if (c == '"' && !escape_symbol) {
            break;
        }
        else if (c != '"' && escape_symbol) {
            str += '\\';
        }
        
        escape_symbol = c == '\\';
        
        if (!escape_symbol) {
            str += c;
        }
    }
    
    // if c is not '"' - we are missing the closing quote
    // throw exception
    
    is.setf(std::ios_base::skipws);
    return str;
}

inline std::istream& operator>>(std::istream& is, std::string& value)
{
    is >> std::ws;
    
    if (is.peek() == '"') {
        value = parse_multiword_string(is);
        return is;
    }
    
    // read a single word string
    return std::operator>>(is, value);
}

template <typename T>
T parse(std::istream& is)
{
    T value;
    is >> value;
    return value;
}

template <typename T, size_t I, typename ArgTuple>
void get_argument(std::istream& cmd_stream, ArgTuple& args)
{
    std::get<I>(args) = parse<typename std::decay<T>::type>(cmd_stream);
}

template <typename... Args, std::size_t... I>
auto create_arguments(std::istream& cmd_stream, std::index_sequence<I...>)
{
    auto args = std::tuple<Args...>();
    (get_argument<Args, I>(cmd_stream, args), ...);
    return args;
}

template <typename Ret, typename ...Args, typename Indices = std::index_sequence_for<Args...>>
command_callback create_function_call(std::function<Ret(Args...)> f)
{
    return
        [f](std::istream& params) {
            std::tuple<Args...> args = create_arguments<Args...>(params, Indices{});
            (void)std::apply(f, args);
        };
}
}

struct command
{
public:
    command(const std::string& name_, const param_list& params_ = {}):
        name{name_}, params{params_} {}
    
    template <typename Callback>
    command(const std::string& name_, Callback callback_):
        command{name_}
    {
        std::function callback_fn = std::forward<Callback>(callback_);
        callback = cmdrun::detail::create_function_call(callback_fn);
    }
    
    std::string name;
    param_list params;
    command_callback callback;
};

namespace detail
{

inline command_param_values parse_args(int argc, const char* argv[])
{
    command_param_values result;
    
    if (argc <= 1) {
        return {};
    }
    
    result.command = std::string(argv[1]);
    result.params.resize(static_cast<size_t>(argc-2));
    
    for (size_t i=2; i<static_cast<size_t>(argc); i++) {
        result.params[i-2] = std::string(argv[i]);
    }
    
    return result;
}

}

class command_parser
{
    std::vector<command> commands;
    
public:
    command_parser(const command& command_):
        commands{command_} {}
    
    command_parser(const std::vector<command>& commands_ = {}):
        commands{commands_} {}
    
    std::optional<command_param_map> parse(const std::string& command, const param_list& params = {}) const
    {
        const auto it = std::find_if(begin(commands), end(commands),
            [&command](const auto& cmd) {
                return cmd.name == command;
            });
        
        if (it == end(commands)) {
            return std::nullopt;
        }
        
        return command_param_map{command, get_argument_map(it->params, params)};
    }
    
    std::optional<command_param_map> parse(int argc, const char* argv[])
    {
        auto args = cmdrun::detail::parse_args(argc, argv);
        return parse(args.command, args.params);
    }
    
private:
    std::map<std::string, std::string> get_argument_map(const param_list& arg_names, const param_list& params) const
    {
        std::map<std::string, std::string> argMap;
        
        for (size_t i=0; i<arg_names.size(); i++) {
            const auto& name = arg_names[i];
            const auto& value = i < params.size() ? params[i] : std::string();
            argMap[name] = value;
        }
        
        return argMap;
    }
};


class command_runner {
    std::vector<command> commands;
    
public:
    command_runner(const command& command_):
        commands{command_} {}
    
    command_runner(const std::vector<command>& commands_ = {}):
        commands{commands_} {}
    
    void run(const std::string& command, std::istream& args) const
    {
        const auto it = std::find_if(begin(commands), end(commands),
            [&command](const auto& cmd) {
                return cmd.name == command;
            });
        
        if (it->callback) {
            it->callback(args);
        }
    }
    
    void run(int argc, const char* argv[]) const
    {
        auto args = cmdrun::detail::parse_args(argc, argv);
        return run(args.command);
    }
    
    void run(const std::string& command_line) const
    {
        std::istringstream cmd_stream(command_line);
        std::string command;
        
        cmd_stream >> command;
        
        const auto it = std::find_if(begin(commands), end(commands),
            [&command](const auto& cmd) {
                return cmd.name == command;
            });
        
        if (it != end(commands) && it->callback) {
            it->callback(cmd_stream);
        }
    }
    
};



}
