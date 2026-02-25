#include "AudioManager.h"
#include <cstdlib>
#include <algorithm>
#include <initializer_list>

namespace {
bool loadBufferFromCandidates(sf::SoundBuffer& buffer, std::initializer_list<const char*> paths) {
    for (const char* path : paths) {
        if (buffer.loadFromFile(path)) return true;
    }
    return false;
}
}

AudioManager::AudioManager() {
    loadSounds();
}

void AudioManager::loadSounds() {
    // --- Sound Effects (.wav) ---
    if (ballRollBuffer.loadFromFile("assets/ball_roll.wav")) {
        ballRollSound = std::make_unique<sf::Sound>(ballRollBuffer);
        ballRollSound->setLooping(true);
        ballRollSound->setVolume(95.0f);
    }
    
    if (pinHitBuffer1.loadFromFile("assets/pin_hit1.wav")) {
        pinHitSound1 = std::make_unique<sf::Sound>(pinHitBuffer1);
    }
    if (pinHitBuffer2.loadFromFile("assets/pin_hit2.wav")) {
        pinHitSound2 = std::make_unique<sf::Sound>(pinHitBuffer2);
    }
    if (pinHitBuffer3.loadFromFile("assets/pin_hit3.wav")) {
        pinHitSound3 = std::make_unique<sf::Sound>(pinHitBuffer3);
    }
    if (pinHitBuffer4.loadFromFile("assets/pin_hit4.wav")) {
        pinHitSound4 = std::make_unique<sf::Sound>(pinHitBuffer4);
    }
    if (pinHitBuffer5.loadFromFile("assets/pin_hit5.wav")) {
        pinHitSound5 = std::make_unique<sf::Sound>(pinHitBuffer5);
    }
    
    if (pinCollisionBuffer.loadFromFile("assets/pin_collision.wav")) {
        pinCollisionSound = std::make_unique<sf::Sound>(pinCollisionBuffer);
        pinCollisionSound->setVolume(50.0f);
    }

    if (explodingPinBuffer.loadFromFile("assets/exploding_pin.wav")) {
        explodingPinSound = std::make_unique<sf::Sound>(explodingPinBuffer);
        explodingPinSound->setVolume(75.0f);
    }

    if (strikeCheerBuffer.loadFromFile("assets/cheering.wav")) {
        strikeCheerSound = std::make_unique<sf::Sound>(strikeCheerBuffer);
        strikeCheerSound->setVolume(85.0f);
    }

    if (loadBufferFromCandidates(enchantressLaughBuffer, {"assets/EnchantressLaugh.wav"})) {
        enchantressLaughSound = std::make_unique<sf::Sound>(enchantressLaughBuffer);
    }
    if (loadBufferFromCandidates(overlordLaughBuffer, {"assets/OverlordLaugh.wav"})) {
        overlordLaughSound = std::make_unique<sf::Sound>(overlordLaughBuffer);
    }
    if (loadBufferFromCandidates(twinsLaughBuffer, {"assets/TwinsLaugh.wav"})) {
        twinsLaughSound = std::make_unique<sf::Sound>(twinsLaughBuffer);
    }
    if (loadBufferFromCandidates(emperorLaughBuffer, {"assets/EmperorLaugh.wav", "assets/EmeperorLaugh.wav"})) {
        emperorLaughSound = std::make_unique<sf::Sound>(emperorLaughBuffer);
    }
    if (loadBufferFromCandidates(monsterLaughBuffer, {"assets/MonsterLaugh.wav"})) {
        monsterLaughSound = std::make_unique<sf::Sound>(monsterLaughBuffer);
    }
    
    // --- Music Files (Using if checks to silence [[nodiscard]] warnings) ---
    
    if (menuMusic1.openFromFile("assets/music1.wav")) {
        menuMusic1.setLooping(false); 
        menuMusic1.setVolume(30.0f); // Set to 30% volume

    }
    
    if (menuMusic2.openFromFile("assets/music2.wav")) {
        menuMusic2.setLooping(false);
        menuMusic1.setVolume(30.0f); // Set to 30% volume
    }

    if (gameMusic.openFromFile("assets/background_music.flac")) {
        gameMusic.setLooping(true);
    }
    
    soundsLoaded = true;
}

void AudioManager::playMenuMusic() {
    // Stop game music if transitioning from game to menu
    if (gameMusic.getStatus() == sf::Music::Status::Playing) {
        gameMusic.stop();
    }

    // If a menu track is already playing, just let it finish
    if (menuMusic1.getStatus() == sf::Music::Status::Playing || 
        menuMusic2.getStatus() == sf::Music::Status::Playing) {
        return;
    }

    // Cycle between the two menu tracks
    if (currentTrack != 1) {
        menuMusic1.play();
        currentTrack = 1;
    } else {
        menuMusic2.play();
        currentTrack = 2;
    }
}

void AudioManager::playBackgroundMusic() {
    // Stop menu music
    menuMusic1.stop();
    menuMusic2.stop();

    // Play the main game music (.flac)
    if (gameMusic.getStatus() != sf::Music::Status::Playing) {
        gameMusic.play();
    }
}

void AudioManager::stopBackgroundMusic() {
    menuMusic1.stop();
    menuMusic2.stop();
    gameMusic.stop();
    currentTrack = 0;
}

void AudioManager::setMusicVolume(float volume) {
    // SFML volume is 0 to 100
    menuMusic1.setVolume(volume);
    menuMusic2.setVolume(volume);
    gameMusic.setVolume(volume);
}

void AudioManager::setSoundVolume(float volume) {
    sfxVolume = std::clamp(volume, 0.0f, 100.0f);
}

// --- Sound Effect Logic ---

void AudioManager::playRandomPinHit(float volume) {
    if (!soundsLoaded) return;
    int randomSound = rand() % 5 + 1;
    sf::Sound* sound = nullptr;
    switch(randomSound) {
        case 1: sound = pinHitSound1.get(); break;
        case 2: sound = pinHitSound2.get(); break;
        case 3: sound = pinHitSound3.get(); break;
        case 4: sound = pinHitSound4.get(); break;
        case 5: sound = pinHitSound5.get(); break;
    }
    if (sound && sound->getStatus() != sf::SoundSource::Status::Playing) {
        sound->setVolume(volume * (sfxVolume / 100.0f));
        sound->play();
    }
}

void AudioManager::playPinCollision(float volume) {
    if (!soundsLoaded || !pinCollisionSound) return;
    if (pinCollisionSound->getStatus() != sf::SoundSource::Status::Playing) {
        pinCollisionSound->setVolume(volume * (sfxVolume / 100.0f));
        pinCollisionSound->play();
    }
}

void AudioManager::playExplodingPin(float volume) {
    if (!soundsLoaded || !explodingPinSound) return;
    if (explodingPinSound->getStatus() != sf::SoundSource::Status::Playing) {
        explodingPinSound->setVolume(volume * (sfxVolume / 100.0f));
        explodingPinSound->play();
    }
}

void AudioManager::playStrikeCheer(float volume) {
    if (!soundsLoaded || !strikeCheerSound) return;
    if (strikeCheerSound->getStatus() != sf::SoundSource::Status::Playing) {
        strikeCheerSound->setVolume(volume * (sfxVolume / 100.0f));
        strikeCheerSound->play();
    }
}

void AudioManager::playBossLaugh(BossLaughType boss, float volume) {
    if (!soundsLoaded) return;

    sf::Sound* sound = nullptr;
    switch (boss) {
        case BossLaughType::Enchantress: sound = enchantressLaughSound.get(); break;
        case BossLaughType::Overlord:    sound = overlordLaughSound.get(); break;
        case BossLaughType::Twins:       sound = twinsLaughSound.get(); break;
        case BossLaughType::Emperor:     sound = emperorLaughSound.get(); break;
        case BossLaughType::Monster:     sound = monsterLaughSound.get(); break;
    }
    if (!sound) return;

    // Restart so boss intro / boss loss always fires audibly when requested.
    sound->stop();
    sound->setVolume(volume * (sfxVolume / 100.0f));
    sound->play();
}

void AudioManager::startBallRoll() {
    if (!soundsLoaded || !ballRollSound) return;
    if (ballRollSound->getStatus() != sf::SoundSource::Status::Playing) {
        ballRollSound->setVolume(95.0f * (sfxVolume / 100.0f));
        ballRollSound->play();
        ballRolling = true;
    }
}

void AudioManager::stopBallRoll() {
    if (ballRolling && ballRollSound) {
        ballRollSound->stop();
        ballRolling = false;
    }
}
