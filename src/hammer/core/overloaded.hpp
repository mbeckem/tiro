#ifndef HAMMER_CORE_OVERLOADED_HPP
#define HAMMER_CORE_OVERLOADED_HPP

#include <type_traits>
#include <utility>

namespace hammer {

/**
 * Constructs a class composed of lambda expressions, where each
 * lambda takes place in the overload resolution. This class is very useful
 * for ad-hoc visitor implementations.
 * 
 * See https://www.bfilipek.com/2019/02/2lines3featuresoverload.html
 * 
 * Usage expample:
 * 
 *      auto visitor = overloaded{
 *          [](int i) { std::cout << "its an int!" << std::endl; },
 *          [](double d) { std::cout << "its a double!" << std::endl; }
 *      };
 *      visitor(4);     // int
 *      visitor(4.5);   // double
 * 
 */
template<class... Functions>
struct Overloaded : Functions... {
    using Functions::operator()...;
};

template<class... Functions>
Overloaded(Functions...)->Overloaded<Functions...>;

} // namespace hammer

#endif // HAMMER_CORE_OVERLOADED_HPP
