#include "cmdline.h"
#include "tap.h"
#include "wav.h"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>

std::string get_length_string(float seconds)
{
    std::stringstream ss;

    int minutes = int(seconds / 60);
    seconds -= minutes * 60;

    ss << minutes << "m" << seconds << "s";

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
        } catch (std::exception& e) {
            std::cout << "error: Failed to parse '" << files[i] << "', "
                      << e.what() << "\n";
            throw std::runtime_error("Parsing failed");
        }

        std::cout << "\tResulting length: "
                  << get_length_string(wav_files.back().get_length_seconds())
                  << "\n";
    }

    return wav_files;
}

std::vector<wav_t<1, uint8_t>> sideify(
    const std::vector<wav_t<1, uint8_t>>& wav_files, uint32_t side_minutes,
    uint32_t sample_rate)
{
    std::vector<wav_t<1, uint8_t>> sides;
    sides.emplace_back(sample_rate);

    const float side_length = side_minutes * 60;

    for (auto const& wav : wav_files) {
        if (wav.get_length_seconds() > side_length) {
            throw std::runtime_error("Size of tape is larger than one side");
        }

        if (sides.back().get_length_seconds() + wav.get_length_seconds()
            > side_length) {
            sides.emplace_back(sample_rate);
        }

        sides.back().append(wav);
    }

    return sides;
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

        auto sides = sideify(wav_files, args.minutes_arg, args.sample_rate_arg);

        int file_index = 1;
        for (auto& wav : sides) {
            std::stringstream ss;
            ss << args.output_arg << file_index << ".wav";

            std::cout << "Writing file '" << ss.str() << "' (length: "
                      << get_length_string(wav.get_length_seconds())
                      << ")...\n";

            std::ofstream fp_out(ss.str(), std::ofstream::binary);
            if (fp_out.fail()) {
                std::cout << "error: could not open output file '" << ss.str()
                          << "'\n";
                return 1;
            }

            wav.dump(fp_out);
            file_index++;
        }
    } catch (std::exception& e) {
        std::cout << "Failed to parse files: " << e.what() << "\n";
    }

    cmdline_parser_free(&args);

    return 0;
}
