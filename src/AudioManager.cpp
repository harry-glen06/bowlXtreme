#include "AudioManager.h"
#include <cstdlib>
#include <algorithm>
#include <cstring>

// Try loading a sound from multiple candidate paths; returns true on success.
static bool trySoundLoad(Sound& out, const char* const* paths, int count) {
    for (int i = 0; i < count; i++) {
        out = LoadSound(paths[i]);
        if (out.stream.buffer != nullptr) return true;
    }
    return false;
}
static bool tryMusicLoad(Music& out, const char* const* paths, int count) {
    for (int i = 0; i < count; i++) {
        out = LoadMusicStream(paths[i]);
        if (out.stream.buffer != nullptr) return true;
    }
    return false;
}

AudioManager::AudioManager() {
    InitAudioDevice();
    loadSounds();
}

AudioManager::~AudioManager() {
    // Unload sounds
    if (ballRollLoaded)     UnloadSound(ballRollSound);
    for (int i = 0; i < 5; i++) if (pinHitLoaded[i])    UnloadSound(pinHitSounds[i]);
    if (pinCollisionLoaded) UnloadSound(pinCollisionSound);
    if (explodingPinLoaded) UnloadSound(explodingPinSound);
    if (strikeCheerLoaded)  UnloadSound(strikeCheerSound);
    for (int i = 0; i < 5; i++) if (bossLaughLoaded[i]) UnloadSound(bossLaughSounds[i]);
    if (menuMusic1Loaded)   UnloadMusicStream(menuMusic1);
    if (menuMusic2Loaded)   UnloadMusicStream(menuMusic2);
    if (gameMusicLoaded)    UnloadMusicStream(gameMusic);
    CloseAudioDevice();
}

void AudioManager::loadSounds() {
    const char* rollPaths[]  = {"assets/ball_roll.wav"};
    ballRollLoaded = trySoundLoad(ballRollSound, rollPaths, 1);

    const char* hitPaths[5][1] = {
        {"assets/pin_hit1.wav"}, {"assets/pin_hit2.wav"}, {"assets/pin_hit3.wav"},
        {"assets/pin_hit4.wav"}, {"assets/pin_hit5.wav"}
    };
    for (int i = 0; i < 5; i++)
        pinHitLoaded[i] = trySoundLoad(pinHitSounds[i], hitPaths[i], 1);

    const char* colPaths[] = {"assets/pin_collision.wav"};
    pinCollisionLoaded = trySoundLoad(pinCollisionSound, colPaths, 1);

    const char* expPaths[] = {"assets/exploding_pin.wav"};
    explodingPinLoaded = trySoundLoad(explodingPinSound, expPaths, 1);

    const char* cheerPaths[] = {"assets/cheering.wav"};
    strikeCheerLoaded = trySoundLoad(strikeCheerSound, cheerPaths, 1);

    const char* laughPaths[5][2] = {
        {"assets/EnchantressLaugh.wav", nullptr},
        {"assets/OverlordLaugh.wav",    nullptr},
        {"assets/TwinsLaugh.wav",       nullptr},
        {"assets/EmperorLaugh.wav",     "assets/EmeperorLaugh.wav"},
        {"assets/MonsterLaugh.wav",     nullptr}
    };
    for (int i = 0; i < 5; i++) {
        int cnt = (laughPaths[i][1] != nullptr) ? 2 : 1;
        bossLaughLoaded[i] = trySoundLoad(bossLaughSounds[i], laughPaths[i], cnt);
    }

    // Music
    const char* m1[]  = {"assets/music1.wav"};
    const char* m2[]  = {"assets/music2.wav"};
    const char* gm[]  = {"assets/background_music.flac"};
    menuMusic1Loaded = tryMusicLoad(menuMusic1, m1, 1);
    menuMusic2Loaded = tryMusicLoad(menuMusic2, m2, 1);
    gameMusicLoaded  = tryMusicLoad(gameMusic,  gm, 1);

    if (menuMusic1Loaded) { menuMusic1.looping = false; SetMusicVolume(menuMusic1, musicVolVol / 100.f); }
    if (menuMusic2Loaded) { menuMusic2.looping = false; SetMusicVolume(menuMusic2, musicVolVol / 100.f); }
    if (gameMusicLoaded)  { gameMusic.looping = true;   SetMusicVolume(gameMusic,  musicVolVol / 100.f); }

    soundsLoaded = true;
}

void AudioManager::update() {
    if (menuMusic1Loaded) UpdateMusicStream(menuMusic1);
    if (menuMusic2Loaded) UpdateMusicStream(menuMusic2);
    if (gameMusicLoaded)  UpdateMusicStream(gameMusic);
}

void AudioManager::playMenuMusic() {
    if (gameMusicLoaded && IsMusicStreamPlaying(gameMusic)) StopMusicStream(gameMusic);

    if ((menuMusic1Loaded && IsMusicStreamPlaying(menuMusic1)) ||
        (menuMusic2Loaded && IsMusicStreamPlaying(menuMusic2))) {
        return;
    }

    if (currentTrack != 1) {
        if (menuMusic1Loaded) { PlayMusicStream(menuMusic1); currentTrack = 1; }
    } else {
        if (menuMusic2Loaded) { PlayMusicStream(menuMusic2); currentTrack = 2; }
    }
}

void AudioManager::playBackgroundMusic() {
    if (menuMusic1Loaded) StopMusicStream(menuMusic1);
    if (menuMusic2Loaded) StopMusicStream(menuMusic2);
    if (gameMusicLoaded && !IsMusicStreamPlaying(gameMusic))
        PlayMusicStream(gameMusic);
}

void AudioManager::stopBackgroundMusic() {
    if (menuMusic1Loaded) StopMusicStream(menuMusic1);
    if (menuMusic2Loaded) StopMusicStream(menuMusic2);
    if (gameMusicLoaded)  StopMusicStream(gameMusic);
    currentTrack = 0;
}

void AudioManager::setMusicVolume(float volume) {
    musicVolVol = volume;
    float v = volume / 100.f;
    if (menuMusic1Loaded) SetMusicVolume(menuMusic1, v);
    if (menuMusic2Loaded) SetMusicVolume(menuMusic2, v);
    if (gameMusicLoaded)  SetMusicVolume(gameMusic,  v);
}

void AudioManager::setSoundVolume(float volume) {
    sfxVolume = std::clamp(volume, 0.f, 100.f);
}

void AudioManager::playRandomPinHit(float volume) {
    if (!soundsLoaded) return;
    int idx = rand() % 5;
    if (pinHitLoaded[idx] && !IsSoundPlaying(pinHitSounds[idx])) {
        SetSoundVolume(pinHitSounds[idx], volume * (sfxVolume / 100.f) / 100.f);
        PlaySound(pinHitSounds[idx]);
    }
}

void AudioManager::playPinCollision(float volume) {
    if (!soundsLoaded || !pinCollisionLoaded) return;
    if (!IsSoundPlaying(pinCollisionSound)) {
        SetSoundVolume(pinCollisionSound, volume * (sfxVolume / 100.f) / 100.f);
        PlaySound(pinCollisionSound);
    }
}

void AudioManager::playExplodingPin(float volume) {
    if (!soundsLoaded || !explodingPinLoaded) return;
    if (!IsSoundPlaying(explodingPinSound)) {
        SetSoundVolume(explodingPinSound, volume * (sfxVolume / 100.f) / 100.f);
        PlaySound(explodingPinSound);
    }
}

void AudioManager::playStrikeCheer(float volume) {
    if (!soundsLoaded || !strikeCheerLoaded) return;
    if (!IsSoundPlaying(strikeCheerSound)) {
        SetSoundVolume(strikeCheerSound, volume * (sfxVolume / 100.f) / 100.f);
        PlaySound(strikeCheerSound);
    }
}

void AudioManager::playBossLaugh(BossLaughType boss, float volume) {
    if (!soundsLoaded) return;
    int idx = (int)boss;
    if (!bossLaughLoaded[idx]) return;
    StopSound(bossLaughSounds[idx]);
    SetSoundVolume(bossLaughSounds[idx], volume * (sfxVolume / 100.f) / 100.f);
    PlaySound(bossLaughSounds[idx]);
}

void AudioManager::startBallRoll() {
    if (!soundsLoaded || !ballRollLoaded) return;
    if (!IsSoundPlaying(ballRollSound)) {
        SetSoundVolume(ballRollSound, 0.95f * (sfxVolume / 100.f));
        PlaySound(ballRollSound);
        ballRolling = true;
    }
}

void AudioManager::stopBallRoll() {
    if (ballRolling && ballRollLoaded) {
        StopSound(ballRollSound);
        ballRolling = false;
    }
}
