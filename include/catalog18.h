#pragma once
#include "block.h"
#include <cstddef>
namespace game {
struct CatalogEntry {int legacyId,metadata;const char* name;Block block;bool available;};
inline constexpr CatalogEntry Catalog18[]={
#include "content18_catalog.inc"
};
inline constexpr size_t Catalog18Size=sizeof(Catalog18)/sizeof(*Catalog18);
}
