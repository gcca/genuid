#pragma once

#include <cstddef>

namespace genuid {
void Init();
[[nodiscard]] std::size_t LockTierUID();
[[nodiscard]] std::size_t LockFreeUID();
} // namespace genuid
