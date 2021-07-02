#ifndef _TAP_H_
#define _TAP_H_
#include <cstdint>
#include <istream>
#include <vector>
#define ZERO 2500

namespace tap {

namespace frequency {
    constexpr uint32_t C64_PAL  = 123156;
    constexpr uint32_t C64_NTSC = 127841;
    constexpr uint32_t VIC_PAL  = 138551;
    constexpr uint32_t VIC_NTSC = 127841;
    constexpr uint32_t C16_PAL  = 110840;
    constexpr uint32_t C16_NTSC = 111860;
}

enum machine_t {
    C64 = 0,
    VIC = 1,
    C16 = 2
};

enum video_t {
    PAL  = 0,
    NTSC = 1
};

constexpr const char* machine_to_str[] = {
    "C64",
    "VIC",
    "C16"
};

constexpr const char* video_to_str[] = {
    "PAL",
    "NTSC"
};

class header_t {
public:
    explicit header_t(std::istream& stream);
    std::string dump() const;

    uint32_t get_frequency() const { return frequency; }
    uint32_t get_size() const { return size; }
    uint8_t  get_version() const { return version; }

private:
    char      marker_string[13]; // "C64-TAPE-RAW\0"
    uint8_t   version;
    machine_t machine;
    video_t   video_type;
    uint32_t  frequency;
    uint32_t  size;
};

class tape_t {
private:
    header_t             header;
    std::vector<uint8_t> data;
    size_t               index;

    uint8_t get_next_byte();

public:
    explicit tape_t(std::istream& stream);
    std::string dump_header() const;

    void     rewind();
    uint32_t get_next_period();
};
}

#endif
