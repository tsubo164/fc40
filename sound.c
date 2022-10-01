#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <al.h>
#include <alc.h>

#include "sound.h"

static const int SAMPLINGRATE = 44100;
static ALCdevice *device = NULL;
static ALCcontext *context = NULL;

static ALuint buffer = 0;
static ALuint source = 0;

static int16_t *sample_buf = NULL;
static int32_t sample_count = 0;

void init_sound(void)
{
    device = alcOpenDevice(NULL);
    context = alcCreateContext(device, NULL);
    alcMakeContextCurrent(context);

    alGenBuffers(1, &buffer);
    alGenSources(1, &source);
}

void finish_sound(void)
{
    alSourceStop(source);

    alDeleteBuffers(1, &buffer);
    alDeleteSources(1, &source);

    alcMakeContextCurrent(NULL);
    alcDestroyContext(context);
    alcCloseDevice(device);

    free(sample_buf);
}

void play_sound(void)
{
    sample_buf = calloc(SAMPLINGRATE, sizeof(int16_t));
}

void send_samples(void)
{
}

static void unqueue_buffer(void)
{
    for (;;) {
        ALuint unqueued_buff = 0;
        ALint processed_count = 0;
        alGetSourceiv(source, AL_BUFFERS_PROCESSED, &processed_count);

        if (processed_count)
            alSourceUnqueueBuffers(source, 1, &unqueued_buff);
        else
            break;
    }
}

#define MAX_SAMPLE_COUNT (44100)
//#define MAX_SAMPLE_COUNT (44100 / 2 / 2 / 2 / 2)
static void queue_buffer(void)
{
    alBufferData(buffer, AL_FORMAT_MONO16, &sample_buf[0],
            MAX_SAMPLE_COUNT * sizeof(signed short), SAMPLINGRATE);

    /* attach first set of buffers using queuing mechanism */
    alSourceQueueBuffers(source, 1, &buffer);
    /* turn off looping */
    alSourcei(source, AL_LOOPING, AL_FALSE);
}

void push_sample(int16_t sample)
{
    static int64_t i = 0;
    sample_buf[sample_count++] = sample;

    if (sample_count == MAX_SAMPLE_COUNT) {
        sample_count = 0;

        printf("i: %lld sample: %hd\n", i++, sample);
        unqueue_buffer();

        queue_buffer();

        memset(sample_buf, 0, sizeof(signed short) * SAMPLINGRATE);


        /* play source */
        ALint queued_count = 0;
        ALint stat = 0;
        alGetSourcei(source, AL_BUFFERS_QUEUED, &queued_count);
        alGetSourcei(source, AL_SOURCE_STATE, &stat);
        if (queued_count > 1 && stat != AL_PLAYING)
            alSourcePlay(source);
    }
}
