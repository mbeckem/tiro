#ifndef TIRO_VM_HANDLES_FWD_HPP
#define TIRO_VM_HANDLES_FWD_HPP

namespace tiro::vm {

template<typename T, typename Enable = void>
struct WrapperTraits;

template<typename T>
class Handle;

template<typename T>
class MutHandle;

template<typename T>
class OutHandle;

template<typename T>
class MaybeHandle;

template<typename T>
class MaybeMutHandle;

template<typename T>
class MaybeOutHandle;

template<typename T>
class HandleSpan;

template<typename T>
class MutHandleSpan;

class RootedStack;

class Scope;

template<typename T>
class Local;

template<typename T>
class LocalArray;

template<typename T>
class External;

class ExternalStorage;

} // namespace tiro::vm

#endif // TIRO_VM_HANDLES_FWD_HPP
