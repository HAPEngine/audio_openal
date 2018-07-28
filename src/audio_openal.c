/** Modifies the linker so that it uses proper symbol names in MSVC. **/
#ifdef OS_Windows
#pragma comment(linker, "/EXPORT:create=create")
#pragma comment(linker, "/EXPORT:load=load")
#pragma comment(linker, "/EXPORT:update=update")
#pragma comment(linker, "/EXPORT:unload=unload")
#pragma comment(linker, "/EXPORT:destroy=destroy")
#endif

#include <stdbool.h>
#include <stdio.h>

#include <al.h>
#include <alc.h>

#ifdef OS_Darwin
#else
#include <efx.h>
#endif

#include <hap.h>


typedef struct ALState ALState;


typedef enum EffectsType {
    EFFECTS_TYPE_NONE,
    EFFECTS_TYPE_EFX,
} EffectsType;


struct ALState {
    ALCdevice   *device;
    ALCcontext  *context;

    EffectsType effectsType;
    ALCint      totalAuxiliarySends;
};


bool al_check_error(HAPEngine *engine, char *message) {
    if (alGetError() == AL_NO_ERROR) return false;

    (*engine).log_error(engine, "[OpenAL] %s\n", message);
    return true;
}


void* create(HAPEngine *engine, HAPConfigurationSection *configuration) {
    ALState *state = calloc(1, sizeof(ALState));

    if (state == NULL) {
        (*engine).log_error(engine, "Failed to acquire memory for OpenAL module.\n");
        return NULL;
    }

    return (void*) state;
}


void load(HAPEngine *engine, void *state, char *identifier) {
    ALCcontext *context;
    ALCdevice *device;
    ALState *audio;

    EffectsType effectsType = EFFECTS_TYPE_NONE;

    // Default to no gain to avoid sudden loud pops
    float gain = 0.0f;

    audio = (ALState*) state;

    if (audio == NULL) return;

    // Reset the error state before continuing
    alGetError();

    device = alcOpenDevice(NULL);

    if (device == NULL) {
        (*engine).log_error(engine, "Error: Could not open an audio device.");
        return;
    }

    context = alcCreateContext(device, NULL);

    if (!alcMakeContextCurrent(context)) {
        (*engine).log_error(engine, "[OpenAL] Unable to make context current.\n");
        return;
    }

    alListenerfv(AL_GAIN, &gain);

    if (al_check_error(engine, "Can not adjust volume level for audio device.")) return;

    if (alcIsExtensionPresent(device, "ALC_EXT_EFX") != AL_FALSE) {
        effectsType = EFFECTS_TYPE_EFX;
    }

    // TODO:
    //   Apple built their own effects type, so we should imlpement that
    //   someday in the probably distant future. O_o

    switch (effectsType) {
    /*** TODO: Apple effects support **/
    case EFFECTS_TYPE_EFX:
#ifdef AL_METERS_PER_UNIT
        alcGetIntegerv(
            device,
            ALC_MAX_AUXILIARY_SENDS, 1,
            &(*audio).totalAuxiliarySends
        );

        if (al_check_error(engine, "Failed to get the number of auxiliary sends available.")) return;

        // Default of a 1:1 mapping of kilometers for effects processing that
        // needs to know units (EG, air absorption)
        alListenerf(AL_METERS_PER_UNIT, 1.0);

        if (al_check_error(engine, "Failed to set map units for EFX.")) return;
#endif

        break;

    default:
        (*audio).totalAuxiliarySends = 0;
    }

    // 343 kilometers per second (this is the default)
    alSpeedOfSound(343.3);

    if (al_check_error(engine, "Failed to set the speed of sound.")) return;

    (*audio).effectsType = effectsType;
    (*audio).device = device;
    (*audio).context = context;
}


HAPTime update(HAPEngine *engine, void *state) {
    return 0;
}


void unload(HAPEngine *engine, void *state) {
    ALState *audio = (ALState*) state;

    if ((*audio).context != NULL) {
        if ((*audio).device != NULL) {
            alcCloseDevice((*audio).device);
            (*audio).device = NULL;
        }

        alcDestroyContext((*audio).context);
        (*audio).context = NULL;
    }
}


void destroy(HAPEngine *engine, void *state) {
    if (state != NULL) free(state);
}
