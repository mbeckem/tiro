#ifndef TIRO_COMMON_ENTITIES_ENTITY_STORAGE_ACCESSORS_HPP
#define TIRO_COMMON_ENTITIES_ENTITY_STORAGE_ACCESSORS_HPP

#include "common/entities/entity_storage.hpp"

// Defines common entity accessors (ptr_to and operator[]) inside a class body.
// Used by classes that contain inner entity storage instances but only want to forward
// convenient element access to their clients.
#define TIRO_ENTITY_STORAGE_ACCESSORS(Entity, EntityId, storage_expr)            \
    ::tiro::NotNull<::tiro::EntityPtr<Entity>> ptr_to(EntityId id) {             \
        return (storage_expr).ptr_to(id);                                        \
    }                                                                            \
    ::tiro::NotNull<::tiro::EntityPtr<const Entity>> ptr_to(EntityId id) const { \
        return (storage_expr).ptr_to(id);                                        \
    }                                                                            \
                                                                                 \
    Entity& operator[](EntityId id) { return (storage_expr)[id]; }               \
    const Entity& operator[](EntityId id) const { return (storage_expr)[id]; }

#endif // TIRO_COMMON_ENTITIES_ENTITY_STORAGE_ACCESSORS_HPP
