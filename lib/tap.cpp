#include "tap.h"
#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace tap {

uint32_t calc_frequency(machine_t machine, video_t video)
{
    switch (machine) {
    case VIC:
        return video == NTSC ? frequency::VIC_NTSC : frequency::VIC_PAL;
    case C16:
        return video == NTSC ? frequency::C16_NTSC : frequency::C16_PAL;
    case C64:
        return video == NTSC ? frequency::C64_NTSC : frequency::C64_PAL;
    default:
        throw std::runtime_error("Invalid machine");
    }
}

header_t::header_t(std::istream& stream)
{
    char in_byte = 0;
    stream.read(marker_string, 12);

    marker_string[12] = '\0';

    if (strncmp(&marker_string[4], "TAPE-RAW", 8) != 0) {
        std::stringstream ss;
        ss << marker_string;
        throw std::runtime_error(std::string("Incorrect marker string: ") + ss.str());
    }

    stream.read(reinterpret_cast<char*>(&version), 1);

    if (version > 1) {
        throw std::runtime_error("Invalid TAP version");
    }

    stream.read(&in_byte, 1);

    switch (in_byte) {
    case C16:
    case VIC:
    case C64:
        machine = machine_t(in_byte);
        break;
    default:
        throw std::runtime_error("Invalid machine type");
    }

    stream.read(&in_byte, 1);
    switch (in_byte) {
    case PAL:
    case NTSC:
        video_type = video_t(in_byte);
        break;
    default:
        throw std::runtime_error("Invalid video standard");
    }

    stream.read(&in_byte, 1);

    char size_chars[4];

    stream.read(size_chars, 4);

    size = (size_chars[0]) | (size_chars[1] << 8) | (size_chars[2] << 16) | (size_chars[3] << 24);

    frequency = calc_frequency(machine, video_type);
}

std::string header_t::dump() const
{
    std::stringstream ss;
    ss << "Tap file header:\n";
    ss << "\tMarker: " << marker_string << "\n";
    ss << "\tVersion: " << int(version) << "\n";
    ss << "\tMachine: " << machine_to_str[machine] << "\n";
    ss << "\tVideo: " << video_to_str[video_type] << "\n";
    ss << "\tFrequency: " << frequency << "\n";
    ss << "\tData size: " << size << "\n";

    return ss.str();
}

tape_t::tape_t(std::istream& stream)
    : header(stream)
    , data(header.get_size())
    , index(0)
{
    stream.read(reinterpret_cast<char*>(&data[0]), header.get_size());

    if (stream.eof()) {
        throw std::runtime_error("Unexpected end of file");
    }

    char b;
    stream.read(&b, 1);

    if (not stream.eof()) {
        throw std::runtime_error("Expected end of file!");
    }
}

std::string tape_t::dump_header() const { return header.dump(); }

void tape_t::rewind()
{
    index = 0;
}

uint8_t tape_t::get_next_byte()
{
    if (index >= data.size()) {
        throw std::runtime_error("OOB");
    }

    return data[index++];
}

uint32_t tape_t::get_next_period()
{

    unsigned int tap_data = get_next_byte();

    if (tap_data != 0x00) {
        return ((tap_data + 0.5) * 1000000 / header.get_frequency());
    } else {
        if (header.get_version() == 0) {
            throw std::runtime_error("version 0 sucks");
        } else if (header.get_version() == 1) {
            unsigned long pause = 0;
            for (int i = 0; i < 3; i++) {
                pause >>= 8;
                pause += (get_next_byte() << 16);
            }

            return (((pause >> 3) + 0.5) * 1000000 / header.get_frequency());
        }
    }

    return 0;
}
}
