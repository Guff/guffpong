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
#define BALL_RADIUS   10
#define PADDLE_WIDTH  20
#define PADDLE_HEIGHT 150
#define AI_FUDGE      0.5
#define P1_X          370
#define P2_X          (WINDOW_HEIGHT - P1_X - PADDLE_WIDTH)

struct {
    bool up_down;
    bool down_down;
    bool left_down;
    bool right_down;
} key_state = {false, false, false, false};

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
    double x, y;
    double vel_x, vel_y;
} paddle_t;

struct {
    unsigned int p1;
    unsigned int p2;
} scores = {0, 0};

ball_t ball;
paddle_t p1 = {P1_X, (WINDOW_HEIGHT - PADDLE_HEIGHT) / 2, 0, 0};
paddle_t p2 = {P2_X, (WINDOW_HEIGHT - PADDLE_HEIGHT) / 2, 0, 0};

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
                    case SDLK_LEFT:
                        key_state.left_down = true;
                        break;
                    case SDLK_RIGHT:
                        key_state.right_down = true;
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
                    case SDLK_LEFT:
                        key_state.left_down = false;
                        break;
                    case SDLK_RIGHT:
                        key_state.right_down = false;
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
    
    // player 1's paddle
    boxColor(screen, p1.x, p1.y, p1.x + PADDLE_WIDTH,
             p1.y + PADDLE_HEIGHT, 0xffffffff);
    // player 2's paddle
    boxColor(screen, p2.x, p2.y, p2.x + PADDLE_WIDTH,
             p2.y + PADDLE_HEIGHT, 0xffffffff);
    
    // draw the circle
    filledCircleColor(screen, ball.x, ball.y, ball.radius, 0xffffffff);
    // draw an antialiased border for the circle
    aacircleColor(screen, ball.x, ball.y, ball.radius, 0xffffffff);
    
    // score text
    SDLPango_Context *context = SDLPango_CreateContext_GivenFontDesc("Sans 20");
    SDLPango_SetDefaultColor(context, MATRIX_TRANSPARENT_BACK_WHITE_LETTER);
    SDLPango_SetMinimumSize(context, WINDOW_WIDTH, 0);
    char text[8];
    snprintf(text, 8, "%3i %3i", scores.p2, scores.p1);
    SDLPango_SetText_GivenAlignment(context, text, strlen(text),
                                    SDLPANGO_ALIGN_CENTER);
    SDL_Surface *surface = SDLPango_CreateSurfaceDraw(context);
    SDL_BlitSurface(surface, NULL, screen, NULL);
    SDLPango_FreeContext(context);
    SDL_FreeSurface(surface);
    SDL_Flip(screen);
}

void reset_ball(void) {
    double mul = rand() / (double) RAND_MAX;
    ball.vel_x = MAX(mul * 10, 10 - mul * 10);
    ball.vel_y = 10 - ball.vel_x;
    ball.x = WINDOW_WIDTH / 2;
    ball.y = WINDOW_HEIGHT / 2;
    ball.radius = BALL_RADIUS;
}

void ball_compute_position(void) {
    double pos_int_x = ball.x + ball.vel_x;
    double pos_int_y = ball.y + ball.vel_y;
    
    // top and bottom walls
    if (pos_int_y < BALL_RADIUS || pos_int_y > WINDOW_HEIGHT - BALL_RADIUS)
        ball.vel_y = -ball.vel_y;
    // p1's paddle
    if (pos_int_y > p1.y && pos_int_y < p1.y + PADDLE_HEIGHT &&
        (pos_int_x + BALL_RADIUS) > p1.x &&
        (pos_int_x + BALL_RADIUS) < p1.x + PADDLE_WIDTH) {
            ball.vel_x = -ball.vel_x;
            ball.vel_x += p1.vel_x / 2;
            ball.vel_y += p1.vel_y / 2;
    }
    // p2's paddle
    if (pos_int_y > p2.y && pos_int_y < p2.y + PADDLE_HEIGHT &&
        (pos_int_x - BALL_RADIUS) < p2.x + PADDLE_WIDTH &&
        (pos_int_x - BALL_RADIUS) > p2.x) {
            ball.vel_x = -ball.vel_x;
            ball.vel_x += p2.vel_x / 2;
            ball.vel_y += p2.vel_y / 2;
    }
    
    if (pos_int_x < BALL_RADIUS) { // left wall
        scores.p1++, reset_ball();
        printf("%f %f\n", pos_int_x - BALL_RADIUS, p2.x + PADDLE_WIDTH);
    }
    else if (pos_int_x > WINDOW_WIDTH - BALL_RADIUS) // right wall
        scores.p2++, reset_ball();
    
    ball.x += ball.vel_x;
    ball.y += ball.vel_y;
}

void paddle_move(paddle_t *paddle) {
    // use the current position of the paddle to determine which box it's in
    double paddle_x = paddle->x;
    
    // factor in friction for the paddle
    paddle->vel_y = copysign(MAX(abs(paddle->vel_y) - 0.5, 0), paddle->vel_y);
    paddle->y += paddle->vel_y;
    
    if (paddle->y > WINDOW_HEIGHT - PADDLE_HEIGHT) {
        paddle->y = WINDOW_HEIGHT - PADDLE_HEIGHT;
        paddle->vel_y = -paddle->vel_y;
    } else if (paddle->y < 0) {
        paddle->y = 0;
        paddle->vel_y = -paddle->vel_y;
    }
    
    paddle->vel_x = copysign(MAX(abs(paddle->vel_x) - 0.5, 0), paddle->vel_x);
    paddle->x += paddle->vel_x;
    if (paddle_x >= WINDOW_WIDTH / 2) {
        if (paddle->x > WINDOW_WIDTH - PADDLE_WIDTH) {
            paddle->x = WINDOW_WIDTH - PADDLE_WIDTH;
            paddle->vel_x = -paddle->vel_x;
        } else if (paddle->x < WINDOW_WIDTH / 2) {
            paddle->x = WINDOW_WIDTH / 2;
            paddle->vel_x = -paddle->vel_x;
        }
    } else {
        if (paddle->x < 0) {
            paddle->x = 0;
            paddle->vel_x = -paddle->vel_x;
        } else if (paddle->x > WINDOW_WIDTH / 2 - PADDLE_WIDTH) {
            paddle->x = WINDOW_WIDTH / 2 - PADDLE_WIDTH;
            paddle->vel_x = -paddle->vel_x;
        }
    }
}

void ai_paddle(void) {
    if (p2.y + PADDLE_HEIGHT / 2 > ball.y + AI_FUDGE)
        p2.vel_y -= 2;
    else if (p2.y + PADDLE_HEIGHT / 2 < ball.y - AI_FUDGE)
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
        if (key_state.left_down)
            p1.vel_x -= 2;
        if (key_state.right_down)
            p1.vel_x += 2;
        
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
