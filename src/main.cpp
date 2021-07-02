#include "tap.h"
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

#pragma pack(push, 1)

struct format_chunk_t {
    char     chunk_id[4]; // 'fmt '
    uint32_t chunk_size; // 16
    uint16_t audio_format; // PCM = 1
    uint16_t num_channels; // 1 = mono etc
    uint32_t sample_rate; // 44100 ex.
    uint32_t byte_rate; // sample_rate * num_channels * bits_per_sample/8
    uint16_t block_align; // num_channels * bits_per_sample / 8
    uint16_t bits_per_sample; // 8

    format_chunk_t()
    {
        memcpy(chunk_id, "fmt ", 4);
        chunk_size      = 16;
        audio_format    = 1;
        num_channels    = 1;
        sample_rate     = 96000;
        bits_per_sample = 8;
        byte_rate       = sample_rate * num_channels * bits_per_sample / 8;
        block_align     = num_channels * bits_per_sample / 8;
    }

    void dump(std::ostream& stream)
    {
        stream.write(reinterpret_cast<char*>(this), sizeof(format_chunk_t));
    }
};

struct data_chunk_t {
    char                 chunk_id[4]; // data
    uint32_t             chunk_size;
    std::vector<uint8_t> data;

    data_chunk_t()
    {
        memcpy(chunk_id, "data", 4);
        chunk_size = 0;
    }

    void update_size()
    {
        chunk_size = data.size();
    }

    void dump(std::ostream& stream)
    {
        chunk_size = data.size();
        stream.write(reinterpret_cast<char*>(this), 8);
        stream.write(reinterpret_cast<char*>(&data[0]), data.size());
    }
};

struct wav_t {
    char           chunk_id[4]; // 'RIFF'
    uint32_t       chunk_size;
    char           format_id[4]; // 'WAVE'
    format_chunk_t format;
    data_chunk_t   data;

    wav_t()
    {
        memcpy(chunk_id, "RIFF", 4);
        memcpy(format_id, "WAVE", 4);
    }

    void update_size()
    {
        data.update_size();
        chunk_size = 36 + data.chunk_size;
    }

    void dump(std::ostream& stream)
    {
        update_size();
        stream.write(reinterpret_cast<char*>(this), 12);
        format.dump(stream);
        data.dump(stream);
    }
};

#pragma pack(pop)

int main(int argc, char* argv[])
{
    std::ifstream fp("/home/danne/code/tapreader/Commando.tap",
        std::ifstream::binary);

    wav_t wav;

    std::cout << "Hello, world!\n";

    tap::tape_t tape(fp);

    uint32_t sample_period = 1000000 / wav.format.sample_rate;

    std::cout << "Sample period: " << sample_period << "\n";

    std::cout << tape.dump_header();

    try {

        while (true) {
          uint32_t half_period = tape.get_next_period() / 2;

          uint32_t num_samples = half_period / sample_period;

          for(int i = 0; i < num_samples; i++)
          {
            wav.data.data.push_back(255);
          }

          for(int i = 0; i < num_samples; i++)
          {
            wav.data.data.push_back(0);
          }
          
          
        }
    }

    catch (...) {
    }

    std::ofstream fp_out("/home/danne/out.wav", std::ofstream::binary);

    wav.dump(fp_out);

    return 0;
}
