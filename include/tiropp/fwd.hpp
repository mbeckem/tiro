#ifndef TIROPP_FWD_HPP_INCLUDED
#define TIROPP_FWD_HPP_INCLUDED

namespace tiro {

// tiropp/compiler.hpp
enum class severity : int;
struct compiler_settings;
struct compiler_message;
class compiler;
class compiled_module;

// tiropp/error.hpp
enum class api_errc : int;
class error;
class api_error;

// tiropp/objects.hpp
enum class value_kind : int;
class bad_handle_cast;
class handle;
class null;
class boolean;
class integer;
class float_;
class string;
class function;
class tuple;
class record;
class array;
class result;
class exception;
class coroutine;
class module;
class native;
class type;

class sync_frame;
class async_frame;
class resumable_frame;

// tiropp/native_type.hpp
template<typename T>
class native_type;

// tiropp/version.hpp
struct version;

// tiropp/vm.hpp
struct vm_settings;
class vm;

} // namespace tiro

#endif // TIROPP_FWD_HPP_INCLUDED
