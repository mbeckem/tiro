#ifndef TIRO_MODULES_MODULES_HPP
#define TIRO_MODULES_MODULES_HPP

#include "tiro/objects/modules.hpp"

namespace tiro::vm {

// TODO submodules should be members of their parent module.
Module create_std_module(Context& ctx);
Module create_io_module(Context& ctx);

} // namespace tiro::vm

#endif // TIRO_MODULES_MODULES_HPP
