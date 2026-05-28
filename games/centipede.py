import random
import sys
import pygame

WIDTH = 640
HEIGHT = 720
FPS = 60
MUSHROOM_COUNT = 30


class Player(pygame.sprite.Sprite):
    def __init__(self):
        super().__init__()
        self.image = pygame.Surface((40, 20))
        self.image.fill((0, 255, 0))
        self.rect = self.image.get_rect(midbottom=(WIDTH // 2, HEIGHT - 30))
        self.speed = 6
        self.reload = 0

    def update(self):
        keys = pygame.key.get_pressed()
        if keys[pygame.K_LEFT]:
            self.rect.x -= self.speed
        if keys[pygame.K_RIGHT]:
            self.rect.x += self.speed
        self.rect.x = max(0, min(self.rect.x, WIDTH - self.rect.width))
        if self.reload > 0:
            self.reload -= 1

    def shoot(self, bullets):
        if self.reload == 0:
            bullets.add(Bullet(self.rect.midtop))
            self.reload = 20


class Bullet(pygame.sprite.Sprite):
    def __init__(self, pos):
        super().__init__()
        self.image = pygame.Surface((4, 10))
        self.image.fill((255, 255, 0))
        self.rect = self.image.get_rect(midbottom=pos)
        self.speed = -10

    def update(self):
        self.rect.y += self.speed
        if self.rect.bottom < 0:
            self.kill()


class CentipedeSegment(pygame.sprite.Sprite):
    def __init__(self, x, y, index):
        super().__init__()
        self.image = pygame.Surface((22, 18))
        self.image.fill((255, 0, 255) if index == 0 else (255, 200, 200))
        self.rect = self.image.get_rect(topleft=(x, y))
        self.direction = 1

    def update(self):
        self.rect.x += self.direction * 4
        if self.rect.right >= WIDTH - 10 or self.rect.left <= 10:
            self.direction *= -1
            self.rect.y += 20


class Mushroom(pygame.sprite.Sprite):
    def __init__(self, x, y):
        super().__init__()
        self.image = pygame.Surface((16, 16))
        self.image.fill((0, 150, 0))
        self.rect = self.image.get_rect(center=(x, y))


def create_mushrooms():
    mushrooms = pygame.sprite.Group()
    for _ in range(MUSHROOM_COUNT):
        x = random.randint(40, WIDTH - 40)
        y = random.randint(120, HEIGHT - 200)
        mushrooms.add(Mushroom(x, y))
    return mushrooms


def main():
    pygame.init()
    screen = pygame.display.set_mode((WIDTH, HEIGHT))
    pygame.display.set_caption("Centipede")
    clock = pygame.time.Clock()

    player = Player()
    player_group = pygame.sprite.Group(player)
    bullets = pygame.sprite.Group()
    mushrooms = create_mushrooms()
    centipede = pygame.sprite.Group()
    for i in range(12):
        centipede.add(CentipedeSegment(10 + i * 24, 70, i))

    font = pygame.font.SysFont(None, 28)
    score = 0
    running = True

    while running:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
            elif event.type == pygame.KEYDOWN and event.key == pygame.K_SPACE:
                player.shoot(bullets)

        player_group.update()
        bullets.update()
        centipede.update()

        for bullet in bullets:
            hits = pygame.sprite.spritecollide(bullet, mushrooms, True)
            if hits:
                bullet.kill()
                score += 10

        collisions = pygame.sprite.groupcollide(bullets, centipede, True, False)
        for bullet, hit_segments in collisions.items():
            for segment in hit_segments:
                score += 50
                mushrooms.add(Mushroom(segment.rect.centerx, segment.rect.centery))
                segment.kill()

        if pygame.sprite.spritecollideany(player, centipede):
            running = False

        screen.fill((0, 0, 0))
        mushrooms.draw(screen)
        centipede.draw(screen)
        bullets.draw(screen)
        player_group.draw(screen)

        screen.blit(font.render(f"Score: {score}", True, (255, 255, 255)), (10, 10))
        pygame.display.flip()
        clock.tick(FPS)

    screen.fill((0, 0, 0))
    text = font.render("Game Over - Press ESC to quit", True, (255, 255, 255))
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
