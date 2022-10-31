#include <algorithm>
#include <vector>
#include <al.h>
#include <alc.h>

#include "sound.h"

namespace nes {

constexpr int SAMPLING_RATE = 44100;
constexpr int MAX_SAMPLE_COUNT = 8 * SAMPLING_RATE / 60;
constexpr int BUFFER_COUNT = 32;

static ALCdevice *device = nullptr;
static ALCcontext *context = nullptr;

static ALuint source = 0;
static ALuint buffer_list[BUFFER_COUNT] = {0};
static ALuint *pbuffer = buffer_list;

class SampleBuffer {
public:
    SampleBuffer() {}
    ~SampleBuffer() {}

    void Resize(int size)
    {
        data_.resize(size, 0);
        count_ = 0;
    }

    void Push(int32_t sample)
    {
        data_[count_++] = sample;
        if (count_ == data_.size())
            count_ = 0;
    }

    void Clear()
    {
        std::fill(data_.begin(), data_.end(), 0);
        count_ = 0;
    }

    const int16_t *Data() const { return &data_[0]; }
    int Size() const { return count_ * sizeof(data_[0]); }

private:
    std::vector<int16_t> data_;
    int count_;
};

static SampleBuffer sample_data;

void InitSound()
{
    device = alcOpenDevice(nullptr);
    context = alcCreateContext(device, nullptr);
    alcMakeContextCurrent(context);

    alGenBuffers(BUFFER_COUNT, buffer_list);
    alGenSources(1, &source);

    sample_data.Resize(MAX_SAMPLE_COUNT);
}

void FinishSound()
{
    alSourceStop(source);

    alDeleteBuffers(BUFFER_COUNT, buffer_list);
    alDeleteSources(1, &source);

    alcMakeContextCurrent(nullptr);
    alcDestroyContext(context);
    alcCloseDevice(device);

    sample_data.Clear();
}

static void unqueue_buffer()
{
    for (;;) {
        ALuint unqueued_buff[8] = {0};
        ALint processed_count = 0;
        alGetSourceiv(source, AL_BUFFERS_PROCESSED, &processed_count);

        if (processed_count > 0)
            alSourceUnqueueBuffers(source, processed_count, unqueued_buff);
        else
            break;
    }
}

static void switch_buffer()
{
    if (pbuffer == &buffer_list[BUFFER_COUNT - 1])
        pbuffer = buffer_list;
    else
        pbuffer++;
}

static void queue_buffer(const SampleBuffer &buff)
{
    // copy sample buff
    alBufferData(*pbuffer, AL_FORMAT_MONO16, buff.Data(), buff.Size(), SAMPLING_RATE);

    // queue buffer
    alSourceQueueBuffers(source, 1, pbuffer);

    switch_buffer();
}

void PushSample(float sample)
{
    sample_data.Push(INT16_MAX * sample);
}

void SendSamples()
{
    unqueue_buffer();
    queue_buffer(sample_data);

    sample_data.Clear();
}

void PlaySamples()
{
    ALint queued_count = 0;
    ALint stat = 0;

    alGetSourcei(source, AL_BUFFERS_QUEUED, &queued_count);
    alGetSourcei(source, AL_SOURCE_STATE, &stat);

    if (queued_count > 0 && stat != AL_PLAYING) {
        alSourcei(source, AL_LOOPING, AL_FALSE);
        alSourcePlay(source);
        if (0)
            printf("Sound Play: Queued Count: %d\n", queued_count);
    }
}

} // namespace
