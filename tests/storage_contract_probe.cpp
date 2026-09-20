#include <GrevirPeripherals.h>
#include <limits>

namespace {
struct Storage {
  using Resource = ardo::EepromResource;
  static constexpr unsigned capacity = 32;
};
struct OtherStorage : Storage { struct Resource {}; };
struct SameStorageAdapter : Storage {};
struct HugeStorage : Storage {
  static constexpr std::uint64_t capacity = std::uint64_t(1) << 32;
};
struct NonTrivial { virtual ~NonTrivial() {} };
template <int Address, typename Backend = Storage, typename T = std::uint32_t>
using Region = ardo::EepromReaderWriter<T, Address, Backend>;
template <typename... Params>
struct Module : ardo::ModuleBase<ardo::Parameters<Params...>> {};
template <typename... Params>
using App = ardo::Application<Module<Params...>>;
struct WholeStore {
  using Claims = ardo::ResourceClaim<ardo::EepromResource>;
  static void runSetup() {}
  static void runLoop() {}
};
template <int Id>
struct Case;
template <> struct Case<0> { using Type = App<Region<0>, Region<4>>; };
template <> struct Case<1> { using Type = App<Region<0>, Region<0>>; };
template <> struct Case<2> { using Type = App<Region<0>, Region<2>>; };
template <> struct Case<3> { using Type = App<Region<0>, Region<0, OtherStorage>>; };
template <> struct Case<4> { using Type = App<Region<0>, Region<0, SameStorageAdapter>>; };
template <> struct Case<5> { using Type = Region<0, Storage, NonTrivial>; };
template <> struct Case<6> { using Type = Region<-1>; };
template <> struct Case<7> { using Type = Region<30>; };
template <> struct Case<8> { using Type = Region<std::numeric_limits<std::int32_t>::max(), HugeStorage>; };
template <> struct Case<9> { using Type = App<Region<31, Storage, std::uint8_t>>; };
template <> struct Case<10> { using Type = App<Region<0>, WholeStore>; };
static_assert(sizeof(typename Case<CASE_ID>::Type) > 0);
}
