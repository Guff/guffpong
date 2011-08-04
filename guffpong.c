#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <SDL.h>
#include <SDL_Pango.h>
#include <SDL_image.h>

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#define INT_MS        50
#define WINDOW_WIDTH  400
#define WINDOW_HEIGHT 400
#define BALL_RADIUS   10
#define BALL_MASS     1
#define BALL_PNG      "data/ball.png"
#define PADDLE_WIDTH  20
#define PADDLE_HEIGHT 150
#define PADDLE_MASS   15
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
    float vel_x, vel_y;
    float x, y;
    float radius;
    float mass;
    SDL_Surface *surface;
} ball_t;

typedef struct {
    float x, y;
    float vel_x, vel_y;
    float mass;
} paddle_t;

struct {
    unsigned int p1;
    unsigned int p2;
} scores = {0, 0};

ball_t ball;
paddle_t p1 = {P1_X, (WINDOW_HEIGHT - PADDLE_HEIGHT) / 2, 0, 0, PADDLE_MASS};
paddle_t p2 = {P2_X, (WINDOW_HEIGHT - PADDLE_HEIGHT) / 2, 0, 0, PADDLE_MASS};
SDLPango_Context *scores_context;
int update_rects_n = 0;
SDL_Rect *update_rects = NULL;

bool running;

void add_update(int x, int y, int w, int h) {
    update_rects = realloc(update_rects, sizeof(SDL_Rect) * (update_rects_n + 1));
    SDL_Rect rect = {x, y, w, h};
    update_rects[update_rects_n++] = rect;
}

void clear_updates(void) {
    free(update_rects);
    update_rects = NULL;
    update_rects_n = 0;
}

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
        SDL_Rect dash = {WINDOW_WIDTH / 2 - 3, 2 * i * 20, 6, 20};
        SDL_FillRect(screen, &dash, 0xffffffff);
    }
    
    add_update(WINDOW_WIDTH / 2 - 3, 0, 6, WINDOW_HEIGHT);
}

void draw_frame(SDL_Surface *screen) {
    SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0, 0, 0));
    draw_stippled_line(screen);
    
    // player 1's paddle
    SDL_Rect p1_rect = {p1.x, p1.y, PADDLE_WIDTH, PADDLE_HEIGHT};
    SDL_Rect p2_rect = {p2.x, p2.y, PADDLE_WIDTH, PADDLE_HEIGHT};
    SDL_FillRect(screen, &p1_rect, 0xffffffff);
    SDL_FillRect(screen, &p2_rect, 0xffffffff);
    // player 2's paddle
    
    // draw the circle
    SDL_Rect ball_rect = {ball.x - 10, ball.y - 10, 20, 20};
    SDL_BlitSurface(ball.surface, NULL, screen, &ball_rect);
    // score text
    char text[9];
    snprintf(text, 9, "%03i  %03i", scores.p2, scores.p1);
    SDLPango_SetText_GivenAlignment(scores_context, text, strlen(text),
                                    SDLPANGO_ALIGN_CENTER);
    SDL_Surface *surface = SDLPango_CreateSurfaceDraw(scores_context);
    add_update(0, 0, WINDOW_WIDTH, SDLPango_GetLayoutHeight(scores_context));
    SDL_BlitSurface(surface, NULL, screen, NULL);
    SDL_FreeSurface(surface);
    SDL_UpdateRects(screen, update_rects_n, update_rects);
}

void reset_ball(void) {
    float mul = rand() / (float) RAND_MAX;
    ball.vel_x = MAX(mul * 10, 10 - mul * 10);
    ball.vel_y = 10 - ball.vel_x;
    ball.x = WINDOW_WIDTH / 2;
    ball.y = WINDOW_HEIGHT / 2;
    ball.radius = BALL_RADIUS;
    ball.mass = BALL_MASS;
    
    ball.surface = IMG_Load("data/ball.png");
}

void collide(float m0, float *u0, float m1, float *u1) {
    float v0 = (*u0 * (m0 - m1) + 2 * m1 * (*u1)) / (m0 + m1);
    float v1 = (*u1 * (m1 - m0) + 2 * m0 * (*u0)) / (m0 + m1);
    *u0 = v0;
    *u1 = v1;
}

void ball_compute_position(void) {
    float pos_int_x = ball.x + ball.vel_x;
    float pos_int_y = ball.y + ball.vel_y;
    
    // top and bottom walls
    if (pos_int_y <= ball.radius || pos_int_y >= WINDOW_HEIGHT - ball.radius)
        ball.vel_y = -ball.vel_y;
    // p1's paddle
    if (pos_int_y > p1.y && pos_int_y < p1.y + PADDLE_HEIGHT &&
        (pos_int_x + ball.radius) >= p1.x &&
        (pos_int_x + ball.radius) <= p1.x + PADDLE_WIDTH) {
            collide(ball.mass, &ball.vel_x, p1.mass, &p1.vel_x);
            ball.vel_y += p1.vel_y / 4;
    }
    // p2's paddle
    if (pos_int_y >= p2.y && pos_int_y <= p2.y + PADDLE_HEIGHT &&
        (pos_int_x - ball.radius) <= p2.x + PADDLE_WIDTH &&
        (pos_int_x - ball.radius) >= p2.x) {
            collide(ball.mass, &ball.vel_x, p2.mass, &p2.vel_x);
            ball.vel_y += p2.vel_y / 4;
    }
    
    add_update(ball.x - ball.radius, ball.y - ball.radius, 2 * ball.radius,
               2 * ball.radius);

    if (pos_int_x <= ball.radius) { // left wall
        scores.p1++, reset_ball();
    }
    else if (pos_int_x >= WINDOW_WIDTH - ball.radius) // right wall
        scores.p2++, reset_ball();
    
    
    ball.x += ball.vel_x;
    ball.y += ball.vel_y;
    
    add_update(ball.x - ball.radius, ball.y - ball.radius, 2 * ball.radius ,
               2 * ball.radius);
}

void paddle_move(paddle_t *paddle) {
    // use the current position of the paddle to determine which box it's in
    float paddle_x = paddle->x, paddle_y = paddle->y;
    
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
    
    int x, y, w, h;
    x = MIN(paddle->x, paddle_x);
    y = MIN(paddle->y, paddle_y);
    w = MAX(paddle->x, paddle_x) - x + PADDLE_WIDTH;
    h = MAX(paddle->y, paddle_y) - y + PADDLE_HEIGHT;
    
    add_update(x, y, w, h);
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
    
    scores_context = SDLPango_CreateContext_GivenFontDesc("Sans 20");
    SDLPango_SetDefaultColor(scores_context,
                             MATRIX_TRANSPARENT_BACK_WHITE_LETTER);
    SDLPango_SetMinimumSize(scores_context, WINDOW_WIDTH, 0);
    
    while (running) {
        int loop_time = SDL_GetTicks();
        
        get_input();

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
        
        draw_frame(screen);
        
        clear_updates();
        
        SDL_Delay(MAX(0, INT_MS + (signed) (loop_time - SDL_GetTicks())));
    }
}

int main(int argc, char **argv) {    
    SDL_Init(SDL_INIT_VIDEO);
    
    SDLPango_Init();
    IMG_Init(IMG_INIT_PNG);
    
    SDL_WM_SetCaption("guffpong", "guffpong");
    
    SDL_Surface *screen = SDL_SetVideoMode(WINDOW_WIDTH, WINDOW_HEIGHT, 16,
                                           SDL_HWSURFACE);
    SDL_ShowCursor(false);
    
    running = true;
    
    game_loop(screen);
    
    return 0;
}
