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
static ALuint buffer1 = 0;
static ALuint buffer_current = 0;
static ALuint source = 0;

static int16_t *sample_buf = NULL;
static int16_t *sample_buf1 = NULL;
static int16_t *buf_filling = NULL;
static int16_t *buf_filled = NULL;
static int32_t sample_count = 0;

void init_sound(void)
{
    device = alcOpenDevice(NULL);
    context = alcCreateContext(device, NULL);
    alcMakeContextCurrent(context);

    alGenBuffers(1, &buffer);
    alGenBuffers(1, &buffer1);
    alGenSources(1, &source);
}

void finish_sound(void)
{
    alSourceStop(source);

    alDeleteBuffers(1, &buffer);
    alDeleteBuffers(1, &buffer1);
    alDeleteSources(1, &source);

    alcMakeContextCurrent(NULL);
    alcDestroyContext(context);
    alcCloseDevice(device);

    free(sample_buf);
    free(sample_buf1);
}

void play_sound(void)
{
    sample_buf = calloc(SAMPLINGRATE, sizeof(int16_t));
    sample_buf1 = calloc(SAMPLINGRATE, sizeof(int16_t));

    buf_filling = sample_buf;
    buf_filled = sample_buf1;
}

void send_samples(void)
{
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

//#define MAX_SAMPLE_COUNT (44100)
#define AUDIO_FRAME_DELAY 5
#define MAX_SAMPLE_COUNT (AUDIO_FRAME_DELAY * 44100 / 60)
static void queue_buffer(int16_t *buf, int count)
{
    buffer_current = (buf == sample_buf) ? buffer : buffer1;

    alBufferData(buffer_current, AL_FORMAT_MONO16, &buf[0],
            count * sizeof(*buf), SAMPLINGRATE);

    /* attach first set of buffers using queuing mechanism */
    alSourceQueueBuffers(source, 1, &buffer_current);
}

static void swap_buffers(int16_t **a, int16_t **b)
{
    int16_t *tmp = *a;
    *a = *b;
    *b = tmp;
}

void push_sample(int16_t sample)
{
    buf_filling[sample_count++] = sample;

    if (sample_count == MAX_SAMPLE_COUNT - 1) {
        play_samples__();
    }

    if (sample_count == MAX_SAMPLE_COUNT) {
        sample_count = 0;

        unqueue_buffer();

        queue_buffer(buf_filling, MAX_SAMPLE_COUNT);

        swap_buffers(&buf_filling, &buf_filled);

        memset(buf_filling, 0, sizeof(signed short) * SAMPLINGRATE);
    }
}

void push_sample__(int16_t sample)
{
    buf_filling[sample_count++] = sample;

    if (sample_count == MAX_SAMPLE_COUNT)
        sample_count = 0;
}

void send_samples__(void)
{
    unqueue_buffer();

    queue_buffer(buf_filling, sample_count);

    swap_buffers(&buf_filling, &buf_filled);

    memset(buf_filling, 0, sizeof(signed short) * SAMPLINGRATE);

    sample_count = 0;
}

void play_samples__(void)
{
    ALint queued_count = 0;
    ALint stat = 0;

    alGetSourcei(source, AL_BUFFERS_QUEUED, &queued_count);
    alGetSourcei(source, AL_SOURCE_STATE, &stat);

    if (queued_count > 0 && stat != AL_PLAYING) {
        alSourcei(source, AL_LOOPING, AL_FALSE);
        alSourcePlay(source);
        printf("PLAY!!!!!!!!!!\n");
    }
}
