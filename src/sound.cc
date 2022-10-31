#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <al.h>
#include <alc.h>

#include "sound.h"

#define MAX_SAMPLE_COUNT (8 * 44100 / 60)
#define BUFFER_COUNT 32

static const int SAMPLINGRATE = 44100;
static ALCdevice *device = NULL;
static ALCcontext *context = NULL;

static ALuint source = 0;
static ALuint buffer_list[BUFFER_COUNT] = {0};
static ALuint *pbuffer = buffer_list;

static int16_t *sample_data = NULL;
static int32_t sample_count = 0;

void init_sound(void)
{
    device = alcOpenDevice(NULL);
    context = alcCreateContext(device, NULL);
    alcMakeContextCurrent(context);

    alGenBuffers(BUFFER_COUNT, buffer_list);
    alGenSources(1, &source);

    sample_data = (int16_t*) calloc(SAMPLINGRATE, sizeof(int16_t));
}

void finish_sound(void)
{
    alSourceStop(source);

    alDeleteBuffers(BUFFER_COUNT, buffer_list);
    alDeleteSources(1, &source);

    alcMakeContextCurrent(NULL);
    alcDestroyContext(context);
    alcCloseDevice(device);

    free(sample_data);
    sample_data = NULL;
    sample_count = 0;
}

static void unqueue_buffer(void)
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

static void switch_buffer(void)
{
    if (pbuffer == &buffer_list[BUFFER_COUNT - 1])
        pbuffer = buffer_list;
    else
        pbuffer++;
}

static void clear_buffer(int16_t *data, int count)
{
    memset(data, 0, sizeof(*data) * count);
}

static void queue_buffer(int16_t *data, int count)
{
    /* copy sample data */
    alBufferData(*pbuffer, AL_FORMAT_MONO16,
            data, count * sizeof(*data), SAMPLINGRATE);

    /* queue buffer */
    alSourceQueueBuffers(source, 1, pbuffer);

    switch_buffer();
}

void push_sample(float sample)
{
    sample_data[sample_count++] = INT16_MAX * sample;

    if (sample_count == MAX_SAMPLE_COUNT)
        sample_count = 0;
}

void send_samples(void)
{
    unqueue_buffer();
    queue_buffer(sample_data, sample_count);
    clear_buffer(sample_data, sample_count);

    sample_count = 0;
}

void play_samples(void)
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
