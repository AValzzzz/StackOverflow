#ifndef AUDIO_H
#define AUDIO_H

void audio_loadAll(void);
void audio_unloadAll(void);
void audio_playPlace(void); 
void audio_playSlide(void);
void audio_playCombo(float pitch); 
void audio_playCrash(void); 
void audio_playShuffle(void);
void audio_playShopBuy(void);
void audio_playRoundClear(void);
void audio_playUiClick(void);
void audio_playDeny(void);
void audio_playGlitch(void);

void audio_updateMusic(void);
void audio_setMusicVolume(float volume);

#endif
