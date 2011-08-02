#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <clutter/clutter.h>

#define INT_MS        50
#define WINDOW_WIDTH  400
#define WINDOW_HEIGHT 400
#define PADDLE_WIDTH  20
#define PADDLE_HEIGHT 150
#define BALL_RADIUS   10
#define AI_FUDGE      0.5
#define P1_X          370
#define P2_X          (WINDOW_HEIGHT - P1_X - PADDLE_WIDTH)

struct {
    gboolean up_down;
    gboolean down_down;
} key_state = {FALSE, FALSE};

typedef struct {
    gdouble x, y;
    gdouble width, height;
} rectangle_t;

typedef struct {
    gdouble vel_x, vel_y;
    gdouble x, y;
    gdouble radius;
    ClutterActor *actor;
} ball_t;

typedef struct {
    gdouble x;
    gdouble y;
    gdouble vel_y;
    ClutterActor *actor;
} paddle_t;

struct {
    guint p1;
    guint p2;
} scores = {0, 0};

ClutterActor *scores_text = NULL;

static GOptionEntry options[] = {
    // get gcc to shut up about missing initializers in this instance
    { NULL, 0, 0, 0, NULL, NULL, NULL }
};

ball_t ball;
paddle_t p1 = {P1_X, (WINDOW_HEIGHT - PADDLE_HEIGHT) / 2, 0, NULL};
paddle_t p2 = {P2_X, (WINDOW_HEIGHT - PADDLE_HEIGHT) / 2, 0, NULL};

gboolean running;

gboolean key_press(ClutterActor *actor, ClutterEvent *ev, gpointer data) {
    switch (((ClutterKeyEvent *)ev)->keyval) {
        case CLUTTER_KEY_Escape:
            running = FALSE;
            break;
        case CLUTTER_KEY_Up:
            key_state.up_down = TRUE;
            break;
        case CLUTTER_KEY_Down:
            key_state.down_down = TRUE;
        default:
            break;
    }
    
    return TRUE;
}

gboolean key_release(ClutterActor *actor, ClutterEvent *ev, gpointer data) {
    switch (((ClutterKeyEvent *)ev)->keyval) {
        case CLUTTER_KEY_Up:
            key_state.up_down = FALSE;
            break;
        case CLUTTER_KEY_Down:
            key_state.down_down = FALSE;
            break;
        default:
            break;
    }
    
    return TRUE;
}

void draw_stippled_line(ClutterActor *stage) {
    ClutterActor *actor = clutter_cairo_texture_new(4, WINDOW_HEIGHT);
    cairo_t *cr = clutter_cairo_texture_create(CLUTTER_CAIRO_TEXTURE(actor));
    cairo_set_source_rgb(cr, 1, 1, 1);
    gdouble dash = 20;
    cairo_set_dash(cr, &dash, 1, 10);
    cairo_move_to(cr, 2, 0);
    cairo_line_to(cr, 2, WINDOW_HEIGHT);
    cairo_set_line_width(cr, 4);
    cairo_stroke(cr);
    cairo_destroy(cr);
    
    clutter_actor_set_position(actor, WINDOW_WIDTH / 2, 0);
    clutter_container_add_actor(CLUTTER_CONTAINER(stage), actor);
}

gboolean point_in_rect(int x, int y, rectangle_t rect) {
    return (x >= rect.x && x <= rect.x + rect.width &&
            y >= rect.y && y <= rect.y + rect.height);
}

gboolean ball_will_collide(double x, double y, double width, double height) {
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
    if (ball_will_collide(0, -100, WINDOW_WIDTH, 100)) // top wall
        ball.vel_y = -ball.vel_y;
    else if (ball_will_collide(WINDOW_WIDTH, 0, 100, WINDOW_HEIGHT)) // right
        scores.p2++, reset_ball();
    else if (ball_will_collide(0, WINDOW_HEIGHT, WINDOW_WIDTH, 100)) // bottom
        ball.vel_y = -ball.vel_y;
    else if (ball_will_collide(-100, 0, 100, WINDOW_HEIGHT)) // left
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
    
    clutter_actor_set_position(ball.actor, ball.x, ball.y);
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
    
    clutter_actor_set_position(paddle->actor, paddle->x, paddle->y);
}

void ai_paddle(void) {
    if (p2.y + PADDLE_HEIGHT / 2 > ball.y + AI_FUDGE)
        p2.vel_y -= 2;
    else if (p2.y + PADDLE_HEIGHT / 2 < ball.y - AI_FUDGE)
        p2.vel_y += 2;
}

void update_score_text(void) {
    char score_string[8];
    snprintf(score_string, 8, "%3i %3i", scores.p2, scores.p1);
    clutter_text_set_text(CLUTTER_TEXT(scores_text), score_string);
}

gboolean calc_frame(gpointer data) {
    ClutterActor *stage = data;
    
    if (!running)
        clutter_main_quit();
    
    if (key_state.up_down)
        p1.vel_y -= 2;
    if (key_state.down_down)
        p1.vel_y += 2;
    
    ai_paddle();
    
    paddle_move(&p1);
    paddle_move(&p2);
    
    ball_compute_position();
    
    update_score_text();
    
    return TRUE;
}

void create_actors(ClutterActor *stage) {
    cairo_t *cr;
    ball.actor = clutter_cairo_texture_new(BALL_RADIUS * 2, BALL_RADIUS * 2);
    cr = clutter_cairo_texture_create(CLUTTER_CAIRO_TEXTURE(ball.actor));
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_arc(cr, BALL_RADIUS, BALL_RADIUS, BALL_RADIUS, 0, 2 * M_PI);
    cairo_fill(cr);
    cairo_destroy(cr);
    
    p1.actor = clutter_cairo_texture_new(PADDLE_WIDTH, PADDLE_HEIGHT);
    cr = clutter_cairo_texture_create(CLUTTER_CAIRO_TEXTURE(p1.actor));
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_rectangle(cr, 0, 0, PADDLE_WIDTH, PADDLE_HEIGHT);
    cairo_fill(cr);
    cairo_destroy(cr);
    
    p2.actor = clutter_cairo_texture_new(PADDLE_WIDTH, PADDLE_HEIGHT);
    cr = clutter_cairo_texture_create(CLUTTER_CAIRO_TEXTURE(p2.actor));
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_rectangle(cr, 0, 0, PADDLE_WIDTH, PADDLE_HEIGHT);
    cairo_fill(cr);
    cairo_destroy(cr);
    
    scores_text = clutter_text_new();
    clutter_text_set_font_name(CLUTTER_TEXT(scores_text), "Sans 20");
    clutter_text_set_line_alignment(CLUTTER_TEXT(scores_text), PANGO_ALIGN_CENTER);
    ClutterColor *text_color = clutter_color_new(255, 255, 255, 255);
    clutter_text_set_color(CLUTTER_TEXT(scores_text), text_color);
    clutter_color_free(text_color);
    clutter_actor_set_anchor_point_from_gravity(scores_text, CLUTTER_GRAVITY_NORTH);
    clutter_actor_set_position(scores_text, WINDOW_WIDTH / 2, 0);
    
    clutter_container_add_actor(CLUTTER_CONTAINER(stage), ball.actor);
    clutter_container_add_actor(CLUTTER_CONTAINER(stage), p1.actor);
    clutter_container_add_actor(CLUTTER_CONTAINER(stage), p2.actor);
    clutter_container_add_actor(CLUTTER_CONTAINER(stage), scores_text);
    
    clutter_actor_set_anchor_point_from_gravity(ball.actor,
                                                CLUTTER_GRAVITY_CENTER);
    clutter_actor_set_position(ball.actor, ball.x, ball.y);
    clutter_actor_set_position(p1.actor, P1_X, p1.y);
    clutter_actor_set_position(p2.actor, P2_X, p2.y);
}

int main(int argc, char **argv) {    
    GError *error = NULL;
    GOptionContext *context = g_option_context_new("- pong clone");
    g_option_context_add_main_entries(context, options, NULL);
    g_option_context_add_group(context, clutter_get_option_group());
    if (!g_option_context_parse(context, &argc, &argv, &error)) {
        g_print("option parsing failed: %s\n", error->message);
        exit(1);
    }
    
    srand(time(NULL));
    
    ClutterActor *stage = clutter_stage_get_default();
    clutter_actor_set_size(stage, WINDOW_WIDTH, WINDOW_HEIGHT);
    
    clutter_stage_hide_cursor(CLUTTER_STAGE(stage));
    ClutterColor *bg_color = clutter_color_new(0, 0, 0, 1);
    clutter_stage_set_color(CLUTTER_STAGE(stage), bg_color);
    clutter_color_free(bg_color);
    clutter_stage_set_user_resizable(CLUTTER_STAGE(stage), FALSE);
    
    g_signal_connect(stage, "key-press-event", G_CALLBACK(key_press), NULL);
    g_signal_connect(stage, "key-release-event", G_CALLBACK(key_release), NULL);
    
    running = TRUE;
    
    draw_stippled_line(stage);
    
    reset_ball();
    
    create_actors(stage);
    clutter_actor_show(stage);
    
    g_timeout_add(INT_MS, calc_frame, stage);
        
    clutter_main();
    
    //game_loop(stage);
    
    return 0;
}
