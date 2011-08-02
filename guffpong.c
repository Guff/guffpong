#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <SDL.h>
#include <SDL_Pango.h>
#include <SDL_gfxPrimitives.h>

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#define INT_MS        50
#define WINDOW_WIDTH  400
#define WINDOW_HEIGHT 400
#define PADDLE_WIDTH  20
#define PADDLE_HEIGHT 150
#define P1_X          370
#define P2_X          (WINDOW_HEIGHT - P1_X - PADDLE_WIDTH)

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

struct {
    unsigned int p1;
    unsigned int p2;
} scores = {0, 0};

ball_t ball;
paddle_t p1 = {(WINDOW_HEIGHT - PADDLE_HEIGHT) / 2, 0};
paddle_t p2 = {(WINDOW_HEIGHT - PADDLE_HEIGHT) / 2, 0};

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

void draw_stippled_line(SDL_Surface *screen) {
    for (int i = 0; i < WINDOW_HEIGHT / 40; i++) {
        boxColor(screen, WINDOW_WIDTH / 2 - 3, 2 * i * 20, WINDOW_WIDTH / 2 + 3,
                 2 * i * 20 + 20, 0xffffffff);
    }
}

void draw_frame(SDL_Surface *screen) {
    SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0, 0, 0));
    draw_stippled_line(screen);
    boxColor(screen, P1_X, p1.y, P1_X + PADDLE_WIDTH,
             p1.y + PADDLE_HEIGHT, 0xffffffff);
    boxColor(screen, P2_X, p2.y, P2_X + PADDLE_WIDTH,
             p2.y + PADDLE_HEIGHT, 0xffffffff);
    filledCircleColor(screen, ball.x, ball.y, ball.radius, 0xffffffff);
    aacircleColor(screen, ball.x, ball.y, ball.radius, 0xffffffff);
    SDLPango_Context *context = SDLPango_CreateContext_GivenFontDesc("Sans 20");
    SDLPango_SetDefaultColor(context, MATRIX_TRANSPARENT_BACK_WHITE_LETTER);
    SDLPango_SetMinimumSize(context, WINDOW_WIDTH, 0);
    char text[4];
    snprintf(text, 4, "%i %i", scores.p2, scores.p1);
    SDLPango_SetText_GivenAlignment(context, text, strlen(text),
                                    SDLPANGO_ALIGN_CENTER);
    //SDLPango_Draw(context, screen, 0, 0);
    SDL_Surface *surface = SDLPango_CreateSurfaceDraw(context);
    SDL_BlitSurface(surface, NULL, screen, NULL);
    SDLPango_FreeContext(context);
    SDL_FreeSurface(surface);
    SDL_Flip(screen);
}

bool point_in_rect(int x, int y, rectangle_t rect) {
    return (x >= rect.x && x <= rect.x + rect.width &&
            y >= rect.y && y <= rect.y + rect.height);
}

bool ball_will_collide(double x, double y, double width, double height) {
    double pos_int_x = ball.x + ball.vel_x;
    double pos_int_y = ball.y + ball.vel_y;
    
    rectangle_t rect = {x, y, width, height};
    
    return point_in_rect(pos_int_x, pos_int_y, rect);
}

void reset_ball(void) {
    double mul = rand() / (double) RAND_MAX;
    ball.vel_x = mul * 10;
    ball.vel_y = 10 - ball.vel_x;
    ball.x = WINDOW_WIDTH / 2;
    ball.y = WINDOW_HEIGHT / 2;
    ball.radius = 10;
}

void ball_compute_position(void) {
    if (ball_will_collide(0, -10, WINDOW_WIDTH, 10)) // top wall
        ball.vel_y = -ball.vel_y;
    else if (ball_will_collide(WINDOW_WIDTH, 0, 10, WINDOW_HEIGHT)) // right
        scores.p2++, reset_ball();
    else if (ball_will_collide(0, WINDOW_HEIGHT, WINDOW_WIDTH, 10)) // bottom
        ball.vel_y = -ball.vel_y;
    else if (ball_will_collide(-10, 0, 10, WINDOW_HEIGHT)) // left
        scores.p1++, reset_ball();
    else if (ball_will_collide(P1_X, p1.y, PADDLE_WIDTH, PADDLE_HEIGHT)) { // P1
        ball.vel_x = -ball.vel_x;
        ball.vel_y += p1.vel_y / 2;
    }
    else if (ball_will_collide(P2_X, p2.y, PADDLE_WIDTH, PADDLE_HEIGHT)) { // P2
        ball.vel_x = -ball.vel_x;
        ball.vel_y += p1.vel_y / 2;
    }
    
    ball.x += ball.vel_x;
    ball.y += ball.vel_y;
}

void paddle_move(paddle_t *paddle) {
    // factor in friction for the paddle
    paddle->vel_y = copysign(MAX(abs(paddle->vel_y) - 0.5, 0), paddle->vel_y);
    paddle->y += paddle->vel_y;
    if (paddle->y > WINDOW_HEIGHT - PADDLE_HEIGHT) {
        paddle->y = WINDOW_HEIGHT - PADDLE_HEIGHT;
        paddle->vel_y = -paddle->vel_y;
    }
    else if (paddle->y < 0) {
        paddle->y = 0;
        paddle->vel_y = -paddle->vel_y;
    }
}

void ai_paddle(void) {
    if (p2.y > p1.y)
        p2.vel_y -= 2;
    else
        p2.vel_y += 2;
}

void game_loop(SDL_Surface *screen) {
    running = true;
    
    srand(time(NULL));
    
    reset_ball();
    
    while (running) {
        int loop_time = SDL_GetTicks();
        
        if (key_state.up_down)
            p1.vel_y -= 2;
        if (key_state.down_down)
            p1.vel_y += 2;
        
        ai_paddle();
        
        paddle_move(&p1);
        paddle_move(&p2);
        
        ball_compute_position();
        
        get_input();
        draw_frame(screen);
        SDL_Delay(MIN(0, INT_MS - (SDL_GetTicks() - loop_time)));
    }
}

int main(int argc, char **argv) {    
    SDL_Init(SDL_INIT_VIDEO);
    
    SDLPango_Init();
    
    SDL_WM_SetCaption("guffpong", "guffpong");
    
    SDL_Surface *screen = SDL_SetVideoMode(WINDOW_WIDTH, WINDOW_HEIGHT, 16,
                                           SDL_DOUBLEBUF | SDL_HWSURFACE);
    SDL_ShowCursor(false);
    
    running = true;
    
    game_loop(screen);
    
    return 0;
}
