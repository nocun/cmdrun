
#include "cmdrun.hpp"

#include <algorithm>
#include <iostream>
#include <vector>
#include <set>

using namespace cmdrun;

struct Math
{
    void sum(int a, int b)
    {
        std::cout << a + b << '\n';
    }
    
    void set_size(std::set<int> s)
    {
        std::cout << s.size() << '\n';
    }
};

void upcase(std::string str)
{
    std::transform(begin(str), end(str), begin(str), ::toupper);
    std::cout << str << '\n';
}

void sort(std::vector<float> v)
{
    std::sort(begin(v), end(v));
    for (const auto& x : v)
        std::cout << x << ' ';
    std::cout << '\n';
}

int main(int argc, const char* argv[])
{
    Math m;
    
    auto cr = command_runner({
        COMMAND("hi", [](){ std::cout << "hello world!\n"; }),
        FUNCTION(upcase),
        FUNCTION(sort),
        METHOD(m, sum),
        METHOD(m, set_size)
    });
    
    cr.run(argc, argv);
    
    return 0;
}
