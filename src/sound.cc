#include <algorithm>
#include <iostream>
#include <vector>

#define USE_OPENAL 0
#define USE_CALLBACK 1
#if USE_OPENAL
#include <al.h>
#include <alc.h>
#else
#include <SDL.h>
#include <queue>
#include <deque>
#endif

#include "sound.h"

namespace nes {

constexpr int SAMPLING_RATE = 44100;
constexpr int MAX_SAMPLE_COUNT = 8 * SAMPLING_RATE / 60;
#if USE_OPENAL
constexpr int BUFFER_COUNT = 32;
#endif

#if USE_OPENAL
static ALCdevice *device = nullptr;
static ALCcontext *context = nullptr;

static ALuint source = 0;
static ALuint buffer_list[BUFFER_COUNT] = {0};
static ALuint *pbuffer = buffer_list;
#else
static SDL_AudioDeviceID audio_device;
//static std::queue<float> sample_queue;
static std::deque<float> sample_queue(1024 * 8, 0);
void fill_audio(void *udata, Uint8 *stream, int len);
#endif

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
    int Count() const { return count_; }

private:
    std::vector<int16_t> data_;
    int count_;
};

static SampleBuffer sample_data;

void InitSound()
{
#if USE_OPENAL
    device = alcOpenDevice(nullptr);
    context = alcCreateContext(device, nullptr);
    alcMakeContextCurrent(context);

    alGenBuffers(BUFFER_COUNT, buffer_list);
    alGenSources(1, &source);
#else
    //Initialize SDL
    //if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
    if(SDL_Init(SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        //return -1;
        return;
    }

    // Audio
    SDL_AudioSpec audio_spec;
    SDL_zero(audio_spec);
    audio_spec.freq = SAMPLING_RATE;
    audio_spec.format = AUDIO_S16SYS;
    audio_spec.channels = 1;
    audio_spec.samples = 1024 / 1;
    audio_spec.callback = nullptr;
    if (USE_CALLBACK)
        audio_spec.callback = fill_audio;
    audio_device = SDL_OpenAudioDevice(nullptr, 0, &audio_spec, nullptr, 0);
#endif

    sample_data.Resize(MAX_SAMPLE_COUNT);
}

void FinishSound()
{
#if USE_OPENAL
    alSourceStop(source);

    alDeleteBuffers(BUFFER_COUNT, buffer_list);
    alDeleteSources(1, &source);

    alcMakeContextCurrent(nullptr);
    alcDestroyContext(context);
    alcCloseDevice(device);
#else
    SDL_CloseAudioDevice(audio_device);
#endif

    sample_data.Clear();
}

static void unqueue_buffer()
{
#if USE_OPENAL
    for (;;) {
        ALuint unqueued_buff[8] = {0};
        ALint processed_count = 0;
        alGetSourceiv(source, AL_BUFFERS_PROCESSED, &processed_count);

        if (processed_count > 0)
            alSourceUnqueueBuffers(source, processed_count, unqueued_buff);
        else
            break;
    }
#else
#endif
}

static void switch_buffer()
{
#if USE_OPENAL
    if (pbuffer == &buffer_list[BUFFER_COUNT - 1])
        pbuffer = buffer_list;
    else
        pbuffer++;
#else
#endif
}

static void queue_buffer(const SampleBuffer &buff)
{
#if USE_OPENAL
    // copy sample buff
    alBufferData(*pbuffer, AL_FORMAT_MONO16, buff.Data(), buff.Size(), SAMPLING_RATE);

    // queue buffer
    alSourceQueueBuffers(source, 1, pbuffer);
#else
    SDL_QueueAudio(audio_device, buff.Data(), buff.Size());
#endif

    switch_buffer();
}

void PushSample(float sample)
{
#if USE_OPENAL
    sample_data.Push(INT16_MAX * sample);
#else
    if (USE_CALLBACK) {
        SDL_LockAudioDevice(audio_device);
        sample_queue.push_back(sample);
        SDL_UnlockAudioDevice(audio_device);
    }
    else {
        sample_data.Push(INT16_MAX * sample * .5);
    }
    //const int16_t data = INT16_MAX * sample * .5;
    //SDL_QueueAudio(audio_device, &data, sizeof(data));
    //SDL_LockAudioDevice(audio_device);
    //SDL_UnlockAudioDevice(audio_device);
#endif
}

void SendSamples()
{
    if (USE_CALLBACK && !USE_OPENAL)
        return;

#if USE_OPENAL
    printf("OpenAL samples: %d\n", sample_data.Count());
#else
    printf("SDL2 samples: %d\n", sample_data.Count());
#endif
    unqueue_buffer();
    queue_buffer(sample_data);

    sample_data.Clear();
}

void PlaySamples()
{
#if USE_OPENAL
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
#else
    SDL_PauseAudioDevice(audio_device, 0);
#endif
}

void PauseSamples()
{
#if USE_OPENAL
    alSourcePause(source);
#else
    SDL_PauseAudioDevice(audio_device, 1);
#endif
}

int GetQueuedSamples()
{
#if USE_OPENAL
    return 0;
#else
    const Uint32 count = SDL_GetQueuedAudioSize(audio_device) / sizeof(int16_t);
    //printf("queued samples: %d\n", count);
    return count;
#endif
}

#if USE_OPENAL
#else
// The audio function callback takes the following parameters:
// stream:  A pointer to the audio buffer to be filled
// len:     The length (in bytes) of the audio buffer
void fill_audio(void *udata, Uint8 *stream, int len)
{
    const int n = sample_queue.size();
    const int N = len / 2;
    int16_t *stream16 = reinterpret_cast<int16_t*>(stream);
    static int16_t sample = 0;

    if (n < N) {
        for (int i = 0; i < N; i++) {
            stream16[i] = sample;
        }
        SDL_PauseAudioDevice(audio_device, 0);
        return;
    }

    for (int i = 0; i < N; i++) {
        if (!sample_queue.empty()) {
            sample = sample_queue.front() * INT16_MAX * .5;
            sample_queue.pop_front();
        }
        else {
            //printf("i: %d, size: %d, N: %d\n", i, n, N);
        }

        stream16[i] = sample;
    }
}
#endif

} // namespace
