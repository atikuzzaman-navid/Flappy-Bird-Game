#include "iGraphics.h"
#include "iSound.h"
#include <Windows.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define N_CLOUDS 4
#define N_PIPES 8
#define SCREEN_WIDTH 1000
#define SCREEN_HEIGHT 600

#define N_COINS 5
#define COIN_WIDTH (SCREEN_WIDTH * 0.04f)
#define COIN_HEIGHT (SCREEN_HEIGHT * 0.06f)
#define COIN_SPEED (SCREEN_WIDTH * 0.01f)
#define COIN_FRAMES 6

#define BIRD_WIDTH (SCREEN_WIDTH * 0.07f)
#define BIRD_HEIGHT (SCREEN_HEIGHT * 0.1f)
#define PIPE_WIDTH (SCREEN_WIDTH * 0.08f)

#define GRAVITY 0.5f
#define JUMP_VELOCITY 10.0f
#define PIPE_SPEED (SCREEN_WIDTH * 0.01f)

#define N_FRAMES 8

#define MAX_SCORES 5
#define NAME_LENGTH 20
#define MIN_PIPE_SPACING (SCREEN_WIDTH * 0.35f)
#define EXTRA_PIPE_SPACING (SCREEN_WIDTH * 0.2f)
#define MIN_PIPE_HEIGHT (SCREEN_HEIGHT * 0.05f)
#define MAX_PIPE_GAP_Y (SCREEN_HEIGHT * 0.6f)

bool showCursor = true;
int cursorTimer;

typedef struct {
    char name[NAME_LENGTH];
    int score;
} HighScoreEntry;

HighScoreEntry highScores[MAX_SCORES];

void updateClouds();
void updateBirdAnimation();
void updateGame();
void updateCoins();
void updateCoinAnimation();

Image startstr;
Image homestr;
Image setstr;
Image levelstr;
Image aboutstr;
Image rulesstr;
Image scorestr;

Image cloudImages[N_CLOUDS];
float cloud_x[N_CLOUDS];
float cloud_y[N_CLOUDS];

Image lowerPipeImages[N_PIPES];
Image upperPipeImages[N_PIPES];

Image BG;
Image BG1;
Image BG2;
Image birdFrames[N_FRAMES];
int flyingFrame = 0;

Image coinFrames[N_COINS][COIN_FRAMES];
float coin_x[N_COINS];
float coin_y[N_COINS];
int coinFrameIndex[N_COINS] = {0};

int pipe_x[N_PIPES];
int pipe_gap_y[N_PIPES];

float bird_x;
float bird_y;
float bird_velocity = 0;

int score = 0;

bool gameOver = false;

bool showDeathFlash = false;
int deathFlashTimer = 0;
const int DEATH_FLASH_DURATION = 10;

bool enteringName = false;
char currentPlayerName[NAME_LENGTH] = "";
int nameCharIndex = 0;

bool scoreCountedPerPipe[N_PIPES] = { false };

int animTimer, physicsTimer, coinAnimTimer;

int PIPE_GAP;
int gameState = 0;

int difficulty = 0;
int mainScore = 0;
static int highScore = 0;

void startPage() {
    iShowLoadedImage(0, 0, &startstr);
}

void homePage() {
    iShowLoadedImage(0, 0, &homestr);
}

void aboutPage() {
    iShowLoadedImage(0, 0, &aboutstr);
}

void levelPage() {
    iShowLoadedImage(0, 0, &levelstr);
}

void rulesPage(){
    iShowLoadedImage(0, 0, &rulesstr);
}

void loadHighScores() {
    FILE* file = fopen("highscore.txt", "r");
    if (file) {
        for (int i = 0; i < MAX_SCORES; i++) {
            if (fscanf(file, "%s %d", highScores[i].name, &highScores[i].score) != 2) break;
        }
        fclose(file);
    } else {
        for (int i = 0; i < MAX_SCORES; i++) {
            strcpy(highScores[i].name, "---");
            highScores[i].score = 0;
        }
    }
}

void saveHighScores() {
    FILE* file = fopen("highscore.txt", "w");
    if (file) {
        for (int i = 0; i < MAX_SCORES; i++) {
            fprintf(file, "%s %d\n", highScores[i].name, highScores[i].score);
        }
        fclose(file);
    }
}

void insertHighScore(const char* name, int newScore) {
    for (int i = 0; i < MAX_SCORES; i++) {
        if (newScore > highScores[i].score) {
            for (int j = MAX_SCORES - 1; j > i; j--) {
                highScores[j] = highScores[j - 1];
            }
            strncpy(highScores[i].name, name, NAME_LENGTH);
            highScores[i].name[NAME_LENGTH - 1] = '\0';
            highScores[i].score = newScore;
            break;
        }
    }
    saveHighScores();
}

void scorePage() {
    iShowLoadedImage(0, 0, &scorestr);
    iSetColor(255, 255, 255);
    iText(SCREEN_WIDTH * 0.15f, SCREEN_HEIGHT * 0.8f, "High Scores:", GLUT_BITMAP_TIMES_ROMAN_24);
    for (int i = 0; i < MAX_SCORES; i++) {
        char line[100];
        sprintf(line, "%d. %s - %d", i + 1, highScores[i].name, highScores[i].score);
        iText(SCREEN_WIDTH * 0.15f, SCREEN_HEIGHT * 0.72f - i * SCREEN_HEIGHT * 0.08f, line, GLUT_BITMAP_HELVETICA_18);
    }
}

void gameOverPage() {
    iShowLoadedImage(0, 0, &BG);
    iSetColor(0, 0, 255);
    char scoreString[4];

    if (difficulty == 0) {
        mainScore = score / 7;
    } else if (difficulty == 1) {
        mainScore = score / 5;
    } else if (difficulty == 2) {
        mainScore = score / 3;
    }

    sprintf(scoreString, "%d", mainScore);
    iText(SCREEN_WIDTH * 0.57f, SCREEN_HEIGHT * 0.56f, scoreString);

    FILE* hScore = fopen("highScore.txt", "r");
    int scores[5] = {0, 0, 0, 0, 0};
    int i = 0;
    char str[100];

    while (fgets(str, 100, hScore) != NULL && i < 5) {
        scores[i] = atoi(str);
        i++;
    }

    int flag = 1;
    for (int i = 0; i < 5; i++) {
        if (mainScore == scores[i]) {
            flag = 0;
        }
    }

    if (flag) {
        for (int i = 0; i < 5; i++) {
            if (mainScore > scores[i]) {
                for (int j = 4; j > i; j--) {
                    scores[j] = scores[j - 1];
                }
                scores[i] = mainScore;
                break;
            }
        }
    }

    fclose(hScore);

    hScore = fopen("highScore.txt", "w");
    for (int i = 0; i < 5; i++) {
        fprintf(hScore, "%d\n", scores[i]);
    }
    fclose(hScore);

    hScore = fopen("highScore.txt", "r");
    char highScoreString[3];
    if (fgets(highScoreString, 3, hScore) != NULL) {
        iSetColor(0, 0, 255);
        iText(SCREEN_WIDTH * 0.68f, SCREEN_HEIGHT * 0.46f, highScoreString);
    }
    fclose(hScore);
}

void resetGame() {
    if (gameOver && score > 0) {
        enteringName = true;
        nameCharIndex = 0;
        currentPlayerName[0] = '\0';
        iResumeTimer(cursorTimer);
        return;
    }

    if (difficulty == 0) PIPE_GAP = SCREEN_HEIGHT * 0.4f;
    else if (difficulty == 1) PIPE_GAP = SCREEN_HEIGHT * 0.35f;
    else if (difficulty == 2) PIPE_GAP = SCREEN_HEIGHT * 0.3f;

    bird_x = SCREEN_WIDTH * 0.3f;
    bird_y = SCREEN_HEIGHT * 0.5f;
    bird_velocity = 0;

    for (int i = 0; i < N_PIPES; i++) {
        pipe_x[i] = SCREEN_WIDTH + i * (SCREEN_WIDTH / 2);
        pipe_gap_y[i] = SCREEN_HEIGHT * 0.2f + rand() % (int)(SCREEN_HEIGHT * 0.4f);
        if (pipe_gap_y[i] > MAX_PIPE_GAP_Y) pipe_gap_y[i] = MAX_PIPE_GAP_Y;
        if (pipe_gap_y[i] < MIN_PIPE_HEIGHT) pipe_gap_y[i] = MIN_PIPE_HEIGHT;
        int topPipeHeight = SCREEN_HEIGHT - (pipe_gap_y[i] + PIPE_GAP);
        if (topPipeHeight < MIN_PIPE_HEIGHT) topPipeHeight = MIN_PIPE_HEIGHT;
        printf("Pipe %d: lower height = %d, upper height = %d\n", i, pipe_gap_y[i], topPipeHeight);
        iResizeImage(&lowerPipeImages[i], PIPE_WIDTH, pipe_gap_y[i]);
        iResizeImage(&upperPipeImages[i], PIPE_WIDTH, topPipeHeight);
        scoreCountedPerPipe[i] = false;
    }

    for (int i = 0; i < N_COINS; i++) {
        coin_x[i] = SCREEN_WIDTH + i * (SCREEN_WIDTH / 2);
        coin_y[i] = SCREEN_HEIGHT * 0.3f + rand() % (int)(SCREEN_HEIGHT * 0.4f);
    }

    for (int i = 0; i < N_CLOUDS; i++) {
        cloud_x[i] = SCREEN_WIDTH + i * (SCREEN_WIDTH / 2);
        cloud_y[i] = SCREEN_HEIGHT * (0.6f + i * 0.05f);
    }

    score = 0;
    gameOver = false;
    iResumeTimer(physicsTimer);
}

void updateClouds() {
    if (gameState != 4 || gameOver) return;
    for (int i = 0; i < N_CLOUDS; i++) {
        cloud_x[i] -= SCREEN_WIDTH * 0.01f;
        if (cloud_x[i] + (SCREEN_WIDTH * 0.4f) < 0) {
            cloud_x[i] = SCREEN_WIDTH;
        }
    }
}

void updateCoins() {
    if (gameState != 4 || gameOver) return; 
    for (int i = 0; i < N_COINS; i++) {
        coin_x[i] -= COIN_SPEED;
        if (coin_x[i] + COIN_WIDTH < 0) {
            coin_x[i] = SCREEN_WIDTH;
            coin_y[i] = SCREEN_HEIGHT * 0.3f + rand() % (int)(SCREEN_HEIGHT * 0.4f);
        }
    }
}

void updateCoinAnimation() {
    for (int i = 0; i < N_COINS; i++) {
        coinFrameIndex[i] = (coinFrameIndex[i] + 1) % COIN_FRAMES;
    }
}

void updateBirdAnimation() {
    if (gameOver) return;
    flyingFrame = (flyingFrame + 1) % N_FRAMES;
}

void updateGame() {
    if (gameState != 4 || gameOver) return;

    bird_velocity -= GRAVITY;
    if (bird_velocity < -2 * JUMP_VELOCITY) bird_velocity = -2 * JUMP_VELOCITY;
    bird_y += bird_velocity;

    if (bird_y < 0) {
        bird_y = 0;
        if (!gameOver) {
            gameOver = true;
            showDeathFlash = true;
            deathFlashTimer = DEATH_FLASH_DURATION;
            iPauseTimer(physicsTimer);
            iPlaySound("hit.wav", false);
            iPlaySound("die.wav", false);
        }
    }

    if (bird_y + BIRD_HEIGHT > SCREEN_HEIGHT) {
        bird_y = SCREEN_HEIGHT - BIRD_HEIGHT;
    }

    for (int i = 0; i < N_PIPES; i++) {
        pipe_x[i] -= PIPE_SPEED;
        if (pipe_x[i] + PIPE_WIDTH < 0) {
            int farthestX = 0;
            for (int j = 0; j < N_PIPES; j++) {
                if (pipe_x[j] > farthestX) {
                    farthestX = pipe_x[j];
                }
            }
            int spacing = MIN_PIPE_SPACING + rand() % (int)EXTRA_PIPE_SPACING;
            pipe_x[i] = farthestX + spacing;
            pipe_gap_y[i] = SCREEN_HEIGHT * 0.2f + rand() % (int)(SCREEN_HEIGHT * 0.4f);
            if (pipe_gap_y[i] > MAX_PIPE_GAP_Y) pipe_gap_y[i] = MAX_PIPE_GAP_Y;
            if (pipe_gap_y[i] < MIN_PIPE_HEIGHT) pipe_gap_y[i] = MIN_PIPE_HEIGHT;
            int topPipeHeight = SCREEN_HEIGHT - (pipe_gap_y[i] + PIPE_GAP);
            if (topPipeHeight < MIN_PIPE_HEIGHT) topPipeHeight = MIN_PIPE_HEIGHT;
            printf("Pipe %d: lower height = %d, upper height = %d\n", i, pipe_gap_y[i], topPipeHeight);
            iResizeImage(&lowerPipeImages[i], PIPE_WIDTH, pipe_gap_y[i]);
            iResizeImage(&upperPipeImages[i], PIPE_WIDTH, topPipeHeight);
            scoreCountedPerPipe[i] = false;
        }

        if (bird_x + BIRD_WIDTH > pipe_x[i] && bird_x < pipe_x[i] + PIPE_WIDTH &&
            (bird_y < pipe_gap_y[i] || bird_y + BIRD_HEIGHT > pipe_gap_y[i] + PIPE_GAP)) {
            if (!gameOver) {
                gameOver = true;
                iPlaySound("hit.wav", false);
                iPlaySound("die.wav", false);
                showDeathFlash = true;
                deathFlashTimer = DEATH_FLASH_DURATION;
                iPauseTimer(physicsTimer);
            }
        }

        if (!scoreCountedPerPipe[i] && pipe_x[i] + PIPE_WIDTH < bird_x) {
            score++;
            scoreCountedPerPipe[i] = true;
        }
    }

    for (int i = 0; i < N_COINS; i++) {
        if (bird_x + BIRD_WIDTH > coin_x[i] && bird_x < coin_x[i] + COIN_WIDTH &&
            bird_y + BIRD_HEIGHT > coin_y[i] && bird_y < coin_y[i] + COIN_HEIGHT) {
            score += 5;
            iPlaySound("coin.wav", false);
            coin_x[i] = SCREEN_WIDTH;
            coin_y[i] = SCREEN_HEIGHT * 0.3f + rand() % (int)(SCREEN_HEIGHT * 0.4f);
        }
    }
}

void easyPage() {
    PIPE_GAP = SCREEN_HEIGHT * 0.4f;
    iClear();
    iShowLoadedImage(0, 0, &BG);
    if (difficulty == 0) {
        for (int i = 0; i < N_CLOUDS; i++) {
            iShowLoadedImage((int)cloud_x[i], (int)cloud_y[i], &cloudImages[i]);
        }
    }
    for (int i = 0; i < N_COINS; i++) {
        iShowLoadedImage((int)coin_x[i], (int)coin_y[i], &coinFrames[i][coinFrameIndex[i]]);
    }
    iShowLoadedImage((int)bird_x, (int)bird_y, &birdFrames[flyingFrame]);
    for (int i = 0; i < N_PIPES; i++) {
        iShowLoadedImage(pipe_x[i], 0, &lowerPipeImages[i]);
        iShowLoadedImage(pipe_x[i], pipe_gap_y[i] + PIPE_GAP, &upperPipeImages[i]);
    }
    iSetColor(255, 255, 255);
    char scoreText[20];
    sprintf(scoreText, "Score: %d", score);
    iText(SCREEN_WIDTH * 0.03f, SCREEN_HEIGHT * 0.92f, scoreText, GLUT_BITMAP_TIMES_ROMAN_24);
    if (gameOver) {
        iSetColor(255, 0, 0);
        iText(SCREEN_WIDTH * 0.5f - 60, SCREEN_HEIGHT * 0.5f, "GAME OVER", GLUT_BITMAP_TIMES_ROMAN_24);
        iText(SCREEN_WIDTH * 0.5f - 90, SCREEN_HEIGHT * 0.5f - 30, "Press 'R' to Enter Your Name", GLUT_BITMAP_TIMES_ROMAN_24);
    }
}

void hardPage() {
    PIPE_GAP = SCREEN_HEIGHT * 0.35f;
    iClear();
    iShowLoadedImage(0, 0, &BG1);
    for (int i = 0; i < N_COINS; i++) {
        iShowLoadedImage((int)coin_x[i], (int)coin_y[i], &coinFrames[i][coinFrameIndex[i]]);
    }
    iShowLoadedImage((int)bird_x, (int)bird_y, &birdFrames[flyingFrame]);
    for (int i = 0; i < N_PIPES; i++) {
        iShowLoadedImage(pipe_x[i], 0, &lowerPipeImages[i]);
        iShowLoadedImage(pipe_x[i], pipe_gap_y[i] + PIPE_GAP, &upperPipeImages[i]);
    }
    iSetColor(255, 255, 255);
    char scoreText[20];
    sprintf(scoreText, "Score: %d", score);
    iText(SCREEN_WIDTH * 0.03f, SCREEN_HEIGHT * 0.92f, scoreText, GLUT_BITMAP_TIMES_ROMAN_24);
    if (gameOver) {
        iSetColor(255, 0, 0);
        iText(SCREEN_WIDTH * 0.5f - 60, SCREEN_HEIGHT * 0.5f, "GAME OVER", GLUT_BITMAP_TIMES_ROMAN_24);
        iText(SCREEN_WIDTH * 0.5f - 90, SCREEN_HEIGHT * 0.5f - 30, "Press 'R' to Enter Your Name", GLUT_BITMAP_TIMES_ROMAN_24);
    }
}

void insanePage() {
    PIPE_GAP = SCREEN_HEIGHT * 0.3f;
    iClear();
    iShowLoadedImage(0, 0, &BG2);
    for (int i = 0; i < N_COINS; i++) {
        iShowLoadedImage((int)coin_x[i], (int)coin_y[i], &coinFrames[i][coinFrameIndex[i]]);
    }
    iShowLoadedImage((int)bird_x, (int)bird_y, &birdFrames[flyingFrame]);
    for (int i = 0; i < N_PIPES; i++) {
        iShowLoadedImage(pipe_x[i], 0, &lowerPipeImages[i]);
        iShowLoadedImage(pipe_x[i], pipe_gap_y[i] + PIPE_GAP, &upperPipeImages[i]);
    }
    iSetColor(255, 255, 255);
    char scoreText[20];
    sprintf(scoreText, "Score: %d", score);
    iText(SCREEN_WIDTH * 0.03f, SCREEN_HEIGHT * 0.92f, scoreText, GLUT_BITMAP_TIMES_ROMAN_24);
    if (gameOver) {
        iSetColor(255, 0, 0);
        iText(SCREEN_WIDTH * 0.5f - 60, SCREEN_HEIGHT * 0.5f, "GAME OVER", GLUT_BITMAP_TIMES_ROMAN_24);
        iText(SCREEN_WIDTH * 0.5f - 90, SCREEN_HEIGHT * 0.5f - 30, "Press 'R' to Enter Your Name", GLUT_BITMAP_TIMES_ROMAN_24);
    }
}

void iDraw() {
    iClear();
    if (gameState == 0) {
        startPage();
    } else if (gameState == 1) {
        homePage();
    } else if (gameState == 2) {
        levelPage();
    } else if (gameState == 3) {
        rulesPage();
    } else if (gameState == 5) {
        aboutPage();
    } else if (gameState == 6) {
        scorePage();
    }
    if (gameState == 4 && difficulty == 0) {
        easyPage();
    } else if (gameState == 4 && difficulty == 1) {
        hardPage();
    } else if (gameState == 4 && difficulty == 2) {
        insanePage();
    }
    if (enteringName) {
        iSetColor(255, 255, 255);
        iFilledRectangle(SCREEN_WIDTH * 0.5f - SCREEN_WIDTH * 0.3f, SCREEN_HEIGHT * 0.5f - SCREEN_HEIGHT * 0.06f, SCREEN_WIDTH * 0.6f, SCREEN_HEIGHT * 0.12f);
        iSetColor(0, 0, 0);
        iRectangle(SCREEN_WIDTH * 0.5f - SCREEN_WIDTH * 0.3f, SCREEN_HEIGHT * 0.5f - SCREEN_HEIGHT * 0.06f, SCREEN_WIDTH * 0.6f, SCREEN_HEIGHT * 0.12f);
        iText(SCREEN_WIDTH * 0.5f - SCREEN_WIDTH * 0.29f, SCREEN_HEIGHT * 0.5f + SCREEN_HEIGHT * 0.08f, "Enter your name:", GLUT_BITMAP_HELVETICA_18);
        int textLength = strlen(currentPlayerName);
        int textPixelWidth = 9 * textLength;
        int nameX = SCREEN_WIDTH * 0.5f - textPixelWidth / 2;
        int nameY = SCREEN_HEIGHT * 0.5f;
        iText(nameX, nameY, currentPlayerName, GLUT_BITMAP_HELVETICA_18);
        if (showCursor && nameCharIndex < NAME_LENGTH - 1) {
            iLine(nameX + textPixelWidth + 2, nameY, nameX + textPixelWidth + 2, nameY + 18);
        }
    }
    if (!gameOver) {
        if (difficulty == 0) iWrapImage(&BG, -20);
        if (difficulty == 1) iWrapImage(&BG1, -10);
        if (difficulty == 2) iWrapImage(&BG2, -20);
    }
    if (showDeathFlash) {
        iSetColor(255, 255, 255);
        iFilledRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
        deathFlashTimer--;
        if (deathFlashTimer <= 0) {
            showDeathFlash = false;
        }
    }
}

void iMouseMove(int mx, int my) {
    printf("x = %d, y= %d\n", mx, my);
}

void iMouse(int button, int state, int mx, int my) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        iPlaySound("click.wav", false);
        if (gameState == 1) {
            if (mx >= SCREEN_WIDTH * 0.42f && mx <= SCREEN_WIDTH * 0.72f && my >= SCREEN_HEIGHT * 0.74f && my <= SCREEN_HEIGHT * 0.83f) {
                gameState = 2;
            } else if (mx >= SCREEN_WIDTH * 0.42f && mx <= SCREEN_WIDTH * 0.72f && my >= SCREEN_HEIGHT * 0.61f && my <= SCREEN_HEIGHT * 0.70f) {
                gameState = 6;
            } else if (mx >= SCREEN_WIDTH * 0.42f && mx <= SCREEN_WIDTH * 0.72f && my >= SCREEN_HEIGHT * 0.36f && my <= SCREEN_HEIGHT * 0.45f) {
                gameState = 5;
            } else if (mx >= SCREEN_WIDTH * 0.42f && mx <= SCREEN_WIDTH * 0.72f && my >= SCREEN_HEIGHT * 0.23f && my <= SCREEN_HEIGHT * 0.32f) {
                gameState = 3;
            } else if (mx >= SCREEN_WIDTH * 0.42f && mx <= SCREEN_WIDTH * 0.72f && my >= SCREEN_HEIGHT * 0.11f && my <= SCREEN_HEIGHT * 0.20f) {
                exit(0);
            }
        } else if (gameState == 2) {
            if (mx >= SCREEN_WIDTH * 0.43f && mx <= SCREEN_WIDTH * 0.71f && my >= SCREEN_HEIGHT * 0.57f && my <= SCREEN_HEIGHT * 0.73f) {
                difficulty = 0;
                gameState = 4;
                resetGame();
            } else if (mx >= SCREEN_WIDTH * 0.43f && mx <= SCREEN_WIDTH * 0.71f && my >= SCREEN_HEIGHT * 0.37f && my <= SCREEN_HEIGHT * 0.52f) {
                difficulty = 1;
                gameState = 4;
                resetGame();
            } else if (mx >= SCREEN_WIDTH * 0.43f && mx <= SCREEN_WIDTH * 0.71f && my >= SCREEN_HEIGHT * 0.16f && my <= SCREEN_HEIGHT * 0.31f) {
                difficulty = 2;
                gameState = 4;
                resetGame();
            }
        }
    }
}

void iKeyboard(unsigned char key) {
    if (enteringName) {
        if (key == '\r' || key == '\n') {
            currentPlayerName[nameCharIndex] = '\0';
            insertHighScore(currentPlayerName[0] ? currentPlayerName : "Unknown", score);
            enteringName = false;
            score = 0;
            gameOver = false;
            iPauseTimer(cursorTimer);
            gameState = 1; 
            resetGame(); 
            return;
        } else if (key == 8 && nameCharIndex > 0) {
            nameCharIndex--;
            currentPlayerName[nameCharIndex] = '\0';
        } else if (nameCharIndex < NAME_LENGTH - 1 && key >= 32 && key <= 126) {
            currentPlayerName[nameCharIndex++] = key;
            currentPlayerName[nameCharIndex] = '\0';
        }
        return; 
    }

    if (key == 27 || key == 13) {
        gameState = 1;
    }
    if (key == ' ' && gameState == 4) {
        bird_velocity = JUMP_VELOCITY;
        iPlaySound("space.wav", false);
    }
    if ((key == 'r' || key == 'R') && !enteringName) {
        resetGame();
    }
}

void iSpecialKeyboard(unsigned char key) {
    if (key == GLUT_KEY_END) {
        exit(0);
    }
}

void iMouseDrag(int mx, int my) {}
void iMouseWheel(int dir, int mx, int my) {}

int main(int argc, char *argv[]) {
    srand((unsigned)time(NULL));
    glutInit(&argc, argv);
    iInitializeSound();

    
    int bgChannel = iPlaySound("bg.wav", true, 100);
    if (bgChannel == -1) {
        printf("Failed to play bg.wav\n");
    }

    if (!iLoadImage(&startstr, "start-page.png")) {
        printf("Failed to load start-page.png\n");
        exit(1);
    }
    iResizeImage(&startstr, SCREEN_WIDTH, SCREEN_HEIGHT);

    if (!iLoadImage(&homestr, "home-page.png")) {
        printf("Failed to load home-page.png\n");
        exit(1);
    }
    iResizeImage(&homestr, SCREEN_WIDTH, SCREEN_HEIGHT);

    if (!iLoadImage(&rulesstr, "rules.png")) {
        printf("Failed to load rules.png\n");
        exit(1);
    }
    iResizeImage(&rulesstr, SCREEN_WIDTH, SCREEN_HEIGHT);

    if (!iLoadImage(&aboutstr, "about-us.png")) {
        printf("Failed to load about-us.png\n");
        exit(1);
    }
    iResizeImage(&aboutstr, SCREEN_WIDTH, SCREEN_HEIGHT);

    if (!iLoadImage(&levelstr, "level.png")) {
        printf("Failed to load level.png\n");
        exit(1);
    }
    iResizeImage(&levelstr, SCREEN_WIDTH, SCREEN_HEIGHT);

    if (!iLoadImage(&scorestr, "high-score.png")) {
        printf("Failed to load high-score.png\n");
        exit(1);
    }
    iResizeImage(&scorestr, SCREEN_WIDTH, SCREEN_HEIGHT);

    if (!iLoadImage(&BG, "flappybg.png")) {
        printf("Failed to load flappybg.png\n");
        exit(1);
    }
    iResizeImage(&BG, SCREEN_WIDTH, SCREEN_HEIGHT);

    if (!iLoadImage(&BG1, "hardbg.png")) {
        printf("Failed to load hardbg.png\n");
        exit(1);
    }
    iResizeImage(&BG1, SCREEN_WIDTH, SCREEN_HEIGHT);

    if (!iLoadImage(&BG2, "insanebg.png")) {
        printf("Failed to load insanebg.png\n");
        exit(1);
    }
    iResizeImage(&BG2, SCREEN_WIDTH, SCREEN_HEIGHT);

    for (int i = 0; i < N_FRAMES; i++) {
        char filename[50];
        sprintf(filename, "tile00%d.png", i);
        if (!iLoadImage(&birdFrames[i], filename)) {
            printf("Failed to load %s\n", filename);
            exit(1);
        }
        iResizeImage(&birdFrames[i], BIRD_WIDTH, BIRD_HEIGHT);
    }

    for (int i = 0; i < N_COINS; i++) {
        for (int f = 0; f < COIN_FRAMES; f++) {
            char filename[50];
            sprintf(filename, "coin (%d).png", f + 1);
            if (!iLoadImage(&coinFrames[i][f], filename)) {
                printf("Failed to load %s\n");
                exit(1);
            }
            iResizeImage(&coinFrames[i][f], COIN_WIDTH, COIN_HEIGHT);
        }
    }

    for (int i = 0; i < N_CLOUDS; i++) {
        if (!iLoadImage(&cloudImages[i], "cloud.png")) {
            printf("Failed to load cloud.png\n");
            exit(1);
        }
        iResizeImage(&cloudImages[i], SCREEN_WIDTH * 0.4f, SCREEN_HEIGHT * 0.3f);
    }

    for (int i = 0; i < N_PIPES; i++) {
        if (!iLoadImage(&lowerPipeImages[i], "lowerpipe.png")) {
            printf("Failed to load lowerpipe.png\n");
            exit(1);
        }
        if (!iLoadImage(&upperPipeImages[i], "upperpipe.png")) {
            printf("Failed to load upperpipe.png\n");
            exit(1);
        }
    }

    loadHighScores();
    resetGame();

    iSetTimer(10, updateClouds);
    animTimer = iSetTimer(100, updateBirdAnimation);
    physicsTimer = iSetTimer(15, updateGame);
    iSetTimer(10, updateCoins);
    coinAnimTimer = iSetTimer(100, updateCoinAnimation);
    cursorTimer = iSetTimer(500, []() { showCursor = !showCursor; });

    iInitialize(SCREEN_WIDTH, SCREEN_HEIGHT, "Flappy Bird - Clean Version");
    return 0;
}
