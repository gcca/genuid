#include <atomic>
#include <chrono>
#include <ctime>
#include <mutex>

#include "genuid.hpp"

using namespace genuid;

static std::size_t base_timestamp = 0;
static std::size_t last_timestamp = 0;
static std::size_t datacenter_id = 0;
static std::size_t machine_id = 0;
static std::size_t sequence_number = 0;
static std::mutex uid_mutex;
static std::atomic<std::size_t> lf_sequence_number = 0;

static constexpr std::size_t datacenter_id_bits = 5;
static constexpr std::size_t machine_id_bits = 5;
static constexpr std::size_t sequence_number_bits = 12;

static constexpr std::size_t max_datacenter_id =
    -1L ^ (-1L << datacenter_id_bits);
static constexpr std::size_t max_machine_id = -1L ^ (-1L << machine_id_bits);
static constexpr std::size_t sequence_number_mask =
    -1L ^ (-1L << sequence_number_bits);
static constexpr std::size_t timestamp_mask =
    -1L ^ ((-1L ^ (-1L << sequence_number_bits))
           << ((sizeof(std::size_t) << 3) - sequence_number_bits));

static constexpr std::size_t machine_id_shift = sequence_number_bits;
static constexpr std::size_t datacenter_id_shift =
    sequence_number_bits + machine_id_bits;
static constexpr std::size_t timestamp_shift =
    sequence_number_bits + datacenter_id_bits + machine_id_bits;

[[nodiscard]] static inline std::size_t GetCurrentTimestamp() {
  std::chrono::time_point now = std::chrono::system_clock::now();
  return std::chrono::duration_cast<std::chrono::seconds>(
             now.time_since_epoch())
      .count();
}

[[nodiscard]] static inline std::size_t
GetNextTimestamp(const std::size_t base_timestamp) {
  std::size_t timestamp = GetCurrentTimestamp();
  while (timestamp <= base_timestamp) { timestamp = GetCurrentTimestamp(); }
  return timestamp;
}

void genuid::Init() {
  std::tm db = {0};
  db.tm_year = 2024 - 1900;
  db.tm_mon = 5 - 1;
  db.tm_mday = 3;

  std::time_t tb = std::mktime(&db);

  std::size_t timestamp =
      std::chrono::duration_cast<std::chrono::seconds>(
          std::chrono::system_clock::from_time_t(tb).time_since_epoch())
          .count();

  base_timestamp = timestamp;

  std::size_t start_sequence_number = GetCurrentTimestamp();
  lf_sequence_number.store(start_sequence_number << sequence_number_bits);
}

[[nodiscard]] std::size_t genuid::LockTierUID() {
  std::lock_guard<std::mutex> guard{uid_mutex};
  std::size_t timestamp = GetCurrentTimestamp();

  if (timestamp == last_timestamp) {
    sequence_number = (sequence_number + 1) & sequence_number_mask;
    if (sequence_number == 0) { timestamp = GetNextTimestamp(last_timestamp); }
  } else {
    sequence_number = 0;
  }

  last_timestamp = timestamp;

  return ((timestamp - base_timestamp) << timestamp_shift) |
         (datacenter_id << datacenter_id_shift) |
         (machine_id << machine_id_shift) | sequence_number;
}

[[nodiscard]] std::size_t genuid::LockFreeUID() {
  for (;;) {
    std::size_t timestamp = GetCurrentTimestamp(),
                first_sequence_number = lf_sequence_number.fetch_add(1) + 1;

    if ((timestamp & timestamp_mask) ==
        (first_sequence_number >> sequence_number_bits)) {
      return ((timestamp - base_timestamp) << timestamp_shift) |
             (datacenter_id << datacenter_id_shift) |
             (machine_id << machine_id_shift) |
             (first_sequence_number & sequence_number_mask);
    }

    if (lf_sequence_number.compare_exchange_strong(
            first_sequence_number, first_sequence_number & timestamp_mask)) {
      return first_sequence_number & timestamp_mask;
    }
  }
}
