#include <stdlib.h>
#include <math.h>
#include <al.h>
#include <alc.h>

static const int SAMPLINGRATE = 44100;
static ALCdevice *device = NULL;
static ALCcontext *context = NULL;
static signed short *wav_data = NULL;

static ALuint buffer = 0;
static ALuint source = 0;

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

    free(wav_data);
    alDeleteBuffers(1, &buffer);
    alDeleteSources(1, &source);

    alcMakeContextCurrent(NULL);
    alcDestroyContext(context);
    alcCloseDevice(device);
}

void play_sound(void)
{
    int i;

    wav_data = malloc(sizeof(signed short) * SAMPLINGRATE);
    for (i = 0; i < SAMPLINGRATE; i++)
        wav_data[i] = 32767 * sin(2 * M_PI*i * 440 / SAMPLINGRATE);

    alBufferData(buffer, AL_FORMAT_MONO16, &wav_data[0],
            SAMPLINGRATE * sizeof(signed short), SAMPLINGRATE);

    /* attach first set of buffers using queuing mechanism */
    alSourceQueueBuffers(source, 1, &buffer);
    /* turn off looping */
    alSourcei(source, AL_LOOPING, AL_FALSE);

    alSourcePlay(source);
}
