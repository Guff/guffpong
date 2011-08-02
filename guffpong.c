#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <SDL.h>
#include <SDL_gfxPrimitives.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define INT_MS        50
#define WINDOW_WIDTH  400
#define WINDOW_HEIGHT 400
#define PADDLE_WIDTH  20
#define PADDLE_HEIGHT 150
#define P1_X          370

struct {
    bool up_down;
    bool down_down;
} key_state = {false, false};

typedef struct {
    double x, y;
    double width, height;
} rectangle_t;

typedef struct {
    double vel_x, vel_y;
    double x, y;
    double radius;
} ball_t;

typedef struct {
    double y;
    double vel_y;
} paddle_t;

ball_t ball;
paddle_t p1;

bool running;

void get_input(void) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                running = false;
                break;
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_ESCAPE:
                        running = false;
                        break;
                    case SDLK_UP:
                        key_state.up_down = true;
                        break;
                    case SDLK_DOWN:
                        key_state.down_down = true;
                        break;
                    default:
                        break;
                }
                break;
            case SDL_KEYUP:
                switch (event.key.keysym.sym) {
                    case SDLK_UP:
                        key_state.up_down = false;
                        break;
                    case SDLK_DOWN:
                        key_state.down_down = false;
                        break;
                    default:
                        break;
                }
                break;
            default:
                break;
        }
    }
}

void draw_frame(SDL_Surface *screen) {
    SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0, 0, 0));
    boxColor(screen, P1_X, p1.y, P1_X + PADDLE_WIDTH,
                   p1.y + PADDLE_HEIGHT, 0xffffffff);
    filledCircleColor(screen, ball.x, ball.y, ball.radius, 0xffffffff);
    aacircleColor(screen, ball.x, ball.y, ball.radius, 0xffffffff);
    SDL_Flip(screen);
}

bool point_in_rect(int x, int y, rectangle_t rect) {
    return (x >= rect.x && x <= rect.x + rect.width &&
            y >= rect.y && y <= rect.y + rect.height);
}

bool ball_will_collide(double x, double y, double width, double height) {
    double pos_int_x = ball.x + ball.vel_x + ball.radius;
    double pos_int_y = ball.y + ball.vel_y + ball.radius;
    
    rectangle_t rect = {x, y, width, height};
    
    return point_in_rect(pos_int_x, pos_int_y, rect);
}

void ball_compute_position(void) {
    if (ball_will_collide(0, 0, WINDOW_WIDTH, 10)) // top wall
        ball.vel_y = -ball.vel_y;
    else if (ball_will_collide(WINDOW_WIDTH, 0, 10, WINDOW_HEIGHT)) // right
        ball.vel_x = -ball.vel_x;
    else if (ball_will_collide(0, WINDOW_HEIGHT, WINDOW_WIDTH, 10)) // bottom
        ball.vel_y = -ball.vel_y;
    else if (ball_will_collide(0, 0, 10, WINDOW_HEIGHT)) // left
        ball.vel_x = -ball.vel_x;
    else if (ball_will_collide(P1_X, p1.y, PADDLE_WIDTH, PADDLE_HEIGHT))
        ball.vel_x = -ball.vel_x;
    
    ball.x += ball.vel_x;
    ball.y += ball.vel_y;
}

void game_loop(SDL_Surface *screen) {
    running = true;
    
    p1.y = 0;
    p1.vel_y = 0;
    
    ball.x = WINDOW_WIDTH / 2;
    ball.y = WINDOW_HEIGHT / 2;
    ball.vel_x = -4;
    ball.vel_y = -3;
    ball.radius = 10;
    
    while (running) {
        int loop_time = SDL_GetTicks();
        
        if (key_state.up_down)
            p1.vel_y -= 2;
        if (key_state.down_down)
            p1.vel_y += 2;
        
        p1.vel_y = copysign(MAX(abs(p1.vel_y) - 0.5, 0), p1.vel_y);
        p1.y += p1.vel_y;
        if (p1.y > WINDOW_HEIGHT - PADDLE_HEIGHT) {
            p1.y = WINDOW_HEIGHT - PADDLE_HEIGHT;
            p1.vel_y = -p1.vel_y;
        }
        else if (p1.y < 0) {
            p1.y = 0;
            p1.vel_y = -p1.vel_y;
        }
        
        ball_compute_position();
        
        get_input();
        draw_frame(screen);
        SDL_Delay(INT_MS - (SDL_GetTicks() - loop_time));
        printf("%f\n", p1.vel_y);
    }
}

int main(int argc, char **argv) {    
    SDL_Init(SDL_INIT_VIDEO);
    
    SDL_WM_SetCaption("guffpong", "guffpong");
    
    SDL_Surface *screen = SDL_SetVideoMode(WINDOW_WIDTH, WINDOW_HEIGHT, 16,
                                           SDL_DOUBLEBUF);
    SDL_ShowCursor(false);
    
    running = true;
    
    game_loop(screen);
    
    return 0;
}
