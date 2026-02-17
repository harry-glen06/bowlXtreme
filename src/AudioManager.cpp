#include "AudioManager.h"
#include <cstdlib>

AudioManager::AudioManager() {
    loadSounds();
}

void AudioManager::loadSounds() {
    // Load ball roll sound
    if (ballRollBuffer.loadFromFile("assets/ball_roll.wav")) {
        ballRollSound = std::make_unique<sf::Sound>(ballRollBuffer);
        ballRollSound->setLooping(true);
        ballRollSound->setVolume(95.0f);
    }
    
    // Load 5 different pin hit sounds
    if (pinHitBuffer1.loadFromFile("assets/pin_hit1.wav")) {
        pinHitSound1 = std::make_unique<sf::Sound>(pinHitBuffer1);
        pinHitSound1->setVolume(70.0f);
    }
    
    if (pinHitBuffer2.loadFromFile("assets/pin_hit2.wav")) {
        pinHitSound2 = std::make_unique<sf::Sound>(pinHitBuffer2);
        pinHitSound2->setVolume(70.0f);
    }
    
    if (pinHitBuffer3.loadFromFile("assets/pin_hit3.wav")) {
        pinHitSound3 = std::make_unique<sf::Sound>(pinHitBuffer3);
        pinHitSound3->setVolume(70.0f);
    }
    
    if (pinHitBuffer4.loadFromFile("assets/pin_hit4.wav")) {
        pinHitSound4 = std::make_unique<sf::Sound>(pinHitBuffer4);
        pinHitSound4->setVolume(70.0f);
    }
    
    if (pinHitBuffer5.loadFromFile("assets/pin_hit5.wav")) {
        pinHitSound5 = std::make_unique<sf::Sound>(pinHitBuffer5);
        pinHitSound5->setVolume(70.0f);
    }
    
    // Load pin collision sound
    if (pinCollisionBuffer.loadFromFile("assets/pin_collision.wav")) {
        pinCollisionSound = std::make_unique<sf::Sound>(pinCollisionBuffer);
        pinCollisionSound->setVolume(50.0f);
    } else if (pinHitBuffer1.getDuration().asSeconds() > 0) {
        // Reuse pin_hit1 if no dedicated collision sound
        pinCollisionSound = std::make_unique<sf::Sound>(pinHitBuffer1);
        pinCollisionSound->setVolume(40.0f);
    }
    
    // Load background music
    if (backgroundMusic.openFromFile("assets/background_music.flac")) {
        backgroundMusic.setLooping(true);
        backgroundMusic.setVolume(masterVolume * 100.0f);
    }
    
    soundsLoaded = true;
}

void AudioManager::playRandomPinHit(float volume) {
    if (!soundsLoaded) return;
    
    // Pick a random sound from 1-5
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
        sound->setVolume(volume);
        sound->play();
    }
}

void AudioManager::playPinCollision(float volume) {
    if (!soundsLoaded || !pinCollisionSound) return;
    
    if (pinCollisionSound->getStatus() != sf::SoundSource::Status::Playing) {
        pinCollisionSound->setVolume(volume);
        pinCollisionSound->play();
    }
}

void AudioManager::startBallRoll() {
    if (!soundsLoaded || !ballRollSound) return;
    
    if (ballRollSound->getStatus() != sf::SoundSource::Status::Playing) {
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

void AudioManager::playBackgroundMusic() {
    if (backgroundMusic.getStatus() != sf::Music::Status::Playing) {
        backgroundMusic.play();
    }
}

void AudioManager::stopBackgroundMusic() {
    backgroundMusic.stop();
}

void AudioManager::setMusicVolume(float volume) {
    masterVolume = volume;
    backgroundMusic.setVolume(masterVolume * 100.0f);
}
