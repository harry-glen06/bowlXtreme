#pragma once
#include <SFML/Audio.hpp>
#include <memory>

class AudioManager {
public:
    AudioManager();
    
    // Initialization
    void loadSounds();
    
    // Sound playback
    void playRandomPinHit(float volume = 70.0f);
    void playPinCollision(float volume = 50.0f);
    void startBallRoll();
    void stopBallRoll();
    
    // Music control
    void playBackgroundMusic();
    void stopBackgroundMusic();
    void setMusicVolume(float volume);
    
    // State
    bool areSoundsLoaded() const { return soundsLoaded; }
    bool isBallRolling() const { return ballRolling; }
    
private:
    // Sound buffers
    sf::SoundBuffer ballRollBuffer;
    sf::SoundBuffer pinHitBuffer1;
    sf::SoundBuffer pinHitBuffer2;
    sf::SoundBuffer pinHitBuffer3;
    sf::SoundBuffer pinHitBuffer4;
    sf::SoundBuffer pinHitBuffer5;
    sf::SoundBuffer pinCollisionBuffer;
    
    // Sound objects
    std::unique_ptr<sf::Sound> ballRollSound;
    std::unique_ptr<sf::Sound> pinHitSound1;
    std::unique_ptr<sf::Sound> pinHitSound2;
    std::unique_ptr<sf::Sound> pinHitSound3;
    std::unique_ptr<sf::Sound> pinHitSound4;
    std::unique_ptr<sf::Sound> pinHitSound5;
    std::unique_ptr<sf::Sound> pinCollisionSound;
    
    // Background music
    sf::Music backgroundMusic;
    
    // State
    bool soundsLoaded = false;
    bool ballRolling = false;
    float masterVolume = 1.0f;
};
