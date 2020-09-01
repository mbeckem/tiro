#ifndef TIROPP_FWD_HPP_INCLUDED
#define TIROPP_FWD_HPP_INCLUDED

namespace tiro {

enum class api_errc : int;
enum class severity : int;
enum class value_kind : int;

struct version;
struct compiler_settings;
struct vm_settings;

class error;
class bad_handle_cast;
class api_error;

class compiler;
class module;
class vm;

class handle;
class null;
class boolean;
class integer;
class float_;
class string;
class function;
class tuple;
class array;
class coroutine;
class native;
class type;

class sync_frame;
class async_frame;

template<typename T>
class native_type;

} // namespace tiro

#endif // TIROPP_FWD_HPP_INCLUDED
