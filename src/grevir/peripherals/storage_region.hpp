#pragma once

#include <grevir/base/utility.hpp>
#include <grevir/base/compat/cstdint.hpp>
#include <grevir/base/compat/type_traits.hpp>
#include <grevir/core/resource_claims.hpp>
#include <grevir/base/compat/cstddef.hpp>
#include <grevir/base/compat/limits.hpp>

namespace ardo {

class EepromResource {};

/** Typed region over a caller-supplied byte store.
 * Backend provides Resource, capacity (bytes), read(address), update(address, byte).
 * Values use their native object representation, not a portable wire format.
 */
template <typename T, std::int32_t Address, typename Backend>
class EepromReaderWriter : public setl::not_copyable {
public:
  using value_type = T;
  static_assert(std::is_trivially_copyable<T>::value,
    "GREVIR_STORAGE_TYPE_NOT_TRIVIALLY_COPYABLE");
  static_assert(Address >= 0, "GREVIR_STORAGE_NEGATIVE_ADDRESS");
  static constexpr std::uint64_t end_address =
    std::uint64_t(Address >= 0 ? Address : 0) + sizeof(T);
  static_assert(end_address <= (std::numeric_limits<int>::max)(),
    "GREVIR_STORAGE_CLAIM_RANGE_OVERFLOW");
  static_assert(end_address <= Backend::capacity,
    "GREVIR_STORAGE_REGION_EXCEEDS_CAPACITY");

  using Claims = ResourceClaim<range_claim<typename Backend::Resource,
    Address, static_cast<int>(end_address)>>;

  static value_type read() {
    value_type result{};
    auto* bytes = reinterpret_cast<unsigned char*>(&result);
    for (std::size_t i = 0; i < sizeof(value_type); ++i) {
      bytes[i] = Backend::read(static_cast<std::size_t>(Address) + i);
    }
    return result;
  }

  static void write(const value_type& value) {
    const auto* bytes = reinterpret_cast<const unsigned char*>(&value);
    for (std::size_t i = 0; i < sizeof(value_type); ++i) {
      Backend::update(static_cast<std::size_t>(Address) + i, bytes[i]);
    }
  }

  static void runSetup() {}
  static void runLoop() {}
};

} // namespace ardo
