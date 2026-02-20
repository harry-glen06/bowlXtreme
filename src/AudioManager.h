#pragma once
#include <SFML/Audio.hpp>
#include <memory>

class AudioManager {
public:
    AudioManager();
    
    // Initialization
    void loadSounds();
    
    // Sound playback (SFX)
    void playRandomPinHit(float volume = 70.0f);
    void playPinCollision(float volume = 50.0f);
    void playExplodingPin(float volume = 75.0f);
    void startBallRoll();
    void stopBallRoll();
    
    // Music control
    void playMenuMusic();          // Cycles music1.wav and music2.wav
    void playBackgroundMusic();    // Plays background_music.flac for gameplay
    void stopBackgroundMusic();    // Stops all music (Menu and Game)
    void setMusicVolume(float volume);
    void setSoundVolume(float volume);
    
    // State Getters
    bool areSoundsLoaded() const { return soundsLoaded; }
    bool isBallRolling() const { return ballRolling; }

private:
    // Sound buffers (Stored in RAM)
    sf::SoundBuffer ballRollBuffer;
    sf::SoundBuffer pinHitBuffer1;
    sf::SoundBuffer pinHitBuffer2;
    sf::SoundBuffer pinHitBuffer3;
    sf::SoundBuffer pinHitBuffer4;
    sf::SoundBuffer pinHitBuffer5;
    sf::SoundBuffer pinCollisionBuffer;
    sf::SoundBuffer explodingPinBuffer;
    
    // Sound objects (Using unique_ptr for safe memory management)
    std::unique_ptr<sf::Sound> ballRollSound;
    std::unique_ptr<sf::Sound> pinHitSound1;
    std::unique_ptr<sf::Sound> pinHitSound2;
    std::unique_ptr<sf::Sound> pinHitSound3;
    std::unique_ptr<sf::Sound> pinHitSound4;
    std::unique_ptr<sf::Sound> pinHitSound5;
    std::unique_ptr<sf::Sound> pinCollisionSound;
    std::unique_ptr<sf::Sound> explodingPinSound;
    
    // Music objects (Streamed from disk to save memory)
    sf::Music menuMusic1;   // For assets/music1.wav
    sf::Music menuMusic2;   // For assets/music2.wav
    sf::Music gameMusic;    // For assets/background_music.flac
    
    // Internal State
    int currentTrack = 0;   // Tracks which menu track to play next
    bool soundsLoaded = false;
    bool ballRolling = false;
    float masterVolume = 0.5f; // Default volume (50%)
    float sfxVolume = 70.0f;
};
