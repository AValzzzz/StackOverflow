#include "audio.h"

#include <math.h>
#include <stdlib.h>

#include "raylib.h"

#define PLACE_COUNT 3
#define SLIDE_COUNT 3
#define SYNTH_SAMPLE_RATE 44100

static Sound g_place[PLACE_COUNT];
static Sound g_slide[SLIDE_COUNT];
static Sound g_combo;
static Sound g_crash;
static Sound g_shuffle;
static Sound g_shopBuy;
static Sound g_roundClear;

static Sound synthComboChime(void)
{
    const float noteDuration = 0.09f;
    int perNote = (int)(noteDuration * SYNTH_SAMPLE_RATE);
    int frameCount = perNote * 2;
    short *samples = malloc(sizeof(short) * (size_t)frameCount);

    const float freqs[2] = { 660.0f, 990.0f }; 
    float phase = 0.0f;
    for (int i = 0; i < frameCount; i++)
    {
        int noteIndex = (i < perNote) ? 0 : 1;
        phase += freqs[noteIndex] / SYNTH_SAMPLE_RATE;
        float localT = (float)(i % perNote) / (float)perNote;
        float envelope = sinf(PI * localT); 
        float sample = sinf(2.0f * PI * phase) * envelope * 0.55f;
        samples[i] = (short)(sample * 32000.0f);
    }

    Wave wave = { (unsigned int)frameCount, SYNTH_SAMPLE_RATE, 16, 1, samples };
    Sound snd = LoadSoundFromWave(wave);
    free(samples);
    return snd;
}

static Sound synthCrashStinger(void)
{
    const float duration = 0.4f;
    int frameCount = (int)(duration * SYNTH_SAMPLE_RATE);
    short *samples = malloc(sizeof(short) * (size_t)frameCount);

    float phase = 0.0f;
    for (int i = 0; i < frameCount; i++)
    {
        float t = (float)i / (float)frameCount;
        float freq = 340.0f - 260.0f * t;
        phase += freq / SYNTH_SAMPLE_RATE;
        float square = (sinf(2.0f * PI * phase) >= 0.0f) ? 1.0f : -1.0f;
        float noise = (float)GetRandomValue(-1000, 1000) / 1000.0f;
        float mixed = square * 0.7f + noise * 0.3f;
        float envelope = powf(1.0f - t, 1.6f);
        float sample = mixed * envelope * 0.6f;
        samples[i] = (short)(sample * 32000.0f);
    }

    Wave wave = { (unsigned int)frameCount, SYNTH_SAMPLE_RATE, 16, 1, samples };
    Sound snd = LoadSoundFromWave(wave);
    free(samples);
    return snd;
}

void audio_loadAll(void)
{
    InitAudioDevice();
    g_place[0] = LoadSound("assets/audio/cardPlace1.ogg");
    g_place[1] = LoadSound("assets/audio/cardPlace2.ogg");
    g_place[2] = LoadSound("assets/audio/cardPlace3.ogg");
    g_slide[0] = LoadSound("assets/audio/cardSlide1.ogg");
    g_slide[1] = LoadSound("assets/audio/cardSlide2.ogg");
    g_slide[2] = LoadSound("assets/audio/cardSlide3.ogg");
    g_combo = synthComboChime();
    g_crash = synthCrashStinger();
    g_shuffle = LoadSound("assets/audio/cardShuffle.ogg");
    g_shopBuy = LoadSound("assets/audio/shopBuy.ogg");
    g_roundClear = LoadSound("assets/audio/roundClear.ogg");
}

void audio_unloadAll(void)
{
    for (int i = 0; i < PLACE_COUNT; i++) UnloadSound(g_place[i]);
    for (int i = 0; i < SLIDE_COUNT; i++) UnloadSound(g_slide[i]);
    UnloadSound(g_combo);
    UnloadSound(g_crash);
    UnloadSound(g_shuffle);
    UnloadSound(g_shopBuy);
    UnloadSound(g_roundClear);
    CloseAudioDevice();
}

void audio_playPlace(void) { PlaySound(g_place[GetRandomValue(0, PLACE_COUNT - 1)]); }
void audio_playSlide(void) { PlaySound(g_slide[GetRandomValue(0, SLIDE_COUNT - 1)]); }

void audio_playCombo(float pitch)
{
    SetSoundPitch(g_combo, pitch);
    PlaySound(g_combo);
}

void audio_playCrash(void) { PlaySound(g_crash); }
void audio_playShuffle(void) { PlaySound(g_shuffle); }
void audio_playShopBuy(void) { PlaySound(g_shopBuy); }
void audio_playRoundClear(void) { PlaySound(g_roundClear); }
