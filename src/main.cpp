#include "tap.h"
#include "wav.h"
#include <array>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

int main(int argc, char* argv[])
{
    std::ifstream fp(
        "/home/danne/code/tapreader/Commando.tap", std::ifstream::binary);

    wav_t<1, 96000, uint8_t> wav;
    tap::tape_t              tape(fp);

    float sample_period = wav.usecs_per_sample();

    std::cout << "Sample period: " << sample_period << "\n";

    std::cout << tape.dump_header();

    try {

        while (true) {
            uint32_t half_period = tape.get_next_period() / 2;

            uint32_t num_samples = half_period / sample_period;

            if (num_samples <= 2) {
                std::cout << "WARNING: Number of samples per waveform is too "
                             "low (raise sample rate of wav file)\n";
            }

            for (int i = 0; i < num_samples; i++) {
                wav.add_sample({ 255 });
            }

            for (int i = 0; i < num_samples; i++) {
                wav.add_sample({ 0 });
            }
        }
    }

    catch (std::exception& e) {
        std::cout << "Exc: " << e.what() << "\n";
    }

    std::ofstream fp_out("/home/danne/out.wav", std::ofstream::binary);

    wav.dump(fp_out);

    return 0;
}
