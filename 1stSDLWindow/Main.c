#include <SDL.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

#define WINDOW_WIDTH     800
#define WINDOW_HEIGHT    600
#define BRICK_WIDTH       80
#define BRICK_HEIGHT      30
#define BRICK_ROWS        10
#define BRICK_COLS        10
#define PADDLE_WIDTH     100
#define PADDLE_HEIGHT     15
#define BALL_SIZE         10
#define BALL_SPEED         5.f
#define MAX_BALL_SPEED    10.f
#define SPEED_INCREASE    0.1f
#define SCORE_THRESHOLD   50
#define BUTTON_WIDTH     200
#define BUTTON_HEIGHT     60
#define FPS               60
#define DT               (1.0f/FPS)

typedef struct {
    float  x, y, w, h;
    int    active;
    int    type;            /* 1 = gray, 2 = orange, 3 = brown (hard) */
    float  fade;            /* For brick fade animation */
} GameObject;

typedef struct {
    float  dx, dy;          /* Ball velocity */
    int    score;
} GameState;

typedef enum {
    SFX_BRICK1,
    SFX_BRICK2,
    SFX_PADDLE,
    SFX_BALL_LOST,
    SFX_GAME_OVER,
    SFX_TOTAL
} SFXId;

static Mix_Chunk* g_sfx[SFX_TOTAL] = { NULL };
static Mix_Music* g_music = NULL;

static void log_sdl_err(const char* msg)
{
    fprintf(stderr, "%s – SDL Error: %s\n", msg, SDL_GetError());
}

static void quit_with_error(const char* msg)
{
    log_sdl_err(msg);
    SDL_Quit();
    exit(EXIT_FAILURE);
}

static int load_audio(void)
{
    const struct { SFXId id; const char* path; } table[] = {
        { SFX_BRICK1,   "sfx/retro_brick1.wav" },
        { SFX_BRICK2,   "sfx/retro_brick2.wav" },
        { SFX_PADDLE,   "sfx/retro_paddle.wav" },
        { SFX_BALL_LOST,"sfx/retro_ball_lost.wav"  },
        { SFX_GAME_OVER,"sfx/retro_game_over.wav"  },
    };

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 512) < 0) {
        fprintf(stderr, "Mix_OpenAudio: %s\n", Mix_GetError());
        return false;
    }
    for (size_t i = 0; i < sizeof table / sizeof * table; ++i) {
        g_sfx[table[i].id] = Mix_LoadWAV(table[i].path);
        if (!g_sfx[table[i].id]) {
            fprintf(stderr, "Failed to load %s – %s\n", table[i].path, Mix_GetError());
            return false;
        }
    }
    g_music = Mix_LoadMUS("sfx/synthwave_bg.wav");
    if (!g_music) {
        fprintf(stderr, "Failed to load music: %s\n", Mix_GetError());
        return false;
    }
    Mix_VolumeMusic(MIX_MAX_VOLUME / 4);
    Mix_PlayMusic(g_music, -1);
    return true;
}

static void play_sfx(SFXId id)
{
    if (g_sfx[id])
        Mix_PlayChannel(-1, g_sfx[id], 0);
}

static void init_bricks(GameObject* bricks)
{
    srand((unsigned)time(NULL));
    int step = BRICK_WIDTH - 5;  // 75 for BRICK_WIDTH=80
    int brick_width = BRICK_WIDTH - 10;  // 70
    int total_layout_width = (BRICK_COLS - 1) * step + brick_width;  // 745
    int starting_x = (WINDOW_WIDTH - total_layout_width) / 2;  // ~28
    for (int r = 0; r < BRICK_ROWS; ++r) {
        for (int c = 0; c < BRICK_COLS; ++c) {
            GameObject* b = &bricks[r * BRICK_COLS + c];
            b->x = starting_x + c * step;
            b->y = r * (BRICK_HEIGHT - 5) + 80;
            b->w = brick_width;
            b->h = BRICK_HEIGHT - 10;
            b->active = 1;
            b->type = rand() % 3 + 1;
            b->fade = 1.0f;
        }
    }
}

static void reset_game(GameObject* bricks, GameObject* paddle, GameObject* ball, GameState* st)
{
    init_bricks(bricks);
    paddle->w = PADDLE_WIDTH;
    paddle->h = PADDLE_HEIGHT;
    paddle->x = WINDOW_WIDTH / 2 - paddle->w / 2;
    paddle->y = WINDOW_HEIGHT - 60;
    paddle->active = 1;
    ball->w = ball->h = BALL_SIZE;
    ball->x = WINDOW_WIDTH / 2;
    ball->y = paddle->y - BALL_SIZE - 4;
    ball->active = 1;
    st->dx = BALL_SPEED;
    st->dy = -BALL_SPEED;
    st->score = 0;
}

static void update_ball(GameObject* ball, GameObject* paddle, GameObject* bricks, GameState* st, int* game_over)
{
    if (!ball->active) return;

    float current_speed = BALL_SPEED + (st->score / SCORE_THRESHOLD) * SPEED_INCREASE;
    if (current_speed > MAX_BALL_SPEED) current_speed = MAX_BALL_SPEED;
    st->dx = (st->dx / fabsf(st->dx)) * current_speed;
    st->dy = (st->dy / fabsf(st->dy)) * current_speed;

    ball->x += st->dx;
    ball->y += st->dy;

    if (ball->x <= 0) {
        st->dx = fabsf(st->dx);
        ball->x = 0;
    }
    else if (ball->x >= WINDOW_WIDTH - ball->w) {
        st->dx = -fabsf(st->dx);
        ball->x = WINDOW_WIDTH - ball->w;
    }
    if (ball->y >= WINDOW_HEIGHT) {
        play_sfx(SFX_BALL_LOST);
        *game_over = 1;
        return;
    }

    if (ball->y < 80) {
        ball->y = 80;
        st->dy = fabsf(st->dy);
    }

    if (st->dy > 0 && ball->y + ball->h >= paddle->y &&
        ball->x + ball->w >= paddle->x && ball->x <= paddle->x + paddle->w) {
        play_sfx(SFX_PADDLE);
        ball->y = paddle->y - ball->h;
        float hit = (ball->x + ball->w / 2.f) - (paddle->x + paddle->w / 2.f);
        st->dx = hit * 0.05f * current_speed;
        st->dy = -fabsf(st->dy) * current_speed;
    }

    for (int i = 0; i < BRICK_ROWS * BRICK_COLS; ++i) {
        GameObject* b = &bricks[i];
        if (!b->active || b->fade < 1.0f) continue;

        if (ball->x + ball->w > b->x && ball->x < b->x + b->w &&
            ball->y + ball->h > b->y && ball->y < b->y + b->h) {
            play_sfx(rand() & 1 ? SFX_BRICK1 : SFX_BRICK2);

            float overlap_left = ball->x + ball->w - b->x;
            float overlap_right = b->x + b->w - ball->x;
            float overlap_top = ball->y + ball->h - b->y;
            float overlap_bottom = b->y + b->h - ball->y;
            float min_overlap = fminf(fminf(overlap_left, overlap_right), fminf(overlap_top, overlap_bottom));

            if (min_overlap == overlap_left) {
                st->dx = -fabsf(st->dx);
                ball->x = b->x - ball->w;
            }
            else if (min_overlap == overlap_right) {
                st->dx = fabsf(st->dx);
                ball->x = b->x + b->w;
            }
            else if (min_overlap == overlap_top) {
                st->dy = -fabsf(st->dy);
                ball->y = b->y - ball->h;
            }
            else {
                st->dy = fabsf(st->dy);
                ball->y = b->y + b->h;
            }

            if (b->type == 3) {
                b->type = 2;
                st->score += 5;
            }
            else {
                b->active = 0;
                b->fade = 0.9f;
                st->score += (b->type == 2) ? 10 : 5;
            }
            break;
        }
    }
}

static void draw_filled_rect(SDL_Renderer* r, SDL_Rect* rect, SDL_Color c)
{
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
    SDL_RenderFillRect(r, rect);
}

static void draw_text(SDL_Renderer* r, TTF_Font* f, const char* txt, int x, int y, SDL_Color col)
{
    SDL_Surface* s = TTF_RenderText_Blended(f, txt, col);
    SDL_Texture* t = SDL_CreateTextureFromSurface(r, s);
    SDL_Rect dst = { x, y, s->w, s->h };
    SDL_RenderCopy(r, t, NULL, &dst);
    SDL_FreeSurface(s);
    SDL_DestroyTexture(t);
}

static void render(SDL_Renderer* ren, TTF_Font* font, TTF_Font* titleFont, GameObject* bricks, GameObject* paddle,
    GameObject* ball, GameState* st, int game_over, GameObject* btn,
    float btn_phase, int hover, float paddle_phase)
{
    SDL_SetRenderDrawColor(ren, 18, 18, 28, 255);
    SDL_RenderClear(ren);

    const char* title = "ShatterBlocks";
    SDL_Color colors[] = {
        {255, 0, 0, 255},    // Neon Red
        {255, 165, 0, 255},  // Neon Orange
        {255, 255, 0, 255},  // Neon Yellow
        {0, 255, 0, 255},    // Neon Green
        {0, 255, 255, 255},  // Neon Cyan
        {255, 105, 180, 255},// Neon Pink
        {255, 0, 255, 255}   // Neon Purple
    };
    int num_colors = sizeof(colors) / sizeof(colors[0]);
    int total_w = 0;
    for (const char* p = title; *p; ++p) {
        char ch[2] = { *p, '\0' };
        int w, h;
        TTF_SizeText(titleFont, ch, &w, &h);
        total_w += w;
    }
    int title_x = (WINDOW_WIDTH - total_w) / 2;
    int title_y = 10;
    int color_idx = 0;
    for (const char* p = title; *p; ++p) {
        char ch[2] = { *p, '\0' };
        draw_text(ren, titleFont, ch, title_x, title_y, colors[color_idx % num_colors]);
        int w, h;
        TTF_SizeText(titleFont, ch, &w, &h);
        title_x += w;
        color_idx++;
    }

    for (int i = 0; i < BRICK_ROWS * BRICK_COLS; ++i) {
        GameObject* b = &bricks[i];
        if (!b->active && b->fade <= 0) continue;
        if (!b->active) b->fade -= 0.05f;
        SDL_Color c = { 150, 150, 150, (Uint8)(255 * b->fade) }; // Gray
        if (b->type == 2) c = (SDL_Color){ 255, 165, 0, (Uint8)(255 * b->fade) }; // Orange
        if (b->type == 3) c = (SDL_Color){ 139, 69, 19, (Uint8)(255 * b->fade) }; // Brown
        SDL_Rect rect = { (int)b->x, (int)b->y, (int)b->w, (int)b->h };
        draw_filled_rect(ren, &rect, c);
    }

    float paddle_scale = 1.0f + 0.05f * sinf(paddle_phase);
    SDL_Rect pRect = {
        (int)(paddle->x - (paddle_scale - 1) * paddle->w / 2),
        (int)paddle->y,
        (int)(paddle->w * paddle_scale),
        (int)paddle->h
    };
    draw_filled_rect(ren, &pRect, (SDL_Color) { 0, 255, 255, 255 }); // Cyan

    SDL_Rect bRect = { (int)ball->x, (int)ball->y, (int)ball->w, (int)ball->h };
    draw_filled_rect(ren, &bRect, (SDL_Color) { 255, 255, 255, 255 }); // White

    char buf[32];
    snprintf(buf, sizeof buf, "Score: %d", st->score);
    TTF_Font* scoreFont = TTF_OpenFont("beon.ttf", 18);
    if (scoreFont) {
        int score_w, score_h;
        TTF_SizeText(scoreFont, buf, &score_w, &score_h);
        draw_text(ren, scoreFont, buf, WINDOW_WIDTH - score_w - 10, 20, (SDL_Color) { 255, 255, 0, 255 }); // Yellow
        TTF_CloseFont(scoreFont);
    }

    if (game_over) {
        static int alpha = 0;
        if (alpha < 200) alpha += 8;
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
        SDL_Rect full = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
        draw_filled_rect(ren, &full, (SDL_Color) { 0, 0, 0, (Uint8)alpha });

        int box_w = 400, box_h = 300;
        int box_x = (WINDOW_WIDTH - box_w) / 2;
        int box_y = (WINDOW_HEIGHT - box_h) / 2;
        SDL_Rect borderRect = { box_x - 5, box_y - 5, box_w + 10, box_h + 10 };
        SDL_SetRenderDrawColor(ren, 255, 0, 255, 255); // Purple border
        SDL_RenderDrawRect(ren, &borderRect);
        SDL_Rect boxRect = { box_x, box_y, box_w, box_h };
        draw_filled_rect(ren, &boxRect, (SDL_Color) { 50, 50, 50, 200 });

        TTF_SetFontStyle(titleFont, TTF_STYLE_BOLD);
        int go_w, go_h;
        TTF_SizeText(titleFont, "GAME OVER", &go_w, &go_h);
        int go_x = box_x + (box_w - go_w) / 2;
        int go_y = box_y + 50;
        draw_text(ren, titleFont, "GAME OVER", go_x, go_y, (SDL_Color) { 255, 0, 0, 255 }); // Red
        TTF_SetFontStyle(titleFont, TTF_STYLE_NORMAL);

        snprintf(buf, sizeof buf, "Final Score: %d", st->score);
        TTF_Font* finalScoreFont = TTF_OpenFont("beon.ttf", 30);  // 30pt font for final score
        if (finalScoreFont) {
            TTF_SetFontStyle(finalScoreFont, TTF_STYLE_BOLD);  // Bold for emphasis
            int final_w, final_h;
            TTF_SizeText(finalScoreFont, buf, &final_w, &final_h);
            int final_x = box_x + (box_w - final_w) / 2;
            int final_y = go_y + go_h + 20;  // Below "GAME OVER"
            draw_text(ren, finalScoreFont, buf, final_x, final_y, (SDL_Color) { 255, 255, 0, 255 }); // Yellow
            TTF_CloseFont(finalScoreFont);
        }

        float s = 1.f + 0.05f * sinf(btn_phase);
        int btn_w = BUTTON_WIDTH * s;
        int btn_h = BUTTON_HEIGHT * s;
        int btn_x = box_x + (box_w - btn_w) / 2;
        int btn_y = box_y + 150;
        SDL_Rect btnRect = { btn_x, btn_y, btn_w, btn_h };
        SDL_Color bc = hover ? (SDL_Color) { 0, 255, 0, 255 } : (SDL_Color) { 0, 200, 0, 255 }; // Green
        draw_filled_rect(ren, &btnRect, bc);
        draw_text(ren, font, "Restart", btn_x + btn_w / 2 - 45, btn_y + btn_h / 2 - 12, (SDL_Color) { 255, 255, 0, 255 }); // Yellow
    }

    SDL_RenderPresent(ren);
}

int main(void)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
        quit_with_error("SDL_Init");

    if (TTF_Init() < 0)
        quit_with_error("TTF_Init");

    if (!load_audio())
        quit_with_error("Audio load");

    SDL_Window* win = SDL_CreateWindow("ShatterBlocks",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if (!win) quit_with_error("SDL_CreateWindow");

    SDL_Renderer* ren = SDL_CreateRenderer(win, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ren) quit_with_error("SDL_CreateRenderer");

    TTF_Font* font = TTF_OpenFont("beon.ttf", 24);
    TTF_Font* titleFont = TTF_OpenFont("beon.ttf", 48);
    if (!font || !titleFont) {
        fprintf(stderr, "Font load error: %s\n", TTF_GetError());
        quit_with_error("TTF_OpenFont");
    }

    GameObject* bricks = calloc(BRICK_ROWS * BRICK_COLS, sizeof * bricks);
    GameObject* paddle = malloc(sizeof * paddle);
    GameObject* ball = malloc(sizeof * ball);
    GameState st;
    GameObject restartBtn;

    int box_w = 400, box_h = 300;
    int box_x = (WINDOW_WIDTH - box_w) / 2;
    int box_y = (WINDOW_HEIGHT - box_h) / 2;
    restartBtn.x = box_x + (box_w - BUTTON_WIDTH) / 2;
    restartBtn.y = box_y + 150;
    restartBtn.w = BUTTON_WIDTH;
    restartBtn.h = BUTTON_HEIGHT;
    restartBtn.active = 1;

    reset_game(bricks, paddle, ball, &st);

    int running = true, paused = false, game_over = false;
    Uint32 lastTicks = SDL_GetTicks();
    float btn_phase = 0.f, paddle_phase = 0.f;

    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_ESCAPE) running = false;
                if (!game_over) {
                    if (e.key.keysym.sym == SDLK_SPACE) {
                        paused = !paused;  // Toggle pause during gameplay
                        play_sfx(SFX_PADDLE);  // Add sound feedback
                    }
                }
                else if (game_over && e.key.keysym.sym == SDLK_SPACE) {
                    play_sfx(SFX_PADDLE);
                    game_over = false;
                    btn_phase = 0.f;
                    reset_game(bricks, paddle, ball, &st);
                }
            }
            if (game_over && e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
                int mx = e.button.x, my = e.button.y;
                if (mx >= restartBtn.x && mx <= restartBtn.x + restartBtn.w &&
                    my >= restartBtn.y && my <= restartBtn.y + restartBtn.h) {
                    play_sfx(SFX_PADDLE);
                    game_over = false;
                    btn_phase = 0.f;
                    reset_game(bricks, paddle, ball, &st);
                }
            }
        }

        Uint32 now = SDL_GetTicks();
        float dt = (now - lastTicks) / 1000.f;
        if (dt < DT) { SDL_Delay((Uint32)((DT - dt) * 1000)); continue; }
        lastTicks = now;

        const Uint8* keys = SDL_GetKeyboardState(NULL);
        if (!paused && !game_over) {
            float paddle_speed = 400.f;
            if (keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_RCTRL]) paddle_speed = 800.f;
            if (keys[SDL_SCANCODE_LEFT] && paddle->x > 0) paddle->x -= paddle_speed * dt;
            if (keys[SDL_SCANCODE_RIGHT] && paddle->x < WINDOW_WIDTH - paddle->w)
                paddle->x += paddle_speed * dt;
            update_ball(ball, paddle, bricks, &st, &game_over);
            if (game_over) play_sfx(SFX_GAME_OVER);
            paddle_phase += dt * 4.f;
        }

        if (game_over) btn_phase += dt * 6.f;

        int mx, my;
        SDL_GetMouseState(&mx, &my);
        int hover = (mx >= restartBtn.x && mx <= restartBtn.x + restartBtn.w &&
            my >= restartBtn.y && my <= restartBtn.y + restartBtn.h);

        render(ren, font, titleFont, bricks, paddle, ball, &st, game_over, &restartBtn, btn_phase, hover, paddle_phase);
    }

    for (int i = 0; i < SFX_TOTAL; ++i) Mix_FreeChunk(g_sfx[i]);
    Mix_FreeMusic(g_music);
    free(bricks); free(paddle); free(ball);
    TTF_CloseFont(font);
    TTF_CloseFont(titleFont);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    Mix_CloseAudio();
    TTF_Quit();
    SDL_Quit();
    return 0;
}