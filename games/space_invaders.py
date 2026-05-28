import random
import sys
import pygame

WIDTH = 640
HEIGHT = 720
FPS = 60
ALIEN_ROWS = 5
ALIEN_COLS = 11
ALIEN_PADDING = 60
BUNKER_ROWS = 3
BUNKER_COLS = 6


class Player(pygame.sprite.Sprite):
    def __init__(self):
        super().__init__()
        self.image = pygame.Surface((50, 30))
        self.image.fill((0, 255, 0))
        self.rect = self.image.get_rect(midbottom=(WIDTH // 2, HEIGHT - 30))
        self.speed = 6
        self.shoot_delay = 300
        self.last_shot = 0

    def update(self):
        keys = pygame.key.get_pressed()
        if keys[pygame.K_LEFT]:
            self.rect.x -= self.speed
        if keys[pygame.K_RIGHT]:
            self.rect.x += self.speed
        self.rect.x = max(0, min(self.rect.x, WIDTH - self.rect.width))

    def shoot(self, bullets, now):
        if now - self.last_shot >= self.shoot_delay:
            bullet = Bullet(self.rect.midtop)
            bullets.add(bullet)
            self.last_shot = now


class Alien(pygame.sprite.Sprite):
    def __init__(self, x, y):
        super().__init__()
        self.image = pygame.Surface((34, 24))
        self.image.fill((255, 255, 255))
        self.rect = self.image.get_rect(topleft=(x, y))


class Bullet(pygame.sprite.Sprite):
    def __init__(self, pos):
        super().__init__()
        self.image = pygame.Surface((4, 12))
        self.image.fill((255, 255, 0))
        self.rect = self.image.get_rect(midbottom=pos)
        self.speed = -10

    def update(self):
        self.rect.y += self.speed
        if self.rect.bottom < 0:
            self.kill()


class AlienBullet(pygame.sprite.Sprite):
    def __init__(self, pos):
        super().__init__()
        self.image = pygame.Surface((4, 12))
        self.image.fill((255, 0, 0))
        self.rect = self.image.get_rect(midtop=pos)
        self.speed = 6

    def update(self):
        self.rect.y += self.speed
        if self.rect.top > HEIGHT:
            self.kill()


class BunkerBlock(pygame.sprite.Sprite):
    def __init__(self, x, y):
        super().__init__()
        self.image = pygame.Surface((8, 8))
        self.image.fill((0, 255, 255))
        self.rect = self.image.get_rect(topleft=(x, y))


def create_aliens():
    aliens = pygame.sprite.Group()
    for row in range(ALIEN_ROWS):
        for col in range(ALIEN_COLS):
            x = 60 + col * ALIEN_PADDING
            y = 80 + row * 50
            aliens.add(Alien(x, y))
    return aliens


def create_bunkers():
    blocks = pygame.sprite.Group()
    start_x = 100
    for bunker in range(4):
        base_x = start_x + bunker * 130
        base_y = HEIGHT - 160
        for row in range(BUNKER_ROWS):
            for col in range(BUNKER_COLS):
                if not (row == 0 and col in [0, 5]):
                    blocks.add(BunkerBlock(base_x + col * 10, base_y + row * 10))
    return blocks


def main():
    pygame.init()
    screen = pygame.display.set_mode((WIDTH, HEIGHT))
    pygame.display.set_caption("Space Invaders")
    clock = pygame.time.Clock()

    player = Player()
    player_group = pygame.sprite.Group(player)
    aliens = create_aliens()
    bullets = pygame.sprite.Group()
    alien_bullets = pygame.sprite.Group()
    bunkers = create_bunkers()

    alien_dx = 1
    move_timer = 0
    move_interval = 600
    score = 0
    lives = 3

    font = pygame.font.SysFont(None, 28)

    running = True
    while running:
        now = pygame.time.get_ticks()
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
            elif event.type == pygame.KEYDOWN:
                if event.key == pygame.K_SPACE:
                    player.shoot(bullets, now)

        player_group.update()
        bullets.update()
        alien_bullets.update()

        if now - move_timer > move_interval:
            move_timer = now
            edge_hit = False
            for alien in aliens:
                alien.rect.x += alien_dx * 10
                if alien.rect.right >= WIDTH - 20 or alien.rect.left <= 20:
                    edge_hit = True
            if edge_hit:
                alien_dx *= -1
                for alien in aliens:
                    alien.rect.y += 20
                    if alien.rect.bottom >= player.rect.top:
                        lives = 0

        for alien in aliens:
            if random.random() < 0.003:
                alien_bullets.add(AlienBullet(alien.rect.midbottom))

        hits = pygame.sprite.groupcollide(bullets, aliens, True, True)
        score += len(hits) * 10

        for bullet in bullets:
            block_hits = pygame.sprite.spritecollide(bullet, bunkers, True)
            if block_hits:
                bullet.kill()

        for shot in alien_bullets:
            bunker_hits = pygame.sprite.spritecollide(shot, bunkers, True)
            if bunker_hits:
                shot.kill()

        if pygame.sprite.spritecollideany(player, alien_bullets):
            lives -= 1
            alien_bullets.empty()
            if lives <= 0:
                running = False

        if pygame.sprite.spritecollideany(player, aliens):
            lives = 0
            running = False

        screen.fill((0, 0, 0))
        player_group.draw(screen)
        aliens.draw(screen)
        bullets.draw(screen)
        alien_bullets.draw(screen)
        bunkers.draw(screen)

        screen.blit(font.render(f"Score: {score}", True, (255, 255, 255)), (10, 10))
        screen.blit(font.render(f"Lives: {lives}", True, (255, 255, 255)), (WIDTH - 120, 10))

        pygame.display.flip()
        clock.tick(FPS)

    screen.fill((0, 0, 0))
    text = font.render("Game Over - Press ESC to exit", True, (255, 255, 255))
    screen.blit(text, text.get_rect(center=(WIDTH // 2, HEIGHT // 2)))
    pygame.display.flip()
    while True:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                pygame.quit()
                sys.exit()
            elif event.type == pygame.KEYDOWN and event.key == pygame.K_ESCAPE:
                pygame.quit()
                sys.exit()


if __name__ == "__main__":
    main()
