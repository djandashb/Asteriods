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
        WaveSummary,
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
        float maxLifeSeconds{2.0F};
        float radius{3.0F};
        bool hasWrapped{false};
        float wrappedDistanceRemaining{0.0F};
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

    struct BreakAnimation {
        Vec2 position{};
        AsteroidSize size{AsteroidSize::Large};
        float elapsedSeconds{0.0F};
    };

    struct SaucerExplosion {
        Vec2 position{};
        float elapsedSeconds{0.0F};
    };

    struct PlayerExplosion {
        Vec2 position{};
        float elapsedSeconds{0.0F};
    };

    struct FloatingScore {
        Vec2 position{};
        int points{0};
        float delaySeconds{0.0F};
        float elapsedSeconds{0.0F};
    };

    struct EnemySaucer {
        Vec2 position{};
        Vec2 velocity{};
        float radius{26.0F};
        float fireCooldownSeconds{1.6F};
        bool active{false};
    };

    struct EnemyBullet {
        Vec2 position{};
        Vec2 velocity{};
        float lifeSeconds{1.8F};
        float radius{3.0F};
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
    std::vector<Texture> loadBreakFrames(const std::string& directory, int frameCount);
    void destroyAssets();
    void destroyAudio();

    void resetGame();
    void jumpToWave(int wave);
    void spawnWave();
    void startWaveSummary();
    void spawnAsteroid(Vec2 position, AsteroidSize size);
    void splitAsteroid(const Asteroid& asteroid);
    void spawnBreakAnimation(const Asteroid& asteroid);
    void spawnSaucerExplosion(Vec2 position);
    void spawnPlayerExplosion(Vec2 position);
    void spawnFloatingScore(Vec2 position, int points, float delaySeconds);
    void fireBullet();
    void playAsteroidHitSound(AsteroidSize size);
    void playMissileFireSound();
    void playEnemySaucerDestroyedSound();
    void playPlayerShipDestroyedSound();
    void playWaveClearFanfareSound();
    void updateThrustSound();
    void updateBackgroundBeat(float deltaSeconds);
    void playBackgroundBeatPulse();
    void updateEnemySaucer(float deltaSeconds);
    void spawnEnemySaucer();
    void fireEnemySaucerBullet();
    void updateEnemySaucerSound();
    void stopEnemySaucerSound();

    void handleEvents(bool& running);
    void update(float deltaSeconds);
    void updatePlaying(float deltaSeconds);
    void updateBreakAnimations(float deltaSeconds);
    void updateSaucerExplosions(float deltaSeconds);
    void updatePlayerExplosions(float deltaSeconds);
    void updateFloatingScores(float deltaSeconds);
    void render();

    void renderSplash();
    void renderStart();
    void renderPlaying();
    void renderPaused();
    void renderWaveSummary();
    void renderGameOver();
    void renderShip();
    void renderHud();
    void renderDiagnostics();
    void renderBreakAnimations();
    void renderSaucerExplosions();
    void renderPlayerExplosions();
    void renderFloatingScores();
    void renderEnemySaucer();
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
    std::vector<EnemyBullet> enemyBullets_;
    std::vector<Asteroid> asteroids_;
    std::vector<BreakAnimation> breakAnimations_;
    std::vector<SaucerExplosion> saucerExplosions_;
    std::vector<PlayerExplosion> playerExplosions_;
    std::vector<FloatingScore> floatingScores_;
    EnemySaucer enemySaucer_{};

    Texture shipTexture_{};
    Texture shipThrustTexture_{};
    Texture enemySaucerTexture_{};
    Texture logoTexture_{};
    std::array<Texture, 3> largeAsteroids_{};
    std::array<Texture, 3> mediumAsteroids_{};
    std::array<Texture, 3> smallAsteroids_{};
    std::vector<Texture> largeBreakFrames_;
    std::vector<Texture> mediumBreakFrames_;
    std::vector<Texture> smallBreakFrames_;
    std::vector<Texture> saucerExplosionFrames_;
    std::vector<Texture> playerExplosionFrames_;
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
    SDL_AudioStream* enemySaucerStream_{nullptr};
    Uint8* enemySaucerBuffer_{nullptr};
    Uint32 enemySaucerBufferLength_{0};
    SDL_AudioStream* enemySaucerDestroyedStream_{nullptr};
    Uint8* enemySaucerDestroyedBuffer_{nullptr};
    Uint32 enemySaucerDestroyedBufferLength_{0};
    SDL_AudioStream* playerShipDestroyedStream_{nullptr};
    Uint8* playerShipDestroyedBuffer_{nullptr};
    Uint32 playerShipDestroyedBufferLength_{0};
    SDL_AudioStream* waveClearFanfareStream_{nullptr};
    Uint8* waveClearFanfareBuffer_{nullptr};
    Uint32 waveClearFanfareBufferLength_{0};

    std::mt19937 rng_;
    int score_{0};
    int lives_{5};
    int wave_{1};
    int waveStartingAsteroids_{0};
    int waveStartingLargeAsteroids_{0};
    int asteroidsDestroyedThisWave_{0};
    int largeAsteroidsDestroyedThisWave_{0};
    int enemySaucersDestroyedThisWave_{0};
    int waveStartScore_{0};
    int summaryWave_{1};
    int summaryAsteroidsDestroyed_{0};
    int summaryEnemySaucersDestroyed_{0};
    int summaryWavePoints_{0};
    int enemySaucerPassesThisWave_{0};
    float enemySaucerSpawnDelaySeconds_{0.0F};
    float fireCooldownSeconds_{0.0F};
    float splashSecondsRemaining_{3.0F};
    float waveSummarySecondsRemaining_{0.0F};
    float backgroundBeatSeconds_{0.0F};
    float rotateLeftGraceSeconds_{0.0F};
    float rotateRightGraceSeconds_{0.0F};
    float thrustAudioGraceSeconds_{0.0F};
    bool thrusting_{false};
    bool enemySaucerSpawnQueued_{false};
    bool enemySaucerSpawnedThisWave_{false};
    bool diagnosticsEnabled_{false};
    SDL_Scancode lastScancode_{SDL_SCANCODE_UNKNOWN};
    SDL_Keycode lastKeycode_{0};
    bool lastKeyPressed_{false};
    std::string lastScancodeName_{"NONE"};
    std::string lastKeyName_{"NONE"};
};
