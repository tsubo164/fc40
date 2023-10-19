#include <algorithm>
#include <iostream>
#include <cstdint>
#include <vector>
#include <limits>

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

static std::vector<int16_t> sample_data;

void InitSound()
{
    device = alcOpenDevice(nullptr);
    context = alcCreateContext(device, nullptr);
    alcMakeContextCurrent(context);

    alGenBuffers(BUFFER_COUNT, buffer_list);
    alGenSources(1, &source);

    sample_data.reserve(MAX_SAMPLE_COUNT);
}

void FinishSound()
{
    alSourceStop(source);

    alDeleteBuffers(BUFFER_COUNT, buffer_list);
    alDeleteSources(1, &source);

    alcMakeContextCurrent(nullptr);
    alcDestroyContext(context);
    alcCloseDevice(device);

    sample_data.clear();
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

int GetQueuedSampleCount()
{
    int queued = 0;
    int processed = 0;
    alGetSourcei(source, AL_BUFFERS_QUEUED, &queued);
    alGetSourcei(source, AL_BUFFERS_PROCESSED, &processed);

    const int unplayed = queued - processed;

    if (unplayed == 0) {
        printf("underflow!! queued: %d, processed: %d, unplayed: %d\n",
                queued, processed, unplayed);
    }

    int index = pbuffer - buffer_list;
    int total_count = 0;

    for (int i = 0; i < unplayed; i++) {
        const int buffer_id = buffer_list[index];

        ALint bytes = 0;
        alGetBufferi(buffer_id, AL_SIZE, &bytes);

        const int count = bytes / sizeof(int16_t);
        total_count += count;
        index = (index - 1 + BUFFER_COUNT) % BUFFER_COUNT;
    }
    if (0)
        printf("total count: %d\n", total_count);

    return total_count;
}

static void queue_buffer(const std::vector<int16_t> &buff)
{
    // copy sample buff
    alBufferData(*pbuffer, AL_FORMAT_MONO16,
            buff.data(), buff.size() * sizeof(buff[0]), SAMPLING_RATE);

    // queue buffer
    alSourceQueueBuffers(source, 1, pbuffer);

    switch_buffer();
}

void PushSample(float sample)
{
    const int value = std::numeric_limits<int16_t>::max() * sample;
    sample_data.push_back(value);
}

void SendSamples()
{
    if (0)
        printf("samples: %lu\n", sample_data.size());

    unqueue_buffer();
    queue_buffer(sample_data);

    sample_data.clear();
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

void PauseSamples()
{
    alSourcePause(source);
}

} // namespace
