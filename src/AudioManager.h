#pragma once
#include "raylib.h"
#include <memory>

class AudioManager {
public:
    enum class BossLaughType {
        Enchantress,
        Overlord,
        Twins,
        Emperor,
        Monster
    };

    AudioManager();
    ~AudioManager();

    void loadSounds();

    void playRandomPinHit(float volume = 70.0f);
    void playPinCollision(float volume = 50.0f);
    void playExplodingPin(float volume = 75.0f);
    void playStrikeCheer(float volume = 85.0f);
    void playBossLaugh(BossLaughType boss, float volume = 70.0f);
    void startBallRoll();
    void stopBallRoll();

    void playMenuMusic();
    void playBackgroundMusic();
    void stopBackgroundMusic();
    void setMusicVolume(float volume);
    void setSoundVolume(float volume);

    // Must be called every frame for streaming music
    void update();

    bool areSoundsLoaded() const { return soundsLoaded; }
    bool isBallRolling()   const { return ballRolling; }

private:
    Sound ballRollSound{};
    Sound pinHitSounds[5]{};
    Sound pinCollisionSound{};
    Sound explodingPinSound{};
    Sound strikeCheerSound{};
    Sound bossLaughSounds[5]{}; // Enchantress, Overlord, Twins, Emperor, Monster

    Music menuMusic1{};
    Music menuMusic2{};
    Music gameMusic{};

    int   currentTrack = 0;
    bool  soundsLoaded = false;
    bool  ballRolling  = false;
    float sfxVolume    = 70.0f;  // 0-100
    float musicVolVol  = 30.0f;  // 0-100

    bool pinHitLoaded[5]      = {};
    bool bossLaughLoaded[5]   = {};
    bool ballRollLoaded       = false;
    bool pinCollisionLoaded   = false;
    bool explodingPinLoaded   = false;
    bool strikeCheerLoaded    = false;
    bool menuMusic1Loaded     = false;
    bool menuMusic2Loaded     = false;
    bool gameMusicLoaded      = false;
};
