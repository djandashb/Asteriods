import sys
import pygame

WIDTH = 800
HEIGHT = 600
FPS = 60
GRAVITY = 0.04
THRUST = -0.11


class Lander(pygame.sprite.Sprite):
    def __init__(self):
        super().__init__()
        self.image = pygame.Surface((24, 32), pygame.SRCALPHA)
        pygame.draw.polygon(self.image, (255, 255, 255), [(12, 0), (0, 32), (24, 32)])
        self.rect = self.image.get_rect(center=(WIDTH // 2, 80))
        self.pos = pygame.math.Vector2(self.rect.center)
        self.vel = pygame.math.Vector2(0, 0)
        self.fuel = 1200

    def update(self):
        keys = pygame.key.get_pressed()
        if keys[pygame.K_UP] and self.fuel > 0:
            self.vel.y += THRUST
            self.fuel -= 2
        self.vel.y += GRAVITY
        self.pos += self.vel
        self.rect.center = self.pos
        if self.rect.left < 0:
            self.rect.left = 0
            self.pos.x = self.rect.centerx
            self.vel.x = 0
        if self.rect.right > WIDTH:
            self.rect.right = WIDTH
            self.pos.x = self.rect.centerx
            self.vel.x = 0
        if self.rect.top < 0:
            self.rect.top = 0
            self.pos.y = self.rect.centery
            self.vel.y = 0


class LandingPad(pygame.sprite.Sprite):
    def __init__(self, x, width):
        super().__init__()
        self.image = pygame.Surface((width, 10))
        self.image.fill((0, 255, 0))
        self.rect = self.image.get_rect(midtop=(x, HEIGHT - 50))


def main():
    pygame.init()
    screen = pygame.display.set_mode((WIDTH, HEIGHT))
    pygame.display.set_caption("Lunar Lander")
    clock = pygame.time.Clock()

    lander = Lander()
    lander_group = pygame.sprite.Group(lander)
    pad = LandingPad(WIDTH // 2, 120)
    pad_group = pygame.sprite.Group(pad)

    font = pygame.font.SysFont(None, 28)
    running = True
    landed = False
    crashed = False

    while running:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False

        if not landed and not crashed:
            lander.update()
            if lander.rect.colliderect(pad.rect):
                if abs(lander.vel.y) < 1.5 and abs(lander.vel.x) < 1.5:
                    landed = True
                else:
                    crashed = True
            elif lander.rect.bottom >= HEIGHT:
                crashed = True

        screen.fill((0, 0, 30))
        pygame.draw.rect(screen, (60, 60, 60), (0, HEIGHT - 40, WIDTH, 40))
        pad_group.draw(screen)
        lander_group.draw(screen)

        status = "Flying"
        if landed:
            status = "Landed Successfully!"
        elif crashed:
            status = "Crashed"

        screen.blit(font.render(f"Fuel: {lander.fuel}", True, (255, 255, 255)), (10, 10))
        screen.blit(font.render(f"Vertical Speed: {lander.vel.y:.2f}", True, (255, 255, 255)), (10, 40))
        screen.blit(font.render(status, True, (255, 255, 255)), (10, 70))

        if landed or crashed:
            message = "ESC to quit" if crashed else "Landed! ESC to quit"
            screen.blit(font.render(message, True, (255, 255, 255)), (WIDTH // 2 - 90, HEIGHT // 2))

        pygame.display.flip()
        clock.tick(FPS)

    pygame.quit()
    sys.exit()


if __name__ == "__main__":
    main()
