#include "cmdline.h"
#include "tap.h"
#include "wav.h"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>

std::string get_length_string(float seconds)
{
    std::stringstream ss;

    int minutes = int(seconds / 60);
    seconds -= minutes * 60;

    ss << minutes << "m" << std::fixed << std::setprecision(2) << seconds
       << "s";

    return ss.str();
}

std::string get_sample_offset_and_length_string(
    size_t offset, size_t length, uint32_t sample_rate)
{
    std::stringstream ss;

    float seconds = float(offset) / sample_rate;

    ss << "offset: " << get_length_string(seconds) << ", length: ";

    seconds = float(length) / sample_rate;

    ss << get_length_string(seconds);

    return ss.str();
}

std::vector<wav_t<1, uint8_t>> parse_input_files(char const* const* files,
    int num_files, uint32_t sample_rate, uint32_t spacing)
{
    std::vector<wav_t<1, uint8_t>> wav_files;

    for (int i = 0; i < num_files; i++) {

        std::cout << "Parsing '" << files[i] << "'...\n";
        std::ifstream fp(files[i], std::ifstream::binary);

        if (fp.fail()) {
            std::cout << "error: Could not open input file '" << files[i]
                      << "'\n";
            throw std::runtime_error("File error");
        }

        try {
            tap::tape_t tape(fp);

            wav_files.emplace_back(sample_rate);

            auto& wave_file = wav_files.back();

            bool has_warned = false;

            while (!tape.at_end()) {
                uint32_t half_period = tape.get_next_period() / 2;
                uint32_t num_samples
                    = half_period / wave_file.usecs_per_sample();

                if (!has_warned && num_samples < 12) {
                    has_warned = true;
                    std::cout << "WARNING: number of samples per waveform is "
                                 "too low, raise sample rate\n";
                }

                for (int i = 0; i < num_samples; i++) {
                    wave_file.add_sample({ 255 });
                }

                for (int i = 0; i < num_samples; i++) {
                    wave_file.add_sample({ 0 });
                }
            }

            wave_file.add_silence(spacing);

            wave_file.set_full_info(files[i]);
        } catch (std::exception& e) {
            std::cout << "error: Failed to parse '" << files[i] << "', "
                      << e.what() << "\n";
            throw std::runtime_error("Parsing failed");
        }

        std::cout << "\tlength: "
                  << get_length_string(wav_files.back().get_length_seconds())
                  << "\n";
    }

    return wav_files;
}

std::vector<wav_t<1, uint8_t>> sideify(std::vector<wav_t<1, uint8_t>> wav_files,
    uint32_t side_minutes, uint32_t sample_rate)
{
    std::vector<wav_t<1, uint8_t>> sides;
    sides.emplace_back(sample_rate);

    const float side_length = side_minutes * 60;

    bool done = false;

    while (!done) {
        bool candidate_found = false;

        for (auto it = wav_files.begin(); it != wav_files.end(); it++) {
            if (it->get_length_seconds() > side_length) {
                throw std::runtime_error(
                    "Size of tape is larger than one side");
            }

            if (sides.back().get_length_seconds() + it->get_length_seconds()
                < side_length) {
                sides.back().append(*it);
                wav_files.erase(it);
                candidate_found = true;
                break;
            }
        }

        if (!candidate_found) {
            sides.emplace_back(sample_rate);
        }

        done = wav_files.empty();
    }

    return sides;
}

void write_outputs(
    std::vector<wav_t<1, uint8_t>>& wav_files, const char* prefix)
{
    int file_index = 1;
    for (auto& wav : wav_files) {

        std::stringstream ss;

        ss << prefix << file_index << ".wav";

        std::cout << "Writing file '" << ss.str() << "' (length: "
                  << get_length_string(wav.get_length_seconds()) << ")...\n";

        std::ofstream fp_out(ss.str(), std::ofstream::binary);
        if (fp_out.fail()) {
            std::stringstream ss;
            ss << "error: could not open output file '" << ss.str() << "'\n";
            throw std::runtime_error(ss.str());
        }

        wav.dump(fp_out);

        ss.str("");

        ss << "\tContents: \n";

        for (const auto& index_data : wav.get_index_data()) {
            ss << "\t\t"
               << get_sample_offset_and_length_string(index_data.offset,
                      index_data.length, wav.get_sample_rate());
            ss << ", file: " << index_data.information << "\n";
        }
        std::cout << ss.str();
        file_index++;
    }
}

int main(int argc, char* argv[])
{
    gengetopt_args_info args;
    if (cmdline_parser(argc, argv, &args) != 0) {
        return 1;
    }

    if (!args.inputs_num) {
        std::cout << "You must supply at least one input file. See --help for "
                     "more information\n";
        return 1;
    }

    try {
        auto wav_files = parse_input_files(args.inputs, args.inputs_num,
            args.sample_rate_arg, args.spacing_arg);

        std::sort(wav_files.begin(), wav_files.end(),
            [](const auto& wav1, const auto& wav2) -> bool {
                return wav1.get_length_seconds() > wav2.get_length_seconds();
            });

        auto sides = sideify(wav_files, args.minutes_arg, args.sample_rate_arg);

        write_outputs(sides, args.output_arg);

    } catch (std::exception& e) {
        std::cout << "Failed to parse files: " << e.what() << "\n";
    }

    cmdline_parser_free(&args);

    return 0;
}
