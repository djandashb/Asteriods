#include "Game.h"

#include <SDL3_image/SDL_image.h>

#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <unordered_map>

namespace {
constexpr int kWindowWidth = 1024;
constexpr int kWindowHeight = 768;
constexpr float kPi = 3.1415926535F;
constexpr float kRotationDegreesPerSecond = 520.0F;
constexpr float kRotationGraceSeconds = 0.08F;
constexpr float kAcceleration = 260.0F;
constexpr float kTapThrustImpulse = 80.0F;
constexpr float kThrustAudioGraceSeconds = 0.45F;
constexpr float kMaxShipSpeed = 480.0F;
constexpr float kDragPerSecond = 0.72F;
constexpr float kBulletSpeed = 720.0F;
constexpr float kBulletMaxLifeSeconds = 2.0F;
constexpr float kBulletWrappedTravelFraction = 0.4F;
constexpr float kFireCooldownSeconds = 0.16F;
constexpr int kAudioSampleRate = 44100;
constexpr int kAudioChannels = 2;
constexpr float kBackgroundBeatMaxInterval = 0.78F;
constexpr float kBackgroundBeatMinInterval = 0.22F;
constexpr int kBaseAsteroids = 3;
constexpr float kBreakAnimationFrameSeconds = 0.056F;
constexpr float kSaucerExplosionFrameSeconds = 0.056F;
constexpr float kPlayerExplosionFrameSeconds = 0.070F;
constexpr float kEnemySaucerSpeed = 135.0F;
constexpr float kEnemySaucerBulletSpeed = 360.0F;
constexpr float kEnemySaucerBulletLifeSeconds = 1.8F;
constexpr float kEnemySaucerFireCooldownMin = 1.25F;
constexpr float kEnemySaucerFireCooldownMax = 2.1F;
constexpr float kEnemySaucerAimErrorDegrees = 18.0F;
constexpr float kEnemySaucerFirstArrivalDelay = 20.0F;
constexpr float kEnemySaucerFirstRespawnDelay = 30.0F;
constexpr float kEnemySaucerRespawnDelayStep = 4.0F;
constexpr float kEnemySaucerMinRespawnDelay = 10.0F;
constexpr int kEnemySaucerScore = 500;

bool isKey(SDL_Scancode actualScancode, SDL_Keycode actualKey, SDL_Scancode expectedScancode, SDL_Keycode expectedKey)
{
    return actualScancode == expectedScancode || actualKey == expectedKey;
}

bool isModifier(SDL_Scancode scancode)
{
    return scancode == SDL_SCANCODE_LSHIFT ||
        scancode == SDL_SCANCODE_RSHIFT ||
        scancode == SDL_SCANCODE_LCTRL ||
        scancode == SDL_SCANCODE_RCTRL ||
        scancode == SDL_SCANCODE_LALT ||
        scancode == SDL_SCANCODE_RALT ||
        scancode == SDL_SCANCODE_LGUI ||
        scancode == SDL_SCANCODE_RGUI;
}

float degreesToRadians(float degrees)
{
    return degrees * kPi / 180.0F;
}

std::string assetPath(const std::string& relative)
{
    return "assets/" + relative;
}

std::vector<Sint16> makeBeatPulse(float frequency)
{
    constexpr float durationSeconds = 0.085F;
    constexpr float volume = 0.22F;
    const int sampleCount = static_cast<int>(kAudioSampleRate * durationSeconds);
    std::vector<Sint16> samples(static_cast<std::size_t>(sampleCount * kAudioChannels));

    for (int i = 0; i < sampleCount; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(kAudioSampleRate);
        const float progress = static_cast<float>(i) / static_cast<float>(std::max(1, sampleCount - 1));
        const float attack = std::min(1.0F, progress * 18.0F);
        const float decay = 1.0F - progress;
        const float envelope = attack * decay * decay;
        const float sine = std::sin(2.0F * kPi * frequency * t);
        const Sint16 value = static_cast<Sint16>(32767.0F * volume * envelope * sine);
        samples[static_cast<std::size_t>(i * 2)] = value;
        samples[static_cast<std::size_t>((i * 2) + 1)] = value;
    }

    return samples;
}

const std::array<const char*, 7>& glyphRows(char glyph)
{
    static const std::unordered_map<char, std::array<const char*, 7>> glyphs{
        {' ', {"00000", "00000", "00000", "00000", "00000", "00000", "00000"}},
        {'0', {"01110", "10001", "10011", "10101", "11001", "10001", "01110"}},
        {'1', {"00100", "01100", "00100", "00100", "00100", "00100", "01110"}},
        {'2', {"01110", "10001", "00001", "00010", "00100", "01000", "11111"}},
        {'3', {"11110", "00001", "00001", "01110", "00001", "00001", "11110"}},
        {'4', {"00010", "00110", "01010", "10010", "11111", "00010", "00010"}},
        {'5', {"11111", "10000", "10000", "11110", "00001", "00001", "11110"}},
        {'6', {"01110", "10000", "10000", "11110", "10001", "10001", "01110"}},
        {'7', {"11111", "00001", "00010", "00100", "01000", "01000", "01000"}},
        {'8', {"01110", "10001", "10001", "01110", "10001", "10001", "01110"}},
        {'9', {"01110", "10001", "10001", "01111", "00001", "00001", "01110"}},
        {'A', {"01110", "10001", "10001", "11111", "10001", "10001", "10001"}},
        {'C', {"01111", "10000", "10000", "10000", "10000", "10000", "01111"}},
        {'D', {"11110", "10001", "10001", "10001", "10001", "10001", "11110"}},
        {'E', {"11111", "10000", "10000", "11110", "10000", "10000", "11111"}},
        {'F', {"11111", "10000", "10000", "11110", "10000", "10000", "10000"}},
        {'G', {"01111", "10000", "10000", "10011", "10001", "10001", "01111"}},
        {'H', {"10001", "10001", "10001", "11111", "10001", "10001", "10001"}},
        {'I', {"01110", "00100", "00100", "00100", "00100", "00100", "01110"}},
        {'K', {"10001", "10010", "10100", "11000", "10100", "10010", "10001"}},
        {'L', {"10000", "10000", "10000", "10000", "10000", "10000", "11111"}},
        {'M', {"10001", "11011", "10101", "10101", "10001", "10001", "10001"}},
        {'N', {"10001", "11001", "10101", "10011", "10001", "10001", "10001"}},
        {'O', {"01110", "10001", "10001", "10001", "10001", "10001", "01110"}},
        {'P', {"11110", "10001", "10001", "11110", "10000", "10000", "10000"}},
        {'R', {"11110", "10001", "10001", "11110", "10100", "10010", "10001"}},
        {'S', {"01111", "10000", "10000", "01110", "00001", "00001", "11110"}},
        {'T', {"11111", "00100", "00100", "00100", "00100", "00100", "00100"}},
        {'U', {"10001", "10001", "10001", "10001", "10001", "10001", "01110"}},
        {'V', {"10001", "10001", "10001", "10001", "10001", "01010", "00100"}},
        {'W', {"10001", "10001", "10001", "10101", "10101", "10101", "01010"}},
        {'Y', {"10001", "10001", "01010", "00100", "00100", "00100", "00100"}},
        {':', {"00000", "00100", "00100", "00000", "00100", "00100", "00000"}},
        {'-', {"00000", "00000", "00000", "11111", "00000", "00000", "00000"}},
    };
    static const std::array<const char*, 7> fallback{"11111", "00001", "00010", "00100", "00100", "00000", "00100"};
    const auto found = glyphs.find(glyph);
    return found == glyphs.end() ? fallback : found->second;
}
}

Vec2 Vec2::operator+(Vec2 other) const { return {x + other.x, y + other.y}; }
Vec2 Vec2::operator-(Vec2 other) const { return {x - other.x, y - other.y}; }
Vec2 Vec2::operator*(float scalar) const { return {x * scalar, y * scalar}; }
Vec2& Vec2::operator+=(Vec2 other)
{
    x += other.x;
    y += other.y;
    return *this;
}
Vec2& Vec2::operator*=(float scalar)
{
    x *= scalar;
    y *= scalar;
    return *this;
}

float length(Vec2 value)
{
    return std::sqrt((value.x * value.x) + (value.y * value.y));
}

Vec2 normalized(Vec2 value)
{
    const float len = length(value);
    if (len <= 0.0001F) {
        return {};
    }
    return {value.x / len, value.y / len};
}

Game::Game()
    : rng_(std::random_device{}())
{
}

Game::~Game()
{
    destroyAudio();
    destroyAssets();
    if (renderer_) {
        SDL_DestroyRenderer(renderer_);
    }
    if (window_) {
        SDL_DestroyWindow(window_);
    }
    SDL_Quit();
}

bool Game::init()
{
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD | SDL_INIT_AUDIO)) {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return false;
    }

    if (!SDL_CreateWindowAndRenderer("Retro Arcade Collection - Asteroids", kWindowWidth, kWindowHeight, 0, &window_, &renderer_)) {
        std::fprintf(stderr, "SDL_CreateWindowAndRenderer failed: %s\n", SDL_GetError());
        return false;
    }

    SDL_SetRenderVSync(renderer_, 1);
    if (!loadAssets()) {
        return false;
    }

    loadAudio();
    return true;
}

void Game::run()
{
    bool running = true;
    Uint64 lastTicks = SDL_GetTicks();

    while (running) {
        const Uint64 nowTicks = SDL_GetTicks();
        const float deltaSeconds = std::min((nowTicks - lastTicks) / 1000.0F, 0.05F);
        lastTicks = nowTicks;

        handleEvents(running);
        update(deltaSeconds);
        render();
    }
}

bool Game::loadAssets()
{
    shipTexture_ = loadTexture(assetPath("sprites/ship/player_ship.png"));
    shipThrustTexture_ = loadTexture(assetPath("sprites/ship/player_ship_thrust.png"));
    enemySaucerTexture_ = loadTexture(assetPath("sprites/enemy/enemy_ship.png"));
    logoTexture_ = loadTexture(assetPath("ui/bc_logo_reference.png"));

    for (int i = 0; i < 3; ++i) {
        largeAsteroids_[i] = loadTexture(assetPath("sprites/asteroids/asteroid_large_" + std::to_string(i + 1) + ".png"));
        mediumAsteroids_[i] = loadTexture(assetPath("sprites/asteroids/asteroid_medium_" + std::to_string(i + 1) + ".png"));
        smallAsteroids_[i] = loadTexture(assetPath("sprites/asteroids/asteroid_small_" + std::to_string(i + 1) + ".png"));
    }

    largeBreakFrames_ = loadBreakFrames("sprites/_temp_asteroid_break_review/large_break_radial", 12);
    mediumBreakFrames_ = loadBreakFrames("sprites/_temp_asteroid_break_review/medium_break_radial", 10);
    smallBreakFrames_ = loadBreakFrames("sprites/_temp_asteroid_break_review/small_break_radial", 8);
    saucerExplosionFrames_ = loadBreakFrames("sprites/_temp_enemy_saucer_explosion_review/enemy_saucer_explosion", 12);
    playerExplosionFrames_ = loadBreakFrames("sprites/_temp_player_ship_explosion_review/player_ship_explosion", 12);

    return shipTexture_.handle &&
        shipThrustTexture_.handle &&
        enemySaucerTexture_.handle &&
        logoTexture_.handle &&
        largeAsteroids_[0].handle &&
        mediumAsteroids_[0].handle &&
        smallAsteroids_[0].handle &&
        largeBreakFrames_.size() == 12 &&
        mediumBreakFrames_.size() == 10 &&
        smallBreakFrames_.size() == 8 &&
        saucerExplosionFrames_.size() == 12 &&
        playerExplosionFrames_.size() == 12;
}

bool Game::loadAudio()
{
    SDL_AudioSpec asteroidSpec{};
    if (!SDL_LoadWAV(assetPath("sounds/rocks_breaking.wav").c_str(), &asteroidSpec, &asteroidHitBuffer_, &asteroidHitBufferLength_)) {
        std::fprintf(stderr, "Could not load asteroid hit sound: %s\n", SDL_GetError());
        return false;
    }

    asteroidHitStream_ = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &asteroidSpec, nullptr, nullptr);
    if (!asteroidHitStream_) {
        std::fprintf(stderr, "Could not open asteroid hit audio stream: %s\n", SDL_GetError());
        SDL_free(asteroidHitBuffer_);
        asteroidHitBuffer_ = nullptr;
        asteroidHitBufferLength_ = 0;
        return false;
    }

    SDL_ResumeAudioStreamDevice(asteroidHitStream_);

    SDL_AudioSpec missileSpec{};
    if (!SDL_LoadWAV(assetPath("sounds/missile_fire.wav").c_str(), &missileSpec, &missileFireBuffer_, &missileFireBufferLength_)) {
        std::fprintf(stderr, "Could not load missile fire sound: %s\n", SDL_GetError());
        return false;
    }

    missileFireStream_ = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &missileSpec, nullptr, nullptr);
    if (!missileFireStream_) {
        std::fprintf(stderr, "Could not open missile fire audio stream: %s\n", SDL_GetError());
        SDL_free(missileFireBuffer_);
        missileFireBuffer_ = nullptr;
        missileFireBufferLength_ = 0;
        return false;
    }

    SDL_ResumeAudioStreamDevice(missileFireStream_);

    SDL_AudioSpec thrustSpec{};
    if (!SDL_LoadWAV(assetPath("sounds/ship_thrust.wav").c_str(), &thrustSpec, &thrustBuffer_, &thrustBufferLength_)) {
        std::fprintf(stderr, "Could not load ship thrust sound: %s\n", SDL_GetError());
        return false;
    }

    thrustStream_ = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &thrustSpec, nullptr, nullptr);
    if (!thrustStream_) {
        std::fprintf(stderr, "Could not open ship thrust audio stream: %s\n", SDL_GetError());
        SDL_free(thrustBuffer_);
        thrustBuffer_ = nullptr;
        thrustBufferLength_ = 0;
        return false;
    }

    SDL_ResumeAudioStreamDevice(thrustStream_);

    SDL_AudioSpec beatSpec{SDL_AUDIO_S16, kAudioChannels, kAudioSampleRate};
    backgroundBeatLow_ = makeBeatPulse(150.0F);
    backgroundBeatHigh_ = makeBeatPulse(200.0F);
    backgroundBeatStream_ = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &beatSpec, nullptr, nullptr);
    if (!backgroundBeatStream_) {
        std::fprintf(stderr, "Could not open background beat audio stream: %s\n", SDL_GetError());
        return false;
    }
    SDL_SetAudioStreamGain(backgroundBeatStream_, 1.65F);
    SDL_ResumeAudioStreamDevice(backgroundBeatStream_);

    SDL_AudioSpec saucerSpec{};
    if (!SDL_LoadWAV(assetPath("sounds/enemy_saucer_alert.wav").c_str(), &saucerSpec, &enemySaucerBuffer_, &enemySaucerBufferLength_)) {
        std::fprintf(stderr, "Could not load enemy saucer alert sound: %s\n", SDL_GetError());
        return false;
    }

    enemySaucerStream_ = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &saucerSpec, nullptr, nullptr);
    if (!enemySaucerStream_) {
        std::fprintf(stderr, "Could not open enemy saucer audio stream: %s\n", SDL_GetError());
        SDL_free(enemySaucerBuffer_);
        enemySaucerBuffer_ = nullptr;
        enemySaucerBufferLength_ = 0;
        return false;
    }
    SDL_SetAudioStreamGain(enemySaucerStream_, 0.55F);
    SDL_ResumeAudioStreamDevice(enemySaucerStream_);

    SDL_AudioSpec saucerDestroyedSpec{};
    if (!SDL_LoadWAV(assetPath("sounds/_temp_enemy_saucer_explosion_review/enemy_saucer_destroyed.wav").c_str(), &saucerDestroyedSpec, &enemySaucerDestroyedBuffer_, &enemySaucerDestroyedBufferLength_)) {
        std::fprintf(stderr, "Could not load enemy saucer destroyed sound: %s\n", SDL_GetError());
        return false;
    }

    enemySaucerDestroyedStream_ = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &saucerDestroyedSpec, nullptr, nullptr);
    if (!enemySaucerDestroyedStream_) {
        std::fprintf(stderr, "Could not open enemy saucer destroyed audio stream: %s\n", SDL_GetError());
        SDL_free(enemySaucerDestroyedBuffer_);
        enemySaucerDestroyedBuffer_ = nullptr;
        enemySaucerDestroyedBufferLength_ = 0;
        return false;
    }
    SDL_SetAudioStreamGain(enemySaucerDestroyedStream_, 0.85F);
    SDL_ResumeAudioStreamDevice(enemySaucerDestroyedStream_);

    SDL_AudioSpec playerDestroyedSpec{};
    if (!SDL_LoadWAV(assetPath("sounds/_temp_player_ship_death_review/player_ship_destroyed.wav").c_str(), &playerDestroyedSpec, &playerShipDestroyedBuffer_, &playerShipDestroyedBufferLength_)) {
        std::fprintf(stderr, "Could not load player ship destroyed sound: %s\n", SDL_GetError());
        return false;
    }

    playerShipDestroyedStream_ = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &playerDestroyedSpec, nullptr, nullptr);
    if (!playerShipDestroyedStream_) {
        std::fprintf(stderr, "Could not open player ship destroyed audio stream: %s\n", SDL_GetError());
        SDL_free(playerShipDestroyedBuffer_);
        playerShipDestroyedBuffer_ = nullptr;
        playerShipDestroyedBufferLength_ = 0;
        return false;
    }
    SDL_SetAudioStreamGain(playerShipDestroyedStream_, 0.9F);
    SDL_ResumeAudioStreamDevice(playerShipDestroyedStream_);
    return true;
}

Texture Game::loadTexture(const std::string& path)
{
    Texture texture{};
    texture.handle = IMG_LoadTexture(renderer_, path.c_str());
    if (!texture.handle) {
        std::fprintf(stderr, "Could not load texture %s: %s\n", path.c_str(), SDL_GetError());
        return texture;
    }

    SDL_GetTextureSize(texture.handle, &texture.width, &texture.height);
    SDL_SetTextureScaleMode(texture.handle, SDL_SCALEMODE_LINEAR);
    return texture;
}

std::vector<Texture> Game::loadBreakFrames(const std::string& directory, int frameCount)
{
    std::vector<Texture> frames;
    frames.reserve(static_cast<std::size_t>(frameCount));

    for (int frame = 0; frame < frameCount; ++frame) {
        const std::string suffix = frame < 10 ? "0" + std::to_string(frame) : std::to_string(frame);
        Texture texture = loadTexture(assetPath(directory + "/frame_" + suffix + ".png"));
        if (!texture.handle) {
            return {};
        }
        frames.push_back(texture);
    }

    return frames;
}

void Game::destroyAssets()
{
    auto destroy = [](Texture& texture) {
        if (texture.handle) {
            SDL_DestroyTexture(texture.handle);
            texture = {};
        }
    };

    destroy(shipTexture_);
    destroy(shipThrustTexture_);
    destroy(enemySaucerTexture_);
    destroy(logoTexture_);
    for (auto& texture : largeAsteroids_) {
        destroy(texture);
    }
    for (auto& texture : mediumAsteroids_) {
        destroy(texture);
    }
    for (auto& texture : smallAsteroids_) {
        destroy(texture);
    }
    for (auto& texture : largeBreakFrames_) {
        destroy(texture);
    }
    for (auto& texture : mediumBreakFrames_) {
        destroy(texture);
    }
    for (auto& texture : smallBreakFrames_) {
        destroy(texture);
    }
    for (auto& texture : saucerExplosionFrames_) {
        destroy(texture);
    }
    for (auto& texture : playerExplosionFrames_) {
        destroy(texture);
    }
}

void Game::destroyAudio()
{
    if (backgroundBeatStream_) {
        SDL_DestroyAudioStream(backgroundBeatStream_);
        backgroundBeatStream_ = nullptr;
    }
    if (enemySaucerStream_) {
        SDL_DestroyAudioStream(enemySaucerStream_);
        enemySaucerStream_ = nullptr;
    }
    if (enemySaucerDestroyedStream_) {
        SDL_DestroyAudioStream(enemySaucerDestroyedStream_);
        enemySaucerDestroyedStream_ = nullptr;
    }
    if (playerShipDestroyedStream_) {
        SDL_DestroyAudioStream(playerShipDestroyedStream_);
        playerShipDestroyedStream_ = nullptr;
    }
    if (thrustStream_) {
        SDL_DestroyAudioStream(thrustStream_);
        thrustStream_ = nullptr;
    }
    if (missileFireStream_) {
        SDL_DestroyAudioStream(missileFireStream_);
        missileFireStream_ = nullptr;
    }
    if (asteroidHitStream_) {
        SDL_DestroyAudioStream(asteroidHitStream_);
        asteroidHitStream_ = nullptr;
    }
    if (missileFireBuffer_) {
        SDL_free(missileFireBuffer_);
        missileFireBuffer_ = nullptr;
        missileFireBufferLength_ = 0;
    }
    if (thrustBuffer_) {
        SDL_free(thrustBuffer_);
        thrustBuffer_ = nullptr;
        thrustBufferLength_ = 0;
    }
    if (asteroidHitBuffer_) {
        SDL_free(asteroidHitBuffer_);
        asteroidHitBuffer_ = nullptr;
        asteroidHitBufferLength_ = 0;
    }
    if (enemySaucerBuffer_) {
        SDL_free(enemySaucerBuffer_);
        enemySaucerBuffer_ = nullptr;
        enemySaucerBufferLength_ = 0;
    }
    if (enemySaucerDestroyedBuffer_) {
        SDL_free(enemySaucerDestroyedBuffer_);
        enemySaucerDestroyedBuffer_ = nullptr;
        enemySaucerDestroyedBufferLength_ = 0;
    }
    if (playerShipDestroyedBuffer_) {
        SDL_free(playerShipDestroyedBuffer_);
        playerShipDestroyedBuffer_ = nullptr;
        playerShipDestroyedBufferLength_ = 0;
    }
}

void Game::resetGame()
{
    input_ = {};
    ship_ = {};
    bullets_.clear();
    enemyBullets_.clear();
    asteroids_.clear();
    breakAnimations_.clear();
    saucerExplosions_.clear();
    playerExplosions_.clear();
    enemySaucer_ = {};
    enemySaucerSpawnQueued_ = false;
    enemySaucerSpawnedThisWave_ = false;
    score_ = 0;
    lives_ = 5;
    wave_ = 1;
    asteroidsDestroyedThisWave_ = 0;
    largeAsteroidsDestroyedThisWave_ = 0;
    enemySaucerPassesThisWave_ = 0;
    enemySaucerSpawnDelaySeconds_ = 0.0F;
    fireCooldownSeconds_ = 0.0F;
    backgroundBeatSeconds_ = 0.2F;
    spawnWave();
    state_ = State::Playing;
}

void Game::jumpToWave(int wave)
{
    wave_ = std::max(1, wave);
    bullets_.clear();
    enemyBullets_.clear();
    asteroids_.clear();
    breakAnimations_.clear();
    saucerExplosions_.clear();
    playerExplosions_.clear();
    enemySaucer_ = {};
    stopEnemySaucerSound();
    ship_.position = {kWindowWidth / 2.0F, kWindowHeight / 2.0F};
    ship_.velocity = {};
    ship_.angleDegrees = 0.0F;
    ship_.invulnerableSeconds = 1.0F;
    fireCooldownSeconds_ = 0.0F;
    thrustAudioGraceSeconds_ = 0.0F;
    thrusting_ = false;
    updateThrustSound();
    spawnWave();
    state_ = State::Playing;
}

void Game::spawnWave()
{
    asteroidsDestroyedThisWave_ = 0;
    largeAsteroidsDestroyedThisWave_ = 0;
    enemySaucerPassesThisWave_ = 0;
    enemyBullets_.clear();
    enemySaucer_ = {};
    stopEnemySaucerSound();
    enemySaucerSpawnQueued_ = false;
    enemySaucerSpawnedThisWave_ = false;
    enemySaucerSpawnDelaySeconds_ = 0.0F;
    backgroundBeatSeconds_ = kBackgroundBeatMaxInterval;
    const int asteroidCount = kBaseAsteroids + wave_;
    waveStartingAsteroids_ = asteroidCount;
    waveStartingLargeAsteroids_ = asteroidCount;
    for (int i = 0; i < asteroidCount; ++i) {
        Vec2 position{};
        do {
            position = {static_cast<float>(randomInt(0, kWindowWidth)), static_cast<float>(randomInt(0, kWindowHeight))};
        } while (length(position - Vec2{kWindowWidth / 2.0F, kWindowHeight / 2.0F}) < 170.0F);
        spawnAsteroid(position, AsteroidSize::Large);
    }
}

void Game::spawnAsteroid(Vec2 position, AsteroidSize size)
{
    const float speedMultiplier = 1.0F + ((wave_ - 1) * 0.08F);
    const float angle = randomFloat(0.0F, 360.0F);
    const float speed = randomFloat(45.0F, 105.0F) * speedMultiplier;

    Asteroid asteroid{};
    asteroid.position = position;
    asteroid.velocity = {std::cos(degreesToRadians(angle)) * speed, std::sin(degreesToRadians(angle)) * speed};
    asteroid.size = size;
    asteroid.textureIndex = randomInt(0, 2);
    asteroid.angleDegrees = randomFloat(0.0F, 360.0F);
    asteroid.spinDegreesPerSecond = randomFloat(-72.0F, 72.0F);

    if (size == AsteroidSize::Large) {
        asteroid.radius = 50.0F;
    } else if (size == AsteroidSize::Medium) {
        asteroid.radius = 28.0F;
    } else {
        asteroid.radius = 16.0F;
    }

    asteroids_.push_back(asteroid);
}

void Game::splitAsteroid(const Asteroid& asteroid)
{
    if (asteroid.size == AsteroidSize::Small) {
        return;
    }

    const AsteroidSize childSize = asteroid.size == AsteroidSize::Large ? AsteroidSize::Medium : AsteroidSize::Small;
    spawnAsteroid(asteroid.position, childSize);
    spawnAsteroid(asteroid.position, childSize);
}

void Game::spawnBreakAnimation(const Asteroid& asteroid)
{
    breakAnimations_.push_back({asteroid.position, asteroid.size, 0.0F});
}

void Game::spawnSaucerExplosion(Vec2 position)
{
    saucerExplosions_.push_back({position, 0.0F});
}

void Game::spawnPlayerExplosion(Vec2 position)
{
    playerExplosions_.push_back({position, 0.0F});
}

void Game::fireBullet()
{
    if (state_ != State::Playing || fireCooldownSeconds_ > 0.0F) {
        return;
    }

    const Vec2 direction = forwardVector();
    bullets_.push_back({ship_.position + (direction * 32.0F), direction * kBulletSpeed, kBulletMaxLifeSeconds, 3.0F});
    fireCooldownSeconds_ = kFireCooldownSeconds;
    playMissileFireSound();
}

void Game::playAsteroidHitSound(AsteroidSize size)
{
    if (!asteroidHitStream_ || !asteroidHitBuffer_ || asteroidHitBufferLength_ == 0) {
        return;
    }

    float gain = 1.0F;
    if (size == AsteroidSize::Medium) {
        gain = 0.5F;
    } else if (size == AsteroidSize::Small) {
        gain = 0.25F;
    }

    SDL_SetAudioStreamGain(asteroidHitStream_, gain);
    SDL_ClearAudioStream(asteroidHitStream_);
    SDL_PutAudioStreamData(asteroidHitStream_, asteroidHitBuffer_, static_cast<int>(asteroidHitBufferLength_));
}

void Game::playMissileFireSound()
{
    if (!missileFireStream_ || !missileFireBuffer_ || missileFireBufferLength_ == 0) {
        return;
    }

    SDL_ClearAudioStream(missileFireStream_);
    SDL_PutAudioStreamData(missileFireStream_, missileFireBuffer_, static_cast<int>(missileFireBufferLength_));
}

void Game::playEnemySaucerDestroyedSound()
{
    if (!enemySaucerDestroyedStream_ || !enemySaucerDestroyedBuffer_ || enemySaucerDestroyedBufferLength_ == 0) {
        return;
    }

    SDL_ClearAudioStream(enemySaucerDestroyedStream_);
    SDL_PutAudioStreamData(enemySaucerDestroyedStream_, enemySaucerDestroyedBuffer_, static_cast<int>(enemySaucerDestroyedBufferLength_));
}

void Game::playPlayerShipDestroyedSound()
{
    if (!playerShipDestroyedStream_ || !playerShipDestroyedBuffer_ || playerShipDestroyedBufferLength_ == 0) {
        return;
    }

    SDL_ClearAudioStream(playerShipDestroyedStream_);
    SDL_PutAudioStreamData(playerShipDestroyedStream_, playerShipDestroyedBuffer_, static_cast<int>(playerShipDestroyedBufferLength_));
}

void Game::updateThrustSound()
{
    if (!thrustStream_ || !thrustBuffer_ || thrustBufferLength_ == 0) {
        return;
    }

    if (!thrusting_) {
        SDL_ClearAudioStream(thrustStream_);
        return;
    }

    if (SDL_GetAudioStreamQueued(thrustStream_) < static_cast<int>(thrustBufferLength_ / 2)) {
        SDL_PutAudioStreamData(thrustStream_, thrustBuffer_, static_cast<int>(thrustBufferLength_));
    }
}

void Game::updateBackgroundBeat(float deltaSeconds)
{
    if (!backgroundBeatStream_) {
        return;
    }

    backgroundBeatSeconds_ -= deltaSeconds;
    if (backgroundBeatSeconds_ > 0.0F) {
        return;
    }

    playBackgroundBeatPulse();

    const float speedUp = static_cast<float>(std::min(16, asteroidsDestroyedThisWave_)) * 0.035F;
    backgroundBeatSeconds_ = std::clamp(kBackgroundBeatMaxInterval - speedUp, kBackgroundBeatMinInterval, kBackgroundBeatMaxInterval);
}

void Game::playBackgroundBeatPulse()
{
    if (!backgroundBeatStream_ || backgroundBeatLow_.empty() || backgroundBeatHigh_.empty()) {
        return;
    }

    static bool useHighPulse = false;
    const auto& pulse = useHighPulse ? backgroundBeatHigh_ : backgroundBeatLow_;
    useHighPulse = !useHighPulse;
    SDL_PutAudioStreamData(
        backgroundBeatStream_,
        pulse.data(),
        static_cast<int>(pulse.size() * sizeof(Sint16))
    );
}

void Game::updateEnemySaucer(float deltaSeconds)
{
    const int largeSpawnThreshold = std::max(1, waveStartingLargeAsteroids_ / 2);
    const int totalSpawnThreshold = std::max(1, waveStartingAsteroids_ / 2);
    const bool clearedEnoughLargeAsteroids = largeAsteroidsDestroyedThisWave_ >= largeSpawnThreshold;
    const bool clearedEnoughAsteroids = asteroidsDestroyedThisWave_ >= totalSpawnThreshold;
    if (!enemySaucer_.active && !enemySaucerSpawnQueued_ && (clearedEnoughLargeAsteroids || clearedEnoughAsteroids)) {
        enemySaucerSpawnQueued_ = true;
        if (enemySaucerPassesThisWave_ == 0) {
            enemySaucerSpawnDelaySeconds_ = kEnemySaucerFirstArrivalDelay;
        } else {
            const float baseDelay = kEnemySaucerFirstRespawnDelay - (static_cast<float>(enemySaucerPassesThisWave_ - 1) * kEnemySaucerRespawnDelayStep);
            enemySaucerSpawnDelaySeconds_ = std::max(kEnemySaucerMinRespawnDelay, baseDelay);
        }
    }

    if (enemySaucerSpawnQueued_) {
        enemySaucerSpawnDelaySeconds_ -= deltaSeconds;
        if (enemySaucerSpawnDelaySeconds_ <= 0.0F) {
            spawnEnemySaucer();
            enemySaucerSpawnQueued_ = false;
            enemySaucerSpawnedThisWave_ = true;
        }
    }

    if (enemySaucer_.active) {
        enemySaucer_.position += enemySaucer_.velocity * deltaSeconds;
        enemySaucer_.fireCooldownSeconds -= deltaSeconds;

        if (enemySaucer_.fireCooldownSeconds <= 0.0F) {
            fireEnemySaucerBullet();
            enemySaucer_.fireCooldownSeconds = randomFloat(kEnemySaucerFireCooldownMin, kEnemySaucerFireCooldownMax);
        }

        if (enemySaucer_.position.x < -enemySaucer_.radius) {
            enemySaucer_.position.x = kWindowWidth + enemySaucer_.radius;
        } else if (enemySaucer_.position.x > kWindowWidth + enemySaucer_.radius) {
            enemySaucer_.position.x = -enemySaucer_.radius;
        }

        if (enemySaucer_.position.y < kWindowHeight * 0.12F || enemySaucer_.position.y > kWindowHeight * 0.88F) {
            enemySaucer_.velocity.y *= -1.0F;
        }

        updateEnemySaucerSound();
    } else {
        stopEnemySaucerSound();
    }

    for (auto& bullet : enemyBullets_) {
        bullet.position += bullet.velocity * deltaSeconds;
        bullet.lifeSeconds -= deltaSeconds;
    }

    enemyBullets_.erase(std::remove_if(enemyBullets_.begin(), enemyBullets_.end(), [](const EnemyBullet& bullet) {
        return bullet.lifeSeconds <= 0.0F ||
            bullet.position.x < 0.0F ||
            bullet.position.x > kWindowWidth ||
            bullet.position.y < 0.0F ||
            bullet.position.y > kWindowHeight;
    }), enemyBullets_.end());

    for (std::size_t bulletIndex = 0; bulletIndex < enemyBullets_.size();) {
        bool bulletRemoved = false;
        for (std::size_t asteroidIndex = 0; asteroidIndex < asteroids_.size();) {
            if (!collides(enemyBullets_[bulletIndex].position, enemyBullets_[bulletIndex].radius, asteroids_[asteroidIndex].position, asteroids_[asteroidIndex].radius)) {
                ++asteroidIndex;
                continue;
            }

            const Asteroid hit = asteroids_[asteroidIndex];
            spawnBreakAnimation(hit);
            splitAsteroid(hit);
            asteroids_.erase(asteroids_.begin() + static_cast<std::ptrdiff_t>(asteroidIndex));
            enemyBullets_.erase(enemyBullets_.begin() + static_cast<std::ptrdiff_t>(bulletIndex));
            if (hit.size == AsteroidSize::Large) {
                ++largeAsteroidsDestroyedThisWave_;
            }
            ++asteroidsDestroyedThisWave_;
            playAsteroidHitSound(hit.size);
            bulletRemoved = true;
            break;
        }

        if (!bulletRemoved) {
            ++bulletIndex;
        }
    }

    if (ship_.invulnerableSeconds <= 0.0F) {
        const auto hitShipBullet = std::find_if(enemyBullets_.begin(), enemyBullets_.end(), [this](const EnemyBullet& bullet) {
            return collides(ship_.position, ship_.radius, bullet.position, bullet.radius);
        });

        const bool saucerHitShip = enemySaucer_.active && collides(ship_.position, ship_.radius, enemySaucer_.position, enemySaucer_.radius);
        if (hitShipBullet != enemyBullets_.end() || saucerHitShip) {
            spawnPlayerExplosion(ship_.position);
            playPlayerShipDestroyedSound();
            --lives_;
            if (lives_ <= 0) {
                state_ = State::GameOver;
                stopEnemySaucerSound();
            } else {
                ship_.position = {kWindowWidth / 2.0F, kWindowHeight / 2.0F};
                ship_.velocity = {};
                ship_.angleDegrees = 0.0F;
                ship_.invulnerableSeconds = 2.0F;
                bullets_.clear();
                enemyBullets_.clear();
            }
        }
    }
}

void Game::spawnEnemySaucer()
{
    const bool enterFromLeft = randomInt(0, 1) == 0;
    const float y = randomFloat(kWindowHeight * 0.18F, kWindowHeight * 0.82F);
    const float x = enterFromLeft ? 36.0F : kWindowWidth - 36.0F;
    const float direction = enterFromLeft ? 1.0F : -1.0F;
    enemySaucer_ = {
        {x, y},
        {direction * kEnemySaucerSpeed, randomFloat(-22.0F, 22.0F)},
        26.0F,
        randomFloat(0.6F, 1.4F),
        true
    };
    ++enemySaucerPassesThisWave_;
    updateEnemySaucerSound();
}

void Game::fireEnemySaucerBullet()
{
    if (!enemySaucer_.active) {
        return;
    }

    Vec2 direction = normalized(ship_.position - enemySaucer_.position);
    const float aimError = degreesToRadians(randomFloat(-kEnemySaucerAimErrorDegrees, kEnemySaucerAimErrorDegrees));
    const float cosError = std::cos(aimError);
    const float sinError = std::sin(aimError);
    direction = {
        (direction.x * cosError) - (direction.y * sinError),
        (direction.x * sinError) + (direction.y * cosError)
    };
    enemyBullets_.push_back({enemySaucer_.position, direction * kEnemySaucerBulletSpeed, kEnemySaucerBulletLifeSeconds, 3.0F});
}

void Game::updateEnemySaucerSound()
{
    if (!enemySaucerStream_ || !enemySaucerBuffer_ || enemySaucerBufferLength_ == 0) {
        return;
    }

    if (SDL_GetAudioStreamQueued(enemySaucerStream_) < static_cast<int>(enemySaucerBufferLength_ / 3)) {
        SDL_PutAudioStreamData(enemySaucerStream_, enemySaucerBuffer_, static_cast<int>(enemySaucerBufferLength_));
    }
}

void Game::stopEnemySaucerSound()
{
    if (enemySaucerStream_) {
        SDL_ClearAudioStream(enemySaucerStream_);
    }
}

void Game::handleEvents(bool& running)
{
    SDL_Event event{};
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            running = false;
        }

        if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP) {
            const bool pressed = event.type == SDL_EVENT_KEY_DOWN;
            const SDL_Scancode scancode = event.key.scancode;
            const SDL_Keycode key = event.key.key;

            if (!isModifier(scancode)) {
                lastScancode_ = scancode;
                lastKeycode_ = key;
                lastKeyPressed_ = pressed;
                lastScancodeName_ = SDL_GetScancodeName(scancode);
                lastKeyName_ = SDL_GetKeyName(key);
                if (lastScancodeName_.empty()) {
                    lastScancodeName_ = "UNKNOWN";
                }
                if (lastKeyName_.empty()) {
                    lastKeyName_ = "UNKNOWN";
                }
            }

            if (isKey(scancode, key, SDL_SCANCODE_LEFT, SDLK_LEFT) || isKey(scancode, key, SDL_SCANCODE_A, SDLK_A)) {
                input_.rotateLeft = pressed;
            } else if (isKey(scancode, key, SDL_SCANCODE_RIGHT, SDLK_RIGHT) || isKey(scancode, key, SDL_SCANCODE_D, SDLK_D)) {
                input_.rotateRight = pressed;
            } else if (isKey(scancode, key, SDL_SCANCODE_UP, SDLK_UP) || isKey(scancode, key, SDL_SCANCODE_W, SDLK_W)) {
                input_.thrust = pressed;
            } else if (isKey(scancode, key, SDL_SCANCODE_SPACE, SDLK_SPACE) || isKey(scancode, key, SDL_SCANCODE_F, SDLK_F) || isKey(scancode, key, SDL_SCANCODE_Z, SDLK_Z) || isKey(scancode, key, SDL_SCANCODE_X, SDLK_X)) {
                input_.fire = pressed;
            }
        }

        if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat) {
            const SDL_Scancode scancode = event.key.scancode;
            const SDL_Keycode key = event.key.key;
            if (isKey(scancode, key, SDL_SCANCODE_ESCAPE, SDLK_ESCAPE)) {
                running = false;
            } else if (isKey(scancode, key, SDL_SCANCODE_F10, SDLK_F10)) {
                diagnosticsEnabled_ = !diagnosticsEnabled_;
            } else if (state_ != State::Splash && state_ != State::Start && isKey(scancode, key, SDL_SCANCODE_F6, SDLK_F6)) {
                jumpToWave(wave_ + 1);
            } else if (state_ != State::Splash && state_ != State::Start && isKey(scancode, key, SDL_SCANCODE_F5, SDLK_F5)) {
                jumpToWave(wave_ - 1);
            } else if (state_ == State::Playing && isKey(scancode, key, SDL_SCANCODE_F9, SDLK_F9)) {
                spawnEnemySaucer();
                enemySaucerSpawnQueued_ = false;
                enemySaucerSpawnedThisWave_ = true;
            } else if (state_ == State::Splash && (isKey(scancode, key, SDL_SCANCODE_RETURN, SDLK_RETURN) || scancode == SDL_SCANCODE_KP_ENTER || isKey(scancode, key, SDL_SCANCODE_SPACE, SDLK_SPACE) || isKey(scancode, key, SDL_SCANCODE_F, SDLK_F))) {
                state_ = State::Start;
            } else if (state_ == State::Start && (isKey(scancode, key, SDL_SCANCODE_RETURN, SDLK_RETURN) || scancode == SDL_SCANCODE_KP_ENTER || isKey(scancode, key, SDL_SCANCODE_SPACE, SDLK_SPACE) || isKey(scancode, key, SDL_SCANCODE_F, SDLK_F))) {
                resetGame();
            } else if (state_ == State::Playing && isKey(scancode, key, SDL_SCANCODE_P, SDLK_P)) {
                state_ = State::Paused;
            } else if (state_ == State::Paused && isKey(scancode, key, SDL_SCANCODE_P, SDLK_P)) {
                state_ = State::Playing;
            } else if (state_ == State::GameOver && (isKey(scancode, key, SDL_SCANCODE_RETURN, SDLK_RETURN) || scancode == SDL_SCANCODE_KP_ENTER || isKey(scancode, key, SDL_SCANCODE_R, SDLK_R) || isKey(scancode, key, SDL_SCANCODE_SPACE, SDLK_SPACE) || isKey(scancode, key, SDL_SCANCODE_F, SDLK_F))) {
                resetGame();
            }

            if (state_ == State::Playing) {
                if (isKey(scancode, key, SDL_SCANCODE_LEFT, SDLK_LEFT) || isKey(scancode, key, SDL_SCANCODE_A, SDLK_A)) {
                    rotateLeftGraceSeconds_ = kRotationGraceSeconds;
                } else if (isKey(scancode, key, SDL_SCANCODE_RIGHT, SDLK_RIGHT) || isKey(scancode, key, SDL_SCANCODE_D, SDLK_D)) {
                    rotateRightGraceSeconds_ = kRotationGraceSeconds;
                } else if (isKey(scancode, key, SDL_SCANCODE_UP, SDLK_UP) || isKey(scancode, key, SDL_SCANCODE_W, SDLK_W)) {
                    thrustAudioGraceSeconds_ = kThrustAudioGraceSeconds;
                    ship_.velocity += forwardVector() * kTapThrustImpulse;
                } else if (isKey(scancode, key, SDL_SCANCODE_SPACE, SDLK_SPACE) || isKey(scancode, key, SDL_SCANCODE_F, SDLK_F) || isKey(scancode, key, SDL_SCANCODE_Z, SDLK_Z) || isKey(scancode, key, SDL_SCANCODE_X, SDLK_X)) {
                    fireBullet();
                }
            }
        }
    }
}

void Game::update(float deltaSeconds)
{
    if (state_ == State::Splash) {
        splashSecondsRemaining_ -= deltaSeconds;
        if (splashSecondsRemaining_ <= 0.0F) {
            state_ = State::Start;
        }
    } else if (state_ == State::Playing) {
        updatePlaying(deltaSeconds);
    } else {
        thrusting_ = false;
        updateThrustSound();
        if (backgroundBeatStream_) {
            SDL_ClearAudioStream(backgroundBeatStream_);
        }
        stopEnemySaucerSound();
    }
}

void Game::updatePlaying(float deltaSeconds)
{
    thrustAudioGraceSeconds_ = std::max(0.0F, thrustAudioGraceSeconds_ - deltaSeconds);
    thrusting_ = input_.thrust || thrustAudioGraceSeconds_ > 0.0F;
    updateThrustSound();
    updateBackgroundBeat(deltaSeconds);
    rotateLeftGraceSeconds_ = std::max(0.0F, rotateLeftGraceSeconds_ - deltaSeconds);
    rotateRightGraceSeconds_ = std::max(0.0F, rotateRightGraceSeconds_ - deltaSeconds);

    if (input_.rotateLeft || rotateLeftGraceSeconds_ > 0.0F) {
        ship_.angleDegrees -= kRotationDegreesPerSecond * deltaSeconds;
    }
    if (input_.rotateRight || rotateRightGraceSeconds_ > 0.0F) {
        ship_.angleDegrees += kRotationDegreesPerSecond * deltaSeconds;
    }
    if (input_.thrust) {
        ship_.velocity += forwardVector() * (kAcceleration * deltaSeconds);
    }

    const float speed = length(ship_.velocity);
    if (speed > kMaxShipSpeed) {
        ship_.velocity = normalized(ship_.velocity) * kMaxShipSpeed;
    }

    ship_.velocity *= std::pow(kDragPerSecond, deltaSeconds);
    ship_.position += ship_.velocity * deltaSeconds;
    wrap(ship_.position);

    if (ship_.invulnerableSeconds > 0.0F) {
        ship_.invulnerableSeconds -= deltaSeconds;
    }

    fireCooldownSeconds_ = std::max(0.0F, fireCooldownSeconds_ - deltaSeconds);
    if (input_.fire && fireCooldownSeconds_ <= 0.0F) {
        fireBullet();
    }

    for (auto& bullet : bullets_) {
        const Vec2 previousPosition = bullet.position;
        bullet.position += bullet.velocity * deltaSeconds;
        bullet.maxLifeSeconds -= deltaSeconds;

        if (!bullet.hasWrapped) {
            bool crossedHorizontalEdge = false;
            bool crossedVerticalEdge = false;

            if (bullet.position.x < 0.0F) {
                bullet.position.x += kWindowWidth;
                crossedHorizontalEdge = true;
            } else if (bullet.position.x > kWindowWidth) {
                bullet.position.x -= kWindowWidth;
                crossedHorizontalEdge = true;
            }

            if (bullet.position.y < 0.0F) {
                bullet.position.y += kWindowHeight;
                crossedVerticalEdge = true;
            } else if (bullet.position.y > kWindowHeight) {
                bullet.position.y -= kWindowHeight;
                crossedVerticalEdge = true;
            }

            if (crossedHorizontalEdge || crossedVerticalEdge) {
                bullet.hasWrapped = true;
                const float wrappedDistance = crossedHorizontalEdge ? kWindowWidth : kWindowHeight;
                bullet.wrappedDistanceRemaining = wrappedDistance * kBulletWrappedTravelFraction;
            }
        } else {
            bullet.wrappedDistanceRemaining -= length(bullet.position - previousPosition);
        }
    }
    bullets_.erase(std::remove_if(bullets_.begin(), bullets_.end(), [](const Bullet& bullet) {
        return bullet.maxLifeSeconds <= 0.0F || (bullet.hasWrapped && bullet.wrappedDistanceRemaining <= 0.0F);
    }), bullets_.end());

    if (enemySaucer_.active) {
        for (std::size_t bulletIndex = 0; bulletIndex < bullets_.size();) {
            if (!collides(bullets_[bulletIndex].position, bullets_[bulletIndex].radius, enemySaucer_.position, enemySaucer_.radius)) {
                ++bulletIndex;
                continue;
            }

            const Vec2 destroyedPosition = enemySaucer_.position;
            bullets_.erase(bullets_.begin() + static_cast<std::ptrdiff_t>(bulletIndex));
            enemySaucer_ = {};
            stopEnemySaucerSound();
            spawnSaucerExplosion(destroyedPosition);
            playEnemySaucerDestroyedSound();
            score_ += kEnemySaucerScore;
            break;
        }
    }

    for (auto& asteroid : asteroids_) {
        asteroid.position += asteroid.velocity * deltaSeconds;
        asteroid.angleDegrees += asteroid.spinDegreesPerSecond * deltaSeconds;
        wrap(asteroid.position, asteroid.radius);
    }

    updateBreakAnimations(deltaSeconds);
    updateSaucerExplosions(deltaSeconds);
    updatePlayerExplosions(deltaSeconds);

    for (std::size_t bulletIndex = 0; bulletIndex < bullets_.size();) {
        bool bulletRemoved = false;
        for (std::size_t asteroidIndex = 0; asteroidIndex < asteroids_.size();) {
            if (!collides(bullets_[bulletIndex].position, bullets_[bulletIndex].radius, asteroids_[asteroidIndex].position, asteroids_[asteroidIndex].radius)) {
                ++asteroidIndex;
                continue;
            }

            const Asteroid hit = asteroids_[asteroidIndex];
            spawnBreakAnimation(hit);
            splitAsteroid(hit);
            asteroids_.erase(asteroids_.begin() + static_cast<std::ptrdiff_t>(asteroidIndex));
            bullets_.erase(bullets_.begin() + static_cast<std::ptrdiff_t>(bulletIndex));
            score_ += 100;
            if (hit.size == AsteroidSize::Large) {
                ++largeAsteroidsDestroyedThisWave_;
            }
            ++asteroidsDestroyedThisWave_;
            playAsteroidHitSound(hit.size);
            bulletRemoved = true;
            break;
        }

        if (!bulletRemoved) {
            ++bulletIndex;
        }
    }

    updateEnemySaucer(deltaSeconds);

    if (ship_.invulnerableSeconds <= 0.0F) {
        const auto hitShip = std::find_if(asteroids_.begin(), asteroids_.end(), [this](const Asteroid& asteroid) {
            return collides(ship_.position, ship_.radius, asteroid.position, asteroid.radius);
        });

        if (hitShip != asteroids_.end()) {
            spawnPlayerExplosion(ship_.position);
            playPlayerShipDestroyedSound();
            --lives_;
            if (lives_ <= 0) {
                state_ = State::GameOver;
            } else {
                ship_.position = {kWindowWidth / 2.0F, kWindowHeight / 2.0F};
                ship_.velocity = {};
                ship_.angleDegrees = 0.0F;
                ship_.invulnerableSeconds = 2.0F;
                bullets_.clear();
            }
        }
    }

    if (asteroids_.empty()) {
        ++wave_;
        score_ += 500;
        bullets_.clear();
        enemyBullets_.clear();
        enemySaucer_ = {};
        stopEnemySaucerSound();
        spawnWave();
    }
}

void Game::updateBreakAnimations(float deltaSeconds)
{
    for (auto& animation : breakAnimations_) {
        animation.elapsedSeconds += deltaSeconds;
    }

    breakAnimations_.erase(std::remove_if(breakAnimations_.begin(), breakAnimations_.end(), [this](const BreakAnimation& animation) {
        const std::vector<Texture>* frames = &largeBreakFrames_;
        if (animation.size == AsteroidSize::Medium) {
            frames = &mediumBreakFrames_;
        } else if (animation.size == AsteroidSize::Small) {
            frames = &smallBreakFrames_;
        }

        return frames->empty() || animation.elapsedSeconds >= static_cast<float>(frames->size()) * kBreakAnimationFrameSeconds;
    }), breakAnimations_.end());
}

void Game::updateSaucerExplosions(float deltaSeconds)
{
    for (auto& explosion : saucerExplosions_) {
        explosion.elapsedSeconds += deltaSeconds;
    }

    saucerExplosions_.erase(std::remove_if(saucerExplosions_.begin(), saucerExplosions_.end(), [this](const SaucerExplosion& explosion) {
        return saucerExplosionFrames_.empty() || explosion.elapsedSeconds >= static_cast<float>(saucerExplosionFrames_.size()) * kSaucerExplosionFrameSeconds;
    }), saucerExplosions_.end());
}

void Game::updatePlayerExplosions(float deltaSeconds)
{
    for (auto& explosion : playerExplosions_) {
        explosion.elapsedSeconds += deltaSeconds;
    }

    playerExplosions_.erase(std::remove_if(playerExplosions_.begin(), playerExplosions_.end(), [this](const PlayerExplosion& explosion) {
        return playerExplosionFrames_.empty() || explosion.elapsedSeconds >= static_cast<float>(playerExplosionFrames_.size()) * kPlayerExplosionFrameSeconds;
    }), playerExplosions_.end());
}

void Game::render()
{
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
    SDL_RenderClear(renderer_);

    if (state_ == State::Splash) {
        renderSplash();
    } else if (state_ == State::Start) {
        renderStart();
    } else if (state_ == State::Playing) {
        renderPlaying();
    } else if (state_ == State::Paused) {
        renderPlaying();
        renderPaused();
    } else if (state_ == State::GameOver) {
        renderPlaying();
        renderGameOver();
    }

    SDL_RenderPresent(renderer_);
}

void Game::renderSplash()
{
    constexpr float logoWidth = 270.0F;
    const float aspect = logoTexture_.height > 0.0F ? logoTexture_.width / logoTexture_.height : 1.0F;
    const float logoHeight = logoWidth / aspect;
    SDL_FRect logoRect{
        (kWindowWidth - logoWidth) / 2.0F,
        kWindowHeight * 0.16F,
        logoWidth,
        logoHeight
    };

    SDL_RenderTexture(renderer_, logoTexture_.handle, nullptr, &logoRect);
    drawCenteredText("PRESENTS", kWindowHeight * 0.58F, 3, {255, 255, 255, 255});
    drawCenteredText("RETRO ARCADE COLLECTION", kWindowHeight * 0.66F, 3, {180, 220, 255, 255});
    drawCenteredText("PRESS ENTER", kWindowHeight * 0.80F, 2, {255, 210, 60, 255});
}

void Game::renderStart()
{
    drawCenteredText("ASTEROIDS", kWindowHeight * 0.20F, 6, {255, 255, 255, 255});
    drawCenteredText("PRESS ENTER", kWindowHeight * 0.44F, 3, {255, 210, 60, 255});
    drawCenteredText("ARROWS MOVE  F FIRE  P PAUSE", kWindowHeight * 0.54F, 2, {180, 220, 255, 255});
}

void Game::renderPlaying()
{
    for (auto& asteroid : asteroids_) {
        Texture& texture = asteroidTexture(asteroid);
        const float diameter = asteroid.radius * 2.0F;
        SDL_FRect dst{asteroid.position.x - asteroid.radius, asteroid.position.y - asteroid.radius, diameter, diameter};
        SDL_RenderTextureRotated(renderer_, texture.handle, nullptr, &dst, asteroid.angleDegrees, nullptr, SDL_FLIP_NONE);
    }

    renderBreakAnimations();
    renderSaucerExplosions();
    renderPlayerExplosions();
    renderEnemySaucer();

    SDL_SetRenderDrawColor(renderer_, 255, 230, 70, 255);
    for (const auto& bullet : bullets_) {
        SDL_FRect rect{bullet.position.x - 2.0F, bullet.position.y - 2.0F, 4.0F, 4.0F};
        SDL_RenderFillRect(renderer_, &rect);
    }

    SDL_SetRenderDrawColor(renderer_, 255, 80, 80, 255);
    for (const auto& bullet : enemyBullets_) {
        SDL_FRect rect{bullet.position.x - 2.0F, bullet.position.y - 2.0F, 4.0F, 4.0F};
        SDL_RenderFillRect(renderer_, &rect);
    }

    const bool respawnDelayVisible = ship_.invulnerableSeconds <= 1.35F;
    const auto blinkVisible = ship_.invulnerableSeconds <= 0.0F || (respawnDelayVisible && (static_cast<int>(SDL_GetTicks() / 120) % 2 == 0));
    if (lives_ > 0 && blinkVisible) {
        renderShip();
    }

    renderHud();
}

void Game::renderPaused()
{
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 190);
    SDL_FRect panel{0.0F, kWindowHeight * 0.38F, static_cast<float>(kWindowWidth), 130.0F};
    SDL_RenderFillRect(renderer_, &panel);
    drawCenteredText("PAUSED", kWindowHeight * 0.42F, 4, {255, 255, 255, 255});
    drawCenteredText("PRESS P", kWindowHeight * 0.50F, 2, {255, 210, 60, 255});
}

void Game::renderGameOver()
{
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 210);
    SDL_FRect panel{0.0F, kWindowHeight * 0.30F, static_cast<float>(kWindowWidth), 220.0F};
    SDL_RenderFillRect(renderer_, &panel);
    drawCenteredText("GAME OVER", kWindowHeight * 0.35F, 5, {255, 255, 255, 255});
    drawCenteredText("SCORE " + std::to_string(score_), kWindowHeight * 0.45F, 3, {255, 210, 60, 255});
    drawCenteredText("PRESS R", kWindowHeight * 0.53F, 2, {180, 220, 255, 255});
}

void Game::renderShip()
{
    Texture& texture = thrusting_ ? shipThrustTexture_ : shipTexture_;
    SDL_FRect dst{ship_.position.x - 34.0F, ship_.position.y - 25.0F, 68.0F, 50.0F};
    SDL_RenderTextureRotated(renderer_, texture.handle, nullptr, &dst, ship_.angleDegrees + 90.0F, nullptr, SDL_FLIP_NONE);
}

void Game::renderHud()
{
    drawText("SCORE:" + std::to_string(score_), 12.0F, 12.0F, 2, {255, 255, 255, 255});
    drawText("LIVES:" + std::to_string(lives_), 12.0F, 42.0F, 2, {255, 255, 255, 255});
    drawText("WAVE:" + std::to_string(wave_), 12.0F, 72.0F, 2, {255, 255, 255, 255});
    if (ship_.invulnerableSeconds > 0.0F) {
        drawText("PROTECTED", 12.0F, 102.0F, 2, {255, 210, 60, 255});
    }

    if (diagnosticsEnabled_) {
        renderDiagnostics();
    }
}

void Game::renderDiagnostics()
{
    drawText(
        "SAUCER:" + std::string(enemySaucer_.active ? "ON" : (enemySaucerSpawnQueued_ ? "WAIT" : "OFF")) +
            " PASS:" + std::to_string(enemySaucerPassesThisWave_),
        12.0F,
        static_cast<float>(kWindowHeight - 106),
        2,
        {120, 220, 255, 255}
    );
    drawText(
        "LARGE:" + std::to_string(largeAsteroidsDestroyedThisWave_) + "-" + std::to_string(waveStartingLargeAsteroids_),
        12.0F,
        static_cast<float>(kWindowHeight - 132),
        2,
        {120, 220, 255, 255}
    );
    drawText(
        "LEFT:" + std::to_string(input_.rotateLeft ? 1 : 0) +
            " RIGHT:" + std::to_string(input_.rotateRight ? 1 : 0) +
            " UP:" + std::to_string(input_.thrust ? 1 : 0) +
            " FIRE:" + std::to_string(input_.fire ? 1 : 0),
        12.0F,
        static_cast<float>(kWindowHeight - 28),
        2,
        {120, 220, 255, 255}
    );
    drawText(
        "LAST:" + lastScancodeName_ +
            " " + (lastKeyPressed_ ? "DOWN" : "UP") +
            " SC:" + std::to_string(static_cast<int>(lastScancode_)) +
            " KEY:" + std::to_string(static_cast<int>(lastKeycode_)),
        12.0F,
        static_cast<float>(kWindowHeight - 54),
        2,
        {120, 220, 255, 255}
    );
    drawText(
        "KEYNAME:" + lastKeyName_,
        12.0F,
        static_cast<float>(kWindowHeight - 80),
        2,
        {120, 220, 255, 255}
    );
}

void Game::renderBreakAnimations()
{
    for (const auto& animation : breakAnimations_) {
        const std::vector<Texture>* frames = &largeBreakFrames_;
        if (animation.size == AsteroidSize::Medium) {
            frames = &mediumBreakFrames_;
        } else if (animation.size == AsteroidSize::Small) {
            frames = &smallBreakFrames_;
        }

        if (frames->empty()) {
            continue;
        }

        const std::size_t frameIndex = std::min(
            static_cast<std::size_t>(animation.elapsedSeconds / kBreakAnimationFrameSeconds),
            frames->size() - 1
        );
        const Texture& texture = (*frames)[frameIndex];
        SDL_FRect dst{
            animation.position.x - (texture.width / 2.0F),
            animation.position.y - (texture.height / 2.0F),
            texture.width,
            texture.height
        };
        SDL_RenderTexture(renderer_, texture.handle, nullptr, &dst);
    }
}

void Game::renderSaucerExplosions()
{
    for (const auto& explosion : saucerExplosions_) {
        if (saucerExplosionFrames_.empty()) {
            continue;
        }

        const std::size_t frameIndex = std::min(
            static_cast<std::size_t>(explosion.elapsedSeconds / kSaucerExplosionFrameSeconds),
            saucerExplosionFrames_.size() - 1
        );
        const Texture& texture = saucerExplosionFrames_[frameIndex];
        SDL_FRect dst{
            explosion.position.x - (texture.width / 2.0F),
            explosion.position.y - (texture.height / 2.0F),
            texture.width,
            texture.height
        };
        SDL_RenderTexture(renderer_, texture.handle, nullptr, &dst);
    }
}

void Game::renderPlayerExplosions()
{
    for (const auto& explosion : playerExplosions_) {
        if (playerExplosionFrames_.empty()) {
            continue;
        }

        const std::size_t frameIndex = std::min(
            static_cast<std::size_t>(explosion.elapsedSeconds / kPlayerExplosionFrameSeconds),
            playerExplosionFrames_.size() - 1
        );
        const Texture& texture = playerExplosionFrames_[frameIndex];
        SDL_FRect dst{
            explosion.position.x - (texture.width / 2.0F),
            explosion.position.y - (texture.height / 2.0F),
            texture.width,
            texture.height
        };
        SDL_RenderTexture(renderer_, texture.handle, nullptr, &dst);
    }
}

void Game::renderEnemySaucer()
{
    if (!enemySaucer_.active || !enemySaucerTexture_.handle) {
        return;
    }

    constexpr float saucerWidth = 64.0F;
    const float aspect = enemySaucerTexture_.height > 0.0F ? enemySaucerTexture_.width / enemySaucerTexture_.height : 1.0F;
    const float saucerHeight = saucerWidth / aspect;
    SDL_FRect dst{
        enemySaucer_.position.x - (saucerWidth / 2.0F),
        enemySaucer_.position.y - (saucerHeight / 2.0F),
        saucerWidth,
        saucerHeight
    };
    SDL_RenderTexture(renderer_, enemySaucerTexture_.handle, nullptr, &dst);
}

void Game::drawText(const std::string& text, float x, float y, int scale, SDL_Color color)
{
    float cursor = x;
    for (char raw : text) {
        const char glyph = static_cast<char>(std::toupper(static_cast<unsigned char>(raw)));
        drawGlyph(glyph, cursor, y, scale, color);
        cursor += 6.0F * static_cast<float>(scale);
    }
}

void Game::drawGlyph(char glyph, float x, float y, int scale, SDL_Color color)
{
    const auto& rows = glyphRows(glyph);
    SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
    for (int row = 0; row < 7; ++row) {
        for (int col = 0; col < 5; ++col) {
            if (rows[row][col] == '1') {
                SDL_FRect rect{x + static_cast<float>(col * scale), y + static_cast<float>(row * scale), static_cast<float>(scale), static_cast<float>(scale)};
                SDL_RenderFillRect(renderer_, &rect);
            }
        }
    }
}

void Game::drawCenteredText(const std::string& text, float y, int scale, SDL_Color color)
{
    const float width = static_cast<float>(text.size() * 6 * scale);
    drawText(text, (kWindowWidth - width) / 2.0F, y, scale, color);
}

void Game::wrap(Vec2& position, float margin)
{
    if (position.x < -margin) {
        position.x = kWindowWidth + margin;
    } else if (position.x > kWindowWidth + margin) {
        position.x = -margin;
    }

    if (position.y < -margin) {
        position.y = kWindowHeight + margin;
    } else if (position.y > kWindowHeight + margin) {
        position.y = -margin;
    }
}

bool Game::collides(Vec2 a, float ar, Vec2 b, float br) const
{
    return length(a - b) <= ar + br;
}

Vec2 Game::forwardVector() const
{
    const float radians = degreesToRadians(ship_.angleDegrees + 90.0F);
    return {std::cos(radians), std::sin(radians)};
}

float Game::randomFloat(float min, float max)
{
    std::uniform_real_distribution<float> distribution(min, max);
    return distribution(rng_);
}

int Game::randomInt(int min, int max)
{
    std::uniform_int_distribution<int> distribution(min, max);
    return distribution(rng_);
}

Texture& Game::asteroidTexture(const Asteroid& asteroid)
{
    if (asteroid.size == AsteroidSize::Large) {
        return largeAsteroids_[asteroid.textureIndex];
    }
    if (asteroid.size == AsteroidSize::Medium) {
        return mediumAsteroids_[asteroid.textureIndex];
    }
    return smallAsteroids_[asteroid.textureIndex];
}
