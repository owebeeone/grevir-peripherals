#include <GrevirPeripherals.h>
#include <catch2/catch_test_macros.hpp>
#include <array>
#include <cstring>
#include <utility>
#include <vector>

namespace {
struct Storage {
  using Resource = ardo::EepromResource;
  static constexpr std::size_t capacity = 32;
  inline static std::array<std::uint8_t, capacity> data;
  inline static std::vector<std::size_t> reads;
  inline static std::vector<std::pair<std::size_t, std::uint8_t>> updates;
  inline static unsigned changes = 0;
  static std::uint8_t read(std::size_t address) {
    reads.push_back(address);
    return data.at(address);
  }
  static void update(std::size_t address, std::uint8_t value) {
    updates.emplace_back(address, value);
    if (data.at(address) != value) {
      data.at(address) = value;
      ++changes;
    }
  }
};
struct Fixture {
  Fixture() {
    Storage::data.fill(0);
    Storage::reads.clear();
    Storage::updates.clear();
    Storage::changes = 0;
  }
};
using Region = ardo::EepromReaderWriter<std::uint32_t, 7, Storage>;
}

TEST_CASE_METHOD(Fixture, "storage round trip preserves bytes and uses the configured offset", "[storage]") {
  const std::uint32_t value = 0xff807f01u;
  std::array<std::uint8_t, sizeof(value)> bytes;
  std::memcpy(bytes.data(), &value, sizeof(value));
  Storage::data.fill(0xa5);
  Region::write(value);
  REQUIRE(Storage::updates.size() == sizeof(value));
  for (std::size_t i = 0; i < bytes.size(); ++i) {
    REQUIRE(Storage::updates[i] == std::pair<std::size_t, std::uint8_t>{7 + i, bytes[i]});
  }
  REQUIRE(Storage::data[6] == 0xa5);
  REQUIRE(Storage::data[11] == 0xa5);
  REQUIRE(Region::read() == value);
  REQUIRE(Storage::reads == std::vector<std::size_t>{7, 8, 9, 10});
}

TEST_CASE_METHOD(Fixture, "storage reads preexisting bytes in native object representation", "[storage]") {
  const std::uint32_t value = 0x12345678u;
  std::memcpy(Storage::data.data() + 7, &value, sizeof(value));
  REQUIRE(Region::read() == value);
  REQUIRE(Storage::updates.empty());
}

TEST_CASE_METHOD(Fixture, "storage delegates update semantics including unchanged bytes", "[storage]") {
  using Bytes = std::array<std::uint8_t, 4>;
  using Slot = ardo::EepromReaderWriter<Bytes, 0, Storage>;
  Slot::write(Bytes{1, 2, 3, 4});
  REQUIRE(Storage::changes == 4);
  Slot::write(Bytes{1, 2, 3, 4});
  REQUIRE(Storage::changes == 4);
  REQUIRE(Storage::updates.size() == 8);
  Slot::write(Bytes{1, 2, 9, 4});
  REQUIRE(Storage::changes == 5);
  REQUIRE(Slot::read() == Bytes{1, 2, 9, 4});
}

TEST_CASE_METHOD(Fixture, "storage adjacent regions and the final byte remain independent", "[storage]") {
  using A = ardo::EepromReaderWriter<std::uint32_t, 24, Storage>;
  using B = ardo::EepromReaderWriter<std::array<std::uint8_t, 4>, 28, Storage>;
  struct Module : ardo::ModuleBase<ardo::Parameters<A, B>> {};
  ardo::Application<Module>::runSetup();
  A::write(0xabcdef01u);
  B::write({0x80, 0xff, 0x01, 0x7f});
  REQUIRE(A::read() == 0xabcdef01u);
  REQUIRE(B::read() == std::array<std::uint8_t, 4>{0x80, 0xff, 0x01, 0x7f});
  REQUIRE(Storage::data[31] == 0x7f);
}

TEST_CASE_METHOD(Fixture, "storage application lifecycle performs no reads or writes", "[storage]") {
  struct Module : ardo::ModuleBase<ardo::Parameters<Region>> {};
  ardo::Application<Module>::runSetup();
  ardo::Application<Module>::runLoop();
  REQUIRE(Storage::reads.empty());
  REQUIRE(Storage::updates.empty());
}
