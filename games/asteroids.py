import math
import os
import random
import sys
from array import array
import pygame

WIDTH = 800
HEIGHT = 600
FPS = 60
INVULNERABLE_MS = 2000
BLINK_MS = 120
BASE_ASTEROIDS = 4
SPACE_SHIP_INTERVAL_MS = 60000
SPACE_SHIP_SHOT_MS = 500
SHIELD_USES_PER_WAVE = 2
SHIELD_DURATION_MS = 2000
THRUST_CHANNEL_ID = 1
BASE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ROCK_IMAGE_DIR = os.path.join(BASE_DIR, "Images", "Rocks")
SPACE_SHIP_IMAGE = os.path.join(BASE_DIR, "Images", "Space_ship", "Ship.png")
LOGO_IMAGE = os.path.join(BASE_DIR, "Images", "Logo", "BC_logo.png")
FONT_DIR = os.path.join(BASE_DIR, "Images", "Fonts")
ROCK_IMAGES = {}
SPACE_SHIP_BASE_IMAGE = None
LOGO_SURFACE = None
SOUNDS = {}
DIFFICULTIES = {
    "1": ("Slow", 0.5),
    "2": ("Medium", 0.75),
    "3": ("Fast", 1.0),
}


def load_atari_font(size):
    font_files = (
        "AtariClassic.ttf",
        "AtariClassic-Regular.ttf",
        "AtariST8x16SystemFont.ttf",
        "AtariSmall.ttf",
        "PressStart2P.ttf",
    )
    for font_file in font_files:
        path = os.path.join(FONT_DIR, font_file)
        if os.path.exists(path):
            return pygame.font.Font(path, size)

    font_name = None
    for system_font in ("atari classic", "atari", "arcadeclassic", "press start 2p", "courier new", "consolas"):
        font_name = pygame.font.match_font(system_font)
        if font_name:
            break
    return pygame.font.Font(font_name, size) if font_name else pygame.font.SysFont("consolas", size, bold=True)


def create_collision_mask(surface, brightness_threshold=120):
    mask = pygame.mask.Mask(surface.get_size())
    width, height = surface.get_size()
    for y in range(height):
        for x in range(width):
            color = surface.get_at((x, y))
            if color.a and (color.r + color.g + color.b) / 3 >= brightness_threshold:
                mask.set_at((x, y))
    return mask


def make_tone(frequency, duration_ms, volume=0.35):
    sample_rate = 44100
    sample_count = int(sample_rate * duration_ms / 1000)
    samples = array("h")
    for sample_index in range(sample_count):
        progress = sample_index / max(1, sample_count - 1)
        envelope = min(1.0, progress * 10) * (1.0 - progress)
        value = int(32767 * volume * envelope * math.sin(2 * math.pi * frequency * sample_index / sample_rate))
        samples.append(value)
    return pygame.mixer.Sound(buffer=samples.tobytes())


def make_thrust_sound():
    sample_rate = 44100
    duration_ms = 240
    sample_count = int(sample_rate * duration_ms / 1000)
    samples = array("h")
    for sample_index in range(sample_count):
        t = sample_index / sample_rate
        low = math.sin(2 * math.pi * 58 * t)
        pulse = 1.0 if math.sin(2 * math.pi * 32 * t) > 0 else -1.0
        crackle = random.uniform(-0.35, 0.35)
        value = int(32767 * 0.16 * ((low * 0.55) + (pulse * 0.22) + (crackle * 0.23)))
        samples.append(value)
    return pygame.mixer.Sound(buffer=samples.tobytes())


def setup_sounds():
    if not pygame.mixer.get_init():
        return
    try:
        pygame.mixer.set_reserved(2)
        SOUNDS.update(
            shoot=make_tone(880, 80, 0.25),
            hit=make_tone(170, 130, 0.45),
            lose_life=make_tone(95, 260, 0.5),
            wave=make_tone(520, 220, 0.3),
            pause=make_tone(330, 90, 0.25),
            space_ship=make_tone(720, 180, 0.28),
            thrust=make_thrust_sound(),
        )
    except pygame.error:
        SOUNDS.clear()


def play_sound(name):
    sound = SOUNDS.get(name)
    if sound:
        sound.play()


def set_thrust_sound(active):
    if not pygame.mixer.get_init():
        return
    sound = SOUNDS.get("thrust")
    if not sound:
        return
    channel = pygame.mixer.Channel(THRUST_CHANNEL_ID)
    if active:
        if not channel.get_busy():
            channel.play(sound, loops=-1)
    else:
        channel.stop()


def draw_advert_logo(screen):
    global LOGO_SURFACE
    if LOGO_SURFACE is None:
        logo = pygame.image.load(LOGO_IMAGE).convert_alpha()
        LOGO_SURFACE = pygame.transform.smoothscale(logo, (110, 110))
        LOGO_SURFACE.set_alpha(155)

    logo_rect = LOGO_SURFACE.get_rect(center=(WIDTH - 72, HEIGHT - 145))
    screen.blit(LOGO_SURFACE, logo_rect)


def set_window_icon():
    try:
        icon = pygame.image.load(LOGO_IMAGE).convert_alpha()
        icon = pygame.transform.smoothscale(icon, (32, 32))
        pygame.display.set_icon(icon)
    except pygame.error:
        pass


class Ship(pygame.sprite.Sprite):
    def __init__(self, speed_multiplier=1.0):
        super().__init__()
        self.no_thrust_image = pygame.image.load(
            os.path.join(BASE_DIR, "Images", "Ship", "ship_no_thrust.png")
        ).convert_alpha()
        self.thrust_image = pygame.image.load(
            os.path.join(BASE_DIR, "Images", "Ship", "ship_thrust.png")
        ).convert_alpha()
        self.orig_image = self.no_thrust_image
        self.image = self.orig_image

        self.rect = self.orig_image.get_rect(center=(WIDTH // 2, HEIGHT // 2))
        self.mask = create_collision_mask(self.image, 130)
        self.pos = pygame.math.Vector2(self.rect.center)
        self.vel = pygame.math.Vector2(0, 0)
        self.angle = 0
        self.acceleration = 0.2 * speed_multiplier
        self.max_speed = 8 * speed_multiplier
        self.speed_multiplier = speed_multiplier

    def forward_vector(self):
        rad = math.radians(self.angle)
        return pygame.math.Vector2(math.cos(rad), -math.sin(rad))

    def update(self):
        keys = pygame.key.get_pressed()
        if keys[pygame.K_LEFT]:
            self.angle += 4
        if keys[pygame.K_RIGHT]:
            self.angle -= 4
        if keys[pygame.K_UP]:
            self.vel += self.forward_vector() * self.acceleration
        self.vel *= 0.99
        if self.vel.length() > self.max_speed:
            self.vel.scale_to_length(self.max_speed)
        self.pos += self.vel
        self.rect.center = self.pos
        if keys[pygame.K_UP]:
            self.orig_image = self.thrust_image
        else:
            self.orig_image = self.no_thrust_image
        self.image = pygame.transform.rotate(self.orig_image, self.angle)
        self.rect = self.image.get_rect(center=self.rect.center)
        self.mask = create_collision_mask(self.image, 130)
        self.wrap()

    def wrap(self):
        if self.pos.x < 0:
            self.pos.x = WIDTH
        elif self.pos.x > WIDTH:
            self.pos.x = 0
        if self.pos.y < 0:
            self.pos.y = HEIGHT
        elif self.pos.y > HEIGHT:
            self.pos.y = 0

    def shoot(self):
        direction = self.forward_vector()
        nose_offset = self.orig_image.get_width() * 0.45
        bullet = Bullet(self.pos + direction * nose_offset, direction * 12 * self.speed_multiplier)
        return bullet


class Asteroid(pygame.sprite.Sprite):
    def __init__(self, pos, size, speed_multiplier=1.0):
        super().__init__()
        self.size = size
        self.base_image = self.load_rock_image(size)
        self.image = self.base_image
        self.rect = self.image.get_rect(center=pos)
        self.mask = create_collision_mask(self.image, 125)
        self.pos = pygame.math.Vector2(self.rect.center)
        self.angle = random.uniform(0, 360)
        self.spin = random.uniform(-1.2, 1.2)
        angle = random.uniform(0, 360)
        speed = random.uniform(1.5, 3.5) * speed_multiplier
        self.vel = pygame.math.Vector2(math.cos(math.radians(angle)), math.sin(math.radians(angle))) * speed

    @staticmethod
    def load_rock_image(size):
        if size > 35:
            prefix = "L"
        elif size > 20:
            prefix = "M"
        else:
            prefix = "S"

        if prefix not in ROCK_IMAGES:
            ROCK_IMAGES[prefix] = [
                pygame.image.load(os.path.join(ROCK_IMAGE_DIR, f"{prefix}_rock{index}.png")).convert_alpha()
                for index in range(1, 4)
            ]

        image = random.choice(ROCK_IMAGES[prefix])
        return pygame.transform.smoothscale(image, (size * 2, size * 2))

    def update(self):
        self.pos += self.vel
        self.angle = (self.angle + self.spin) % 360
        self.image = pygame.transform.rotate(self.base_image, self.angle)
        self.rect = self.image.get_rect(center=self.pos)
        self.mask = create_collision_mask(self.image, 125)
        self.wrap()

    def wrap(self):
        if self.pos.x < 0:
            self.pos.x = WIDTH
        elif self.pos.x > WIDTH:
            self.pos.x = 0
        if self.pos.y < 0:
            self.pos.y = HEIGHT
        elif self.pos.y > HEIGHT:
            self.pos.y = 0
        self.rect.center = self.pos


class SpaceShip(pygame.sprite.Sprite):
    def __init__(self, speed_multiplier=1.0):
        super().__init__()
        self.image = self.load_image()
        self.mask = create_collision_mask(self.image, 115)
        y = random.randint(80, HEIGHT - 120)
        from_left = random.choice((True, False))
        speed = random.uniform(2.0, 3.2) * speed_multiplier
        if from_left:
            self.pos = pygame.math.Vector2(-self.image.get_width(), y)
            self.vel = pygame.math.Vector2(speed, random.uniform(-0.4, 0.4))
        else:
            self.pos = pygame.math.Vector2(WIDTH + self.image.get_width(), y)
            self.vel = pygame.math.Vector2(-speed, random.uniform(-0.4, 0.4))
            self.image = pygame.transform.flip(self.image, True, False)
            self.mask = create_collision_mask(self.image, 115)
        self.rect = self.image.get_rect(center=self.pos)
        self.next_shot_at = pygame.time.get_ticks() + SPACE_SHIP_SHOT_MS

    @staticmethod
    def load_image():
        global SPACE_SHIP_BASE_IMAGE
        if SPACE_SHIP_BASE_IMAGE is None:
            image = pygame.image.load(SPACE_SHIP_IMAGE).convert_alpha()
            SPACE_SHIP_BASE_IMAGE = pygame.transform.smoothscale(image, (70, 54))
        return SPACE_SHIP_BASE_IMAGE.copy()

    def update(self):
        self.pos += self.vel
        self.rect.center = self.pos
        if self.rect.right < -40 or self.rect.left > WIDTH + 40 or self.rect.bottom < 0 or self.rect.top > HEIGHT:
            self.kill()

    def shoot(self, target_pos, speed_multiplier=1.0):
        direction = pygame.math.Vector2(target_pos) - self.pos
        if direction.length_squared() == 0:
            direction = pygame.math.Vector2(random.uniform(-1, 1), random.uniform(-1, 1))
        if direction.length_squared() == 0:
            direction = pygame.math.Vector2(1, 0)
        direction = direction.normalize().rotate(random.uniform(-24, 24))
        return EnemyBullet(self.pos + direction * 34, direction * 5.6 * speed_multiplier)


class Bullet(pygame.sprite.Sprite):
    def __init__(self, pos, vel):
        super().__init__()
        self.image = pygame.Surface((4, 4))
        self.image.fill((255, 255, 0))
        self.rect = self.image.get_rect(center=pos)
        self.mask = pygame.mask.from_surface(self.image)
        self.vel = vel
        self.life = 60

    def update(self):
        self.rect.center += self.vel
        self.life -= 1
        if self.life <= 0:
            self.kill()
        if self.rect.centerx < 0 or self.rect.centerx > WIDTH or self.rect.centery < 0 or self.rect.centery > HEIGHT:
            self.kill()


class EnemyBullet(pygame.sprite.Sprite):
    def __init__(self, pos, vel):
        super().__init__()
        self.image = pygame.Surface((5, 5), pygame.SRCALPHA)
        pygame.draw.circle(self.image, (255, 55, 45), (2, 2), 2)
        self.rect = self.image.get_rect(center=pos)
        self.mask = pygame.mask.from_surface(self.image)
        self.vel = vel
        self.life = 150

    def update(self):
        self.rect.center += self.vel
        self.life -= 1
        if self.life <= 0:
            self.kill()
        if self.rect.centerx < 0 or self.rect.centerx > WIDTH or self.rect.centery < 0 or self.rect.centery > HEIGHT:
            self.kill()


def draw_text(surface, text, x, y, font, color=(255, 255, 255)):
    surface.blit(font.render(text, True, color), (x, y))


def draw_player_shield(surface, ship):
    pygame.draw.circle(surface, (255, 210, 60), ship.rect.center, 34, 2)
    pygame.draw.circle(surface, (95, 190, 255), ship.rect.center, 39, 1)


def draw_button(surface, rect, text, font, mouse_pos):
    is_hovered = rect.collidepoint(mouse_pos)
    fill_color = (255, 210, 60) if is_hovered else (230, 180, 35)
    border_color = (255, 255, 255) if is_hovered else (180, 180, 180)
    pygame.draw.rect(surface, fill_color, rect, border_radius=6)
    pygame.draw.rect(surface, border_color, rect, 2, border_radius=6)
    label = font.render(text, True, (15, 15, 15))
    surface.blit(label, label.get_rect(center=rect.center))


def start_screen(screen, clock, font, controls_font):
    screen.fill((0, 0, 0))
    title = font.render("ASTEROIDS", True, (255, 255, 255))
    screen.blit(title, title.get_rect(center=(WIDTH // 2, 100)))

    instructions = [
        "Left / Right arrows: rotate",
        "Up arrow: thrust",
        "Space: shoot",
        "P: pause / restart menu",
        "ESC: quit anytime",
    ]
    for index, line in enumerate(instructions):
        draw_text(screen, line, 120, 220 + index * 34, controls_font)

    difficulty_text = [
        "Select difficulty:",
        "1 - Slow",
        "2 - Medium",
        "3 - Fast",
    ]
    for index, line in enumerate(difficulty_text):
        draw_text(screen, line, 120, 420 + index * 40, font)

    pygame.display.flip()
    while True:
        clock.tick(FPS)
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                pygame.quit()
                sys.exit()
            elif event.type == pygame.KEYDOWN:
                key_name = pygame.key.name(event.key)
                if key_name in DIFFICULTIES:
                    return DIFFICULTIES[key_name]
                elif event.key == pygame.K_ESCAPE:
                    pygame.quit()
                    sys.exit()


def split_asteroid(asteroid, asteroids, speed_multiplier=1.0):
    if asteroid.size > 20:
        for _ in range(2):
            new_size = asteroid.size // 2
            new_ast = Asteroid(asteroid.rect.center, new_size, speed_multiplier)
            asteroids.add(new_ast)


def spawn_wave(asteroids, wave, speed_multiplier):
    wave_speed = speed_multiplier * (1 + (wave - 1) * 0.08)
    asteroid_count = BASE_ASTEROIDS + wave
    for _ in range(asteroid_count):
        while True:
            pos = (random.randint(0, WIDTH), random.randint(0, HEIGHT))
            if math.hypot(pos[0] - WIDTH / 2, pos[1] - HEIGHT / 2) > 170:
                break
        asteroids.add(Asteroid(pos, 50, wave_speed))


def respawn_ship(ship):
    ship.pos = pygame.math.Vector2(WIDTH // 2, HEIGHT // 2)
    ship.vel = pygame.math.Vector2(0, 0)
    ship.angle = 0
    ship.orig_image = ship.no_thrust_image
    ship.image = ship.orig_image
    ship.rect = ship.image.get_rect(center=ship.pos)
    ship.mask = create_collision_mask(ship.image, 130)


def game_over_screen(screen, clock, font, score, wave):
    button_rect = pygame.Rect(0, 0, 220, 56)
    button_rect.center = (WIDTH // 2, HEIGHT // 2 + 70)

    while True:
        mouse_pos = pygame.mouse.get_pos()
        screen.fill((0, 0, 0))
        text = font.render("Game Over", True, (255, 255, 255))
        screen.blit(text, text.get_rect(center=(WIDTH // 2, HEIGHT // 2 - 80)))
        final_score = font.render(f"Final Score: {score}", True, (255, 255, 255))
        screen.blit(final_score, final_score.get_rect(center=(WIDTH // 2, HEIGHT // 2 - 35)))
        final_wave = font.render(f"Reached Wave: {wave}", True, (255, 255, 255))
        screen.blit(final_wave, final_wave.get_rect(center=(WIDTH // 2, HEIGHT // 2)))
        draw_button(screen, button_rect, "New Game", font, mouse_pos)
        draw_text(screen, "R / Enter: new game | ESC: quit", WIDTH // 2 - 165, HEIGHT // 2 + 125, font)
        pygame.display.flip()

        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                pygame.quit()
                sys.exit()
            elif event.type == pygame.KEYDOWN:
                if event.key == pygame.K_ESCAPE:
                    pygame.quit()
                    sys.exit()
                elif event.key in (pygame.K_RETURN, pygame.K_r):
                    return
            elif event.type == pygame.MOUSEBUTTONDOWN and event.button == 1:
                if button_rect.collidepoint(event.pos):
                    return

        clock.tick(FPS)


def pause_screen(screen, clock, font, score, lives, wave):
    resume_rect = pygame.Rect(0, 0, 220, 54)
    restart_rect = pygame.Rect(0, 0, 220, 54)
    resume_rect.center = (WIDTH // 2, HEIGHT // 2 + 20)
    restart_rect.center = (WIDTH // 2, HEIGHT // 2 + 90)
    play_sound("pause")

    while True:
        mouse_pos = pygame.mouse.get_pos()
        screen.fill((0, 0, 0))
        title = font.render("Paused", True, (255, 255, 255))
        screen.blit(title, title.get_rect(center=(WIDTH // 2, HEIGHT // 2 - 110)))
        stats = font.render(f"Score: {score}   Lives: {lives}   Wave: {wave}", True, (255, 255, 255))
        screen.blit(stats, stats.get_rect(center=(WIDTH // 2, HEIGHT // 2 - 65)))
        draw_button(screen, resume_rect, "Resume", font, mouse_pos)
        draw_button(screen, restart_rect, "Restart", font, mouse_pos)
        draw_text(screen, "P / Enter: resume | R: restart | ESC: quit", WIDTH // 2 - 220, HEIGHT // 2 + 145, font)
        pygame.display.flip()

        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                pygame.quit()
                sys.exit()
            elif event.type == pygame.KEYDOWN:
                if event.key == pygame.K_ESCAPE:
                    pygame.quit()
                    sys.exit()
                if event.key in (pygame.K_p, pygame.K_RETURN):
                    return "resume"
                if event.key == pygame.K_r:
                    return "restart"
            elif event.type == pygame.MOUSEBUTTONDOWN and event.button == 1:
                if resume_rect.collidepoint(event.pos):
                    return "resume"
                if restart_rect.collidepoint(event.pos):
                    return "restart"

        clock.tick(FPS)


def main():
    pygame.mixer.pre_init(44100, -16, 1, 512)
    pygame.init()
    screen = pygame.display.set_mode((WIDTH, HEIGHT))
    pygame.display.set_caption("Asteroids")
    set_window_icon()
    clock = pygame.time.Clock()
    setup_sounds()

    font = load_atari_font(24)
    hud_font = load_atari_font(22)
    controls_font = load_atari_font(16)
    difficulty_name, speed_multiplier = start_screen(screen, clock, font, controls_font)

    while True:
        ship = Ship(speed_multiplier=speed_multiplier)
        ship_group = pygame.sprite.Group(ship)
        bullets = pygame.sprite.Group()
        enemy_bullets = pygame.sprite.Group()
        asteroids = pygame.sprite.Group()
        space_ships = pygame.sprite.Group()

        score = 0
        lives = 5
        wave = 1
        shield_uses = SHIELD_USES_PER_WAVE
        shield_active_until = 0
        invulnerable_until = pygame.time.get_ticks() + INVULNERABLE_MS
        next_space_ship_spawn = pygame.time.get_ticks() + SPACE_SHIP_INTERVAL_MS
        spawn_wave(asteroids, wave, speed_multiplier)
        play_sound("wave")
        running = True
        restart_requested = False

        while running:
            now = pygame.time.get_ticks()
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    pygame.quit()
                    sys.exit()
                elif event.type == pygame.KEYDOWN:
                    if event.key == pygame.K_ESCAPE:
                        pygame.quit()
                        sys.exit()
                    elif event.key == pygame.K_SPACE:
                        bullets.add(ship.shoot())
                        play_sound("shoot")
                    elif event.key in (pygame.K_LSHIFT, pygame.K_RSHIFT) and shield_uses > 0 and now > shield_active_until:
                        shield_uses -= 1
                        shield_active_until = now + SHIELD_DURATION_MS
                    elif event.key == pygame.K_p:
                        set_thrust_sound(False)
                        paused_at = pygame.time.get_ticks()
                        if pause_screen(screen, clock, font, score, lives, wave) == "restart":
                            restart_requested = True
                            running = False
                        else:
                            pause_duration = pygame.time.get_ticks() - paused_at
                            invulnerable_until += pause_duration
                            next_space_ship_spawn += pause_duration
                            shield_active_until += pause_duration

            set_thrust_sound(running and pygame.key.get_pressed()[pygame.K_UP])
            ship_group.update()
            bullets.update()
            enemy_bullets.update()
            asteroids.update()
            space_ships.update()

            if now >= next_space_ship_spawn and not space_ships:
                space_ships.add(SpaceShip(speed_multiplier))
                next_space_ship_spawn = now + SPACE_SHIP_INTERVAL_MS
                play_sound("space_ship")

            for space_ship in space_ships:
                if now >= space_ship.next_shot_at:
                    enemy_bullets.add(space_ship.shoot(ship.pos, speed_multiplier))
                    space_ship.next_shot_at = now + SPACE_SHIP_SHOT_MS

            hits = pygame.sprite.groupcollide(bullets, asteroids, True, False, pygame.sprite.collide_mask)
            for bullet, hit_list in hits.items():
                for asteroid in hit_list:
                    score += 100
                    split_asteroid(asteroid, asteroids, speed_multiplier)
                    asteroid.kill()
                    play_sound("hit")

            space_ship_hits = pygame.sprite.groupcollide(bullets, space_ships, True, True, pygame.sprite.collide_mask)
            if space_ship_hits:
                score += 1000 * sum(len(hit_list) for hit_list in space_ship_hits.values())
                play_sound("hit")

            if not asteroids:
                wave += 1
                score += 500
                shield_uses = SHIELD_USES_PER_WAVE
                shield_active_until = 0
                bullets.empty()
                enemy_bullets.empty()
                spawn_wave(asteroids, wave, speed_multiplier)
                play_sound("wave")

            shield_active = now <= shield_active_until
            if shield_active:
                pygame.sprite.spritecollide(ship, enemy_bullets, True, pygame.sprite.collide_mask)

            if not shield_active and now > invulnerable_until and pygame.sprite.spritecollideany(ship, asteroids, pygame.sprite.collide_mask):
                lives -= 1
                play_sound("lose_life")
                if lives <= 0:
                    set_thrust_sound(False)
                    running = False
                else:
                    respawn_ship(ship)
                    bullets.empty()
                    enemy_bullets.empty()
                    invulnerable_until = pygame.time.get_ticks() + INVULNERABLE_MS

            if not shield_active and now > invulnerable_until and pygame.sprite.spritecollideany(ship, space_ships, pygame.sprite.collide_mask):
                lives -= 1
                play_sound("lose_life")
                space_ships.empty()
                if lives <= 0:
                    set_thrust_sound(False)
                    running = False
                else:
                    respawn_ship(ship)
                    bullets.empty()
                    enemy_bullets.empty()
                    invulnerable_until = pygame.time.get_ticks() + INVULNERABLE_MS

            if not shield_active and now > invulnerable_until and pygame.sprite.spritecollideany(ship, enemy_bullets, pygame.sprite.collide_mask):
                lives -= 1
                play_sound("lose_life")
                enemy_bullets.empty()
                if lives <= 0:
                    set_thrust_sound(False)
                    running = False
                else:
                    respawn_ship(ship)
                    bullets.empty()
                    invulnerable_until = pygame.time.get_ticks() + INVULNERABLE_MS

            screen.fill((0, 0, 0))
            draw_advert_logo(screen)
            asteroids.draw(screen)
            space_ships.draw(screen)
            bullets.draw(screen)
            enemy_bullets.draw(screen)
            if now > invulnerable_until or (now // BLINK_MS) % 2 == 0:
                ship_group.draw(screen)
            if shield_active:
                draw_player_shield(screen, ship)

            draw_text(screen, f"Score: {score}", 10, 10, hud_font)
            draw_text(screen, f"Lives: {lives}", 10, 38, hud_font)
            draw_text(screen, f"Wave: {wave}", 10, 66, hud_font)
            draw_text(screen, f"Shield: {shield_uses}", 10, 94, hud_font, (255, 210, 60))
            if now <= invulnerable_until:
                draw_text(screen, "Protected", 10, 122, hud_font, (255, 210, 60))
            draw_text(
                screen,
                "Controls: Left/Right rotate | Up thrust | Space shoot | Shift shield | P pause | ESC quit",
                10,
                HEIGHT - 32,
                controls_font,
            )

            pygame.display.flip()
            clock.tick(FPS)

        set_thrust_sound(False)
        if not restart_requested:
            game_over_screen(screen, clock, font, score, wave)


if __name__ == "__main__":
    main()
