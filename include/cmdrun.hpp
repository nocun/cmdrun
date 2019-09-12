#pragma once

#include <algorithm>
#include <string>
#include <sstream>
#include <array>
#include <vector>
#include <forward_list>
#include <list>
#include <deque>
#include <set>
#include <map>
#include <functional>
#include <cassert>
#include <iostream>
#include <stdexcept>


namespace cmdrun {

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
    if (is.get() != '"')  {
        throw parsing_error("Invalid multi-word string (must start with a quotation mark)");
    }
    
    std::string str;
    
    while (is.good() && is.peek() != '"') {
        char c = is.get();
        
        if (c == '\\' && is.peek() == '"') {
            c = is.get();
        }
        
        str += c;
    }
    
    if (is.get() != '"')  {
        throw parsing_error("Invalid multi-word string (must end with a quotation mark)");
    }
    
    return str;
}

inline void parse_sequence_delimiter(std::istream& is)
{
    is >> std::ws;
    
    if (is.peek() == ',') {
        is.get();
    }
}

template <typename T>
T parse_sequence_element(std::istream& is)
{
    T element;
    is >> element;
    
    if (!is.good()) {
        throw parsing_error("Unable to parse sequence element");
    }

    parse_sequence_delimiter(is);
    return element;
}

template <>
inline std::string parse_sequence_element<std::string>(std::istream& is)
{
    std::string element;
    is >> std::ws;
    
    if (!is.good() || is.peek() == ',') {
        throw parsing_error("Missing element");
    } else if (is.peek() == '"') {
        element = parse_multiword_string(is);
    } else {
        char next = is.peek();
        while (is.good() && !std::isspace(next) && next != ',' && next != '}' ) {
            element += is.get();
            next = is.peek();
        }
    }
    
    parse_sequence_delimiter(is);
    return element;
}

inline std::istream& operator>>(std::istream& is, std::string& value)
{
    is >> std::ws;
    
    if (is.peek() == '"') {
        value = parse_multiword_string(is);
        return is;
    }
    
    // parse a single word
    return std::operator>>(is, value);
}

template <typename T>
std::istream& operator>>(std::istream& is, std::vector<T>& container)
{
    is >> std::ws;
    
    if (is.get() != '{')  {
        throw parsing_error("Invalid vector (must start with a '{')");
    }
    
    is >> std::ws;
    
    while (is.good() && is.peek() != '}') {
        container.push_back(parse_sequence_element<T>(is));
    }
    
    if (is.get() != '}')  {
        throw parsing_error("Invalid vector (must end with a '}')");
    }
    
    return is;
}

template <typename... Args>
std::istream& operator>>(std::istream& is, std::tuple<Args...>& tuple)
{
    is >> std::ws;
    
    if (is.get() != '{')  {
        throw parsing_error("Invalid tuple (must start with a '{')");
    }
    
    is >> std::ws;
    
    tuple = std::tuple<Args...>{ parse_sequence_element<Args>(is)... };
    
    if (is.get() != '}')  {
        throw parsing_error("Invalid tuple (must end with a '}')");
    }
    
    return is;
}

template <typename K, typename V>
std::istream& operator>>(std::istream& is, std::pair<K, V>& p)
{
    std::tuple<K, V> t;
    is >> t;
    p = std::make_pair(std::get<0>(t), std::get<1>(t));
    return is;
}

template <typename T, size_t N>
std::istream& operator>>(std::istream& is, std::array<T, N>& container)
{
    std::vector<T> v;
    is >> v;
    
    if (v.size() != N) {
        throw parsing_error("Invalid static array initialization (number of elements do not match)");
    }
    
    std::copy(begin(v), end(v), begin(container));
    return is;
}

template <typename ValueType, typename Container>
std::istream& parse_container(std::istream& is, Container& container)
{
    std::vector<ValueType> vec;
    is >> vec;
    container = Container(begin(vec), end(vec));
    return is;
}

template <typename Container>
std::istream& parse_container(std::istream& is, Container& container)
{
    return parse_container<typename Container::value_type>(is, container);
}

template <typename T>
std::istream& operator>>(std::istream& is, std::deque<T>& container)
{
    return parse_container(is, container);
}

template <typename T>
std::istream& operator>>(std::istream& is, std::forward_list<T>& container)
{
    return parse_container(is, container);
}

template <typename T>
std::istream& operator>>(std::istream& is, std::list<T>& container)
{
    return parse_container(is, container);
}

template <typename T>
std::istream& operator>>(std::istream& is, std::set<T>& container)
{
    return parse_container(is, container);
}

template <typename K, typename V>
std::istream& operator>>(std::istream& is, std::map<K, V>& container)
{
    return parse_container<std::pair<K, V>>(is, container);
}

template <typename T>
std::istream& operator>>(std::istream& is, std::multiset<T>& container)
{
    return parse_container(is, container);
}

template <typename K, typename V>
std::istream& operator>>(std::istream& is, std::multimap<K, V>& container)
{
    return parse_container<std::pair<K, V>>(is, container);
}

template <typename T>
T parse(std::istream& is)
{
    T value;
    is >> value;
    return value;
}

template <typename Ret, typename... Args, typename Indices = std::index_sequence_for<Args...>>
command_callback create_function_call(std::function<Ret(Args...)> f)
{
    return
        [f](std::istream& params) {
            auto args = std::tuple<Args...>{ parse<Args>(params)... };
            (void)std::apply(f, args);
        };
}
}

using param_list = std::vector<std::string>;

struct command_param_values
{
    std::string command;
    param_list params;
};

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

