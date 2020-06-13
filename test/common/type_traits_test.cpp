#include <catch.hpp>

#include "common/type_traits.hpp"

using namespace tiro;

static_assert(std::is_same_v<remove_cvref_t<const int>, int>);
static_assert(std::is_same_v<remove_cvref_t<const int&>, int>);
static_assert(std::is_same_v<remove_cvref_t<const int&&>, int>);
static_assert(std::is_same_v<remove_cvref_t<int>, int>);
static_assert(std::is_same_v<remove_cvref_t<int&>, int>);
static_assert(std::is_same_v<remove_cvref_t<int&&>, int>);

static_assert(std::is_same_v<preserve_const_t<char, int>, char>);
static_assert(std::is_same_v<preserve_const_t<const char, int>, char>);
static_assert(std::is_same_v<preserve_const_t<char, const int>, const char>);
static_assert(std::is_same_v<preserve_const_t<const char, const int>, const char>);
