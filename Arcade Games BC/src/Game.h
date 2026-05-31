#pragma once

#include <SDL3/SDL.h>

#include <array>
#include <memory>
#include <random>
#include <string>
#include <vector>

struct Vec2 {
    float x{0.0F};
    float y{0.0F};

    Vec2 operator+(Vec2 other) const;
    Vec2 operator-(Vec2 other) const;
    Vec2 operator*(float scalar) const;
    Vec2& operator+=(Vec2 other);
    Vec2& operator*=(float scalar);
};

float length(Vec2 value);
Vec2 normalized(Vec2 value);

struct Texture {
    SDL_Texture* handle{nullptr};
    float width{0.0F};
    float height{0.0F};
};

class Game {
public:
    Game();
    ~Game();

    Game(const Game&) = delete;
    Game& operator=(const Game&) = delete;

    bool init();
    void run();

private:
    enum class State {
        Splash,
        Start,
        Playing,
        Paused,
        GameOver
    };

    enum class AsteroidSize {
        Large,
        Medium,
        Small
    };

    struct Ship {
        Vec2 position{400.0F, 300.0F};
        Vec2 velocity{};
        float angleDegrees{0.0F};
        float radius{24.0F};
        float invulnerableSeconds{2.0F};
    };

    struct Bullet {
        Vec2 position{};
        Vec2 velocity{};
        float lifeSeconds{1.0F};
        float radius{3.0F};
    };

    struct Asteroid {
        Vec2 position{};
        Vec2 velocity{};
        AsteroidSize size{AsteroidSize::Large};
        int textureIndex{0};
        float radius{50.0F};
        float angleDegrees{0.0F};
        float spinDegreesPerSecond{0.0F};
    };

    struct InputState {
        bool rotateLeft{false};
        bool rotateRight{false};
        bool thrust{false};
        bool fire{false};
    };

    bool loadAssets();
    bool loadAudio();
    Texture loadTexture(const std::string& path);
    void destroyAssets();
    void destroyAudio();

    void resetGame();
    void spawnWave();
    void spawnAsteroid(Vec2 position, AsteroidSize size);
    void splitAsteroid(const Asteroid& asteroid);
    void fireBullet();
    void playAsteroidHitSound(AsteroidSize size);
    void playMissileFireSound();
    void updateThrustSound();
    void updateBackgroundBeat(float deltaSeconds);
    void playBackgroundBeatPulse();

    void handleEvents(bool& running);
    void update(float deltaSeconds);
    void updatePlaying(float deltaSeconds);
    void render();

    void renderSplash();
    void renderStart();
    void renderPlaying();
    void renderPaused();
    void renderGameOver();
    void renderShip();
    void renderHud();
    void drawText(const std::string& text, float x, float y, int scale, SDL_Color color);
    void drawGlyph(char glyph, float x, float y, int scale, SDL_Color color);
    void drawCenteredText(const std::string& text, float y, int scale, SDL_Color color);

    void wrap(Vec2& position, float margin = 0.0F);
    bool collides(Vec2 a, float ar, Vec2 b, float br) const;
    Vec2 forwardVector() const;
    float randomFloat(float min, float max);
    int randomInt(int min, int max);
    Texture& asteroidTexture(const Asteroid& asteroid);

    SDL_Window* window_{nullptr};
    SDL_Renderer* renderer_{nullptr};
    State state_{State::Splash};
    InputState input_{};

    Ship ship_{};
    std::vector<Bullet> bullets_;
    std::vector<Asteroid> asteroids_;

    Texture shipTexture_{};
    Texture shipThrustTexture_{};
    Texture logoTexture_{};
    std::array<Texture, 3> largeAsteroids_{};
    std::array<Texture, 3> mediumAsteroids_{};
    std::array<Texture, 3> smallAsteroids_{};
    SDL_AudioStream* asteroidHitStream_{nullptr};
    Uint8* asteroidHitBuffer_{nullptr};
    Uint32 asteroidHitBufferLength_{0};
    SDL_AudioStream* missileFireStream_{nullptr};
    Uint8* missileFireBuffer_{nullptr};
    Uint32 missileFireBufferLength_{0};
    SDL_AudioStream* thrustStream_{nullptr};
    Uint8* thrustBuffer_{nullptr};
    Uint32 thrustBufferLength_{0};
    SDL_AudioStream* backgroundBeatStream_{nullptr};
    std::vector<Sint16> backgroundBeatLow_;
    std::vector<Sint16> backgroundBeatHigh_;

    std::mt19937 rng_;
    int score_{0};
    int lives_{5};
    int wave_{1};
    int asteroidsDestroyedThisWave_{0};
    float fireCooldownSeconds_{0.0F};
    float splashSecondsRemaining_{3.0F};
    float backgroundBeatSeconds_{0.0F};
    float rotateLeftGraceSeconds_{0.0F};
    float rotateRightGraceSeconds_{0.0F};
    float thrustAudioGraceSeconds_{0.0F};
    bool thrusting_{false};
    SDL_Scancode lastScancode_{SDL_SCANCODE_UNKNOWN};
    SDL_Keycode lastKeycode_{0};
    bool lastKeyPressed_{false};
    std::string lastScancodeName_{"NONE"};
    std::string lastKeyName_{"NONE"};
};
