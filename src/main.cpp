#include "tap.h"
#include "wav.h"
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

void usage()
{
    std::cout << "\n";
    std::cout << "usage:\n";
    std::cout << "tapreader -i <tap-file> -o <wav-file> [-s sample_rate (default 96000)]\n";
}

int main(int argc, char* argv[])
{
    const char *in_file = nullptr;
    const char *out_file = nullptr;
    uint32_t sample_rate = 9600;
    
    for(int i = 1; i < argc; i++)
    {
        if(strcmp(argv[i], "-i") == 0 && i + 1 != argc)
        {
            in_file = argv[++i];
        }
        if(strcmp(argv[i], "-o") == 0 && i + 1 != argc)
        {
            out_file = argv[++i];
        }
        if(strcmp(argv[i], "-s") == 0 && i + 1 != argc)
        {
            sample_rate = atoi(argv[++i]);
        }
    }


    if(!in_file)
    {
        std::cout << "error: no input tap file specified\n";
        usage();
        return 1;
    }

    if(!out_file)
    {
        std::cout << "error: no output wav file specified\n";
        usage();
        return 1;
    }
    
    std::ifstream fp(
        "/home/danne/code/tapreader/Commando.tap", std::ifstream::binary);

    wav_t<1, uint8_t> wav(sample_rate);
    tap::tape_t              tape(fp);

    float sample_period = wav.usecs_per_sample();

    if (sample_period > 11) {
        std::cout << "WARNING: Number of samples per waveform is too "
            "low (raise sample rate of wav file)\n";
        return 2;
    }

    std::cout << tape.dump_header();

    std::cout << "Creating waveform...\n";

    try {

        while (true) {
            uint32_t half_period = tape.get_next_period() / 2;
            uint32_t num_samples = half_period / sample_period;

            for (int i = 0; i < num_samples; i++) {
                wav.add_sample({ 255 });
            }

            for (int i = 0; i < num_samples; i++) {
                wav.add_sample({ 0 });
            }
        }
    }

    catch (std::exception& e) {
    }

    std::cout << "Writing wav file...\n";

    std::ofstream fp_out("/home/danne/out.wav", std::ofstream::binary);

    wav.dump(fp_out);

    std::cout << "All done!\n";

    return 0;
}
