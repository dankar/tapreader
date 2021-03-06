#ifndef _WAV_H_
#define _WAV_H_

#include <array>
#include <bits/stdint-uintn.h>
#include <cstring>
#include <vector>

#pragma pack(push, 1)

constexpr uint16_t PCM_FORMAT = 1;

template <uint8_t _num_channels, typename data_type_t> class format_chunk_t {
private:
    char     chunk_id[4]; // 'fmt '
    uint32_t chunk_size; // 16
    uint16_t audio_format; // PCM = 1
    uint16_t num_channels; // 1 = mono etc
    uint32_t sample_rate; // 44100 ex.
    uint32_t byte_rate; // sample_rate * num_channels * bits_per_sample/8
    uint16_t block_align; // num_channels * bits_per_sample / 8
    uint16_t bits_per_sample; // 8

public:
    format_chunk_t(uint32_t _sample_rate)
        : chunk_size(sizeof(*this) - sizeof(chunk_id) - sizeof(chunk_size))
        , audio_format(PCM_FORMAT)
        , num_channels(_num_channels)
        , sample_rate(_sample_rate)
        , bits_per_sample(sizeof(data_type_t) * 8)
    {
        memcpy(chunk_id, "fmt ", 4);
        byte_rate   = sample_rate * num_channels * bits_per_sample / 8;
        block_align = num_channels * bits_per_sample / 8;
    }

    float usec_per_sample() { return 1000000.0f / sample_rate; }

    void dump(std::ostream& stream)
    {
        stream.write(reinterpret_cast<char*>(this), sizeof(format_chunk_t));
    }

    uint32_t get_sample_rate() const { return sample_rate; }
};

template <typename sample_t> class data_chunk_t {
private:
    char                  chunk_id[4]; // data
    uint32_t              chunk_size;
    std::vector<sample_t> data;

public:
    data_chunk_t()
    {
        memcpy(chunk_id, "data", 4);
        chunk_size = 0;
    }

    constexpr uint32_t sample_size()
    {
        return std::tuple_size<sample_t>::value
            * sizeof(std::tuple_element_t<0, sample_t>);
    }

    void update_size() { chunk_size = data.size() * sample_size(); }

    uint32_t get_size()
    {
        update_size();
        return chunk_size;
    }

    void add_sample(const sample_t& sample) { data.push_back(sample); }

    void clear()
    {
        data.clear();
        update_size();
    }

    void append(const std::vector<sample_t>& other)
    {
        data.insert(data.end(), other.begin(), other.end());
    }

    const std::vector<sample_t>& c_samples() const { return data; }

    void dump(std::ostream& stream)
    {
        update_size();
        stream.write(reinterpret_cast<char*>(this),
            sizeof(chunk_id) + sizeof(chunk_size));
        stream.write(reinterpret_cast<char*>(&data[0]), get_size());
    }
};

struct index_data_t {
    size_t      offset;
    size_t      length;
    std::string information;
};

template <uint8_t _num_channels, typename data_type_t> class wav_t {
private:
    using sample_t = std::array<data_type_t, _num_channels>;
    char                                       chunk_id[4]; // 'RIFF'
    uint32_t                                   chunk_size;
    char                                       format_id[4]; // 'WAVE'
    format_chunk_t<_num_channels, data_type_t> format;
    data_chunk_t<sample_t>                     data;

    std::vector<index_data_t> index_data;

public:
    wav_t(uint32_t sample_rate)
        : format(sample_rate)
    {
        memcpy(chunk_id, "RIFF", 4);
        memcpy(format_id, "WAVE", 4);
    }

    float usecs_per_sample() { return format.usec_per_sample(); }

    void update_size()
    {
        static_assert(sizeof(chunk_id) + sizeof(format) + 8 == 36,
            "Something is wrong...");
        chunk_size = sizeof(chunk_id) + sizeof(format) + 8 + data.get_size();
    }

    void add_sample(const sample_t& sample) { data.add_sample(sample); }

    void add_silence(uint32_t seconds)
    {
        for (int i = 0; i < format.get_sample_rate() * seconds; i++) {
            data.add_sample({ 0 });
        }
    }

    const std::vector<sample_t>& c_samples() const { return data.c_samples(); }

    const std::vector<index_data_t>& get_index_data() const
    {
        return index_data;
    }

    void append(const wav_t<_num_channels, data_type_t>& other)
    {
        auto other_data = other.get_index_data();

        for (auto& item : other_data) {
            item.offset += data.c_samples().size();
        }

        data.append(other.c_samples());

        index_data.insert(
            index_data.end(), other_data.begin(), other_data.end());
    }

    void clear()
    {
        data.clear();
        update_size();
    }

    float get_length_seconds() const
    {
        return float(data.c_samples().size()) / format.get_sample_rate();
    }

    void set_full_info(const std::string& info)
    {
        index_data.clear();
        index_data.push_back(index_data_t { 0, data.c_samples().size(), info });
    }

    void dump(std::ostream& stream)
    {
        update_size();
        stream.write(reinterpret_cast<char*>(this),
            sizeof(chunk_id) + sizeof(chunk_size) + sizeof(format_id));
        format.dump(stream);
        data.dump(stream);
    }

    uint32_t get_sample_rate() { return format.get_sample_rate(); }
};

#pragma pack(pop)

#endif
