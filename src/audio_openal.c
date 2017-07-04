#include <stdbool.h>
#include <stdio.h>

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/efx.h>

#include <hap.h>


typedef struct ALState ALState;
typedef enum EffectsType EffectsType;


enum EffectsType {
	EFFECTS_TYPE_NONE,
	EFFECTS_TYPE_EFX,
};


struct ALState {
	ALCdevice   *device;
	ALCcontext  *context;

	EffectsType effectsType;
	ALCint      totalAuxiliarySends;
};


bool al_check_error(char *message) {
	if (alGetError() == AL_NO_ERROR) return false;

	fprintf(stderr, "[OpenAL] %s\n", message);
	return true;
}


void* create(HAPEngine *engine) {
	ALState *state = calloc(1, sizeof(ALState));

	if (state == NULL) {
		fprintf(stderr, "Failed to acquire memory for OpenAL module.\n");
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
		fprintf(stderr, "Error: Could not open an audio device.");
		return;
	}

	context = alcCreateContext(device, NULL);

	if (!alcMakeContextCurrent(context)) {
		fprintf(stderr, "[OpenAL] Unable to make context current.\n");
		return;
	}

	alListenerfv(AL_GAIN, &gain);

	if (al_check_error("Can not adjust volume level for audio device.")) return;

	if (alcIsExtensionPresent(device, "ALC_EXT_EFX") != AL_FALSE) {
		effectsType = EFFECTS_TYPE_EFX;
	}

	// TODO:
	//   Apple built their own effects type, so we should imlpement that
	//   someday in the probably distant future. O_o

	switch (effectsType) {
	case EFFECTS_TYPE_EFX:
		alcGetIntegerv(
		    device,
		    ALC_MAX_AUXILIARY_SENDS, 1,
		    &(*audio).totalAuxiliarySends
		);

		if (al_check_error("Failed to get the number of auxiliary sends available.")) return;

		// Default of a 1:1 mapping of kilometers for effects processing that
		// needs to know units (EG, air absorption)
		alListenerf(AL_METERS_PER_UNIT, 1.0);

		if (al_check_error("Failed to set map units for EFX.")) return;

		break;

	default:
		(*audio).totalAuxiliarySends = 0;
	}

	// 343 kilometers per second (this is the default)
	alSpeedOfSound(343.3);

	if (al_check_error("Failed to set the speed of sound.")) return;

	(*audio).effectsType = effectsType;
	(*audio).device = device;
	(*audio).context = context;
}


HAPTime update(HAPEngine *engine, void *state) {
	return -1;
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
