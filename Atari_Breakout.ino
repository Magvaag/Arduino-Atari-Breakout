#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

// Joystick
const int JOY_SW_pin = 2; // digital
const int JOY_X_pin = 0; // analog
const int JOY_Y_pin = 1; // analog

// Paddle
const float paddleSpeed = .004;
const int paddleWidth = 7;
const int paddleHeight = 2;
const int paddlePaddingY = 10;
float paddleX;
bool pastPaddle; // If the ball has gone past the paddle already

// Ball
const int ballSize = 2;
float ballX, ballY;
float ballVelX, ballVelY;

const int roofPadding = 8;
const int boxWidth = 2;
const int boxHeight = 2;
const int boxAmountWidth = 11, boxAmountHeight = 7;

int score;
int highScore;

bool dead;

// Menus
const int WINDOW_MAIN_MENU = 0;
const int WINDOW_GAME = 1;

int windowState = WINDOW_MAIN_MENU;

class MainMenu {
  public:
    MainMenu() {}

    static const int optionsCount = 3;
    char* options [optionsCount] = {"PLAY", "HSCR", "OPTN"};
    int selected = 0;

    void updateMainMenu() {
      updateLogics();
      drawMainMenu();
    }

    void updateLogics() {
      int joyY = analogRead(JOY_Y_pin) - 512;
      if (abs(joyY) > 450) { // Threshold of 450 of 512
        if (joyY > 0) {
          changeSelected(1);
        } else {
          changeSelected(-1);
        }
        delay(200);
      }

      // Select option
      if (!digitalRead(JOY_SW_pin) && selected == 0) {
        windowState = WINDOW_GAME;
      }
    }

    void changeSelected(int delta) {
      selected += delta;
      if (selected < 0) selected += optionsCount;
      if (selected >= optionsCount) selected -= optionsCount;
    }

    void drawMainMenu() {
      display.setRotation(3);
      drawOptions();
      display.setRotation(0);
    }

    void drawOptions() {
      int rectWidth = 27, rectHeight = 12;
      int rectPaddingX = (display.width() - rectWidth) / 2;
      int rectSpacingY = 3;
      int rectStartingY = (display.height() - (3 * rectHeight + 2 * rectSpacingY)) / 2;

      for (int i = 0; i < 3; i++) {
        char* text = options[i];
      
        display.drawRect(rectPaddingX, rectStartingY + (rectHeight + rectSpacingY) * i, rectWidth, rectHeight, WHITE);
        if (i == selected) display.drawRect(rectPaddingX-1, rectStartingY + (rectHeight + rectSpacingY) * i-1, rectWidth+2, rectHeight+2, WHITE);
  
        int textLen = 5 * 4 + 1 * (4-1); // 1 pixel between the four letters
  
        display.setCursor(rectPaddingX+2, rectStartingY+2 + (rectHeight + rectSpacingY) * i);
        display.println(text);
      }
    }
};

class Box {
  public:
    Box() {}
    Box(int x, int y) {
      this->x = x;
      this->y = y;
      visible = true;
    }

    bool hitBall() {
      if (!visible) return;
      if (ballX + ballSize >= x && ballX < x + boxWidth && ballY + ballSize >= y && ballY < y + boxHeight) {
        // Figure out how much "inside" the box the ball is, so we can direct it in the right direction
        float ballDepthXFromLeft = ((ballX + ballSize) - x);
        float ballDepthXFromRight = ((x + boxWidth) - ballX);
        float ballDepthYFromBottom = ((y + boxHeight) - ballY);
        float ballDepthYFromTop = ((ballY + ballSize) - y);

        if (ballVelX >= 0)
          if (ballVelY >= 0)
            if (ballDepthXFromLeft >= ballDepthYFromTop)
              ballVelY = -ballVelY;
            else
              ballVelX = -ballVelX;
          else
            if (ballDepthXFromLeft >= ballDepthYFromBottom)
              ballVelY = -ballVelY;
            else
              ballVelX = -ballVelX;
        else
          if (ballVelY >= 0)
            if (ballDepthXFromRight >= ballDepthYFromTop)
              ballVelY = -ballVelY;
            else
              ballVelX = -ballVelX;
          else
            if (ballDepthXFromRight >= ballDepthYFromBottom)
              ballVelY = -ballVelY;
            else
              ballVelX = -ballVelX;
        
        visible = false;
        score++;
      }
    }
    
    int x, y;
    bool visible;
};
Box boxes [boxAmountWidth * boxAmountHeight];
MainMenu mainMenu = MainMenu();

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

  // Setup the joystick
  pinMode(JOY_SW_pin, INPUT);
  digitalWrite(JOY_SW_pin, HIGH);

  // initialize with the I2C addr 0x3C (for the 128x32)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  // Init objects
  for (int i = 0; i < boxAmountWidth * boxAmountHeight; i++) {
    int x = i % boxAmountWidth;
    int y = i / boxAmountWidth;
    
    Box box = Box(x * 3, roofPadding + 2 + y * 3);
    boxes[i] = box;
  }
  
  highScore = 0;
  ballSpawn();
}

// 32 = 2 * n + 1 * (n-1) = 3n - 1
// 30 = 3 * n + 2 * (n-1) = 5n - 2

void ballSpawn() {
  pastPaddle = false;
  ballX = display.height() / 2.0f - ballSize / 2.0f;
  ballY = display.width() / 3.0f - ballSize / 2.0f;
  ballVelX = .5f * ((rand () % 100) / 50.0f - 1.0f);
  ballVelY = 2.0f;
  paddleX = display.height() / 2.0f - paddleWidth / 2.0f;
  if (score > highScore) highScore = score;
  score = 0;
  showAllBoxes();
  dead = false;
}

void showAllBoxes() {
  for (int i = 0; i < boxAmountWidth * boxAmountHeight; i++) {
    boxes[i].visible = true;
  }
}

void loop() {
  // Clear the display and make it ready for more rendering
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  // Call the appropriate update method
  switch (windowState) {
    case WINDOW_MAIN_MENU: 
      mainMenu.updateMainMenu();
      break;
    case WINDOW_GAME: 
      updateGame();
      break;
  }

  // Render to the display
  display.display();
  delay(10);
}

//void updateMainMenu() {
  //mainMenu->updateMainMenu();
//}

void updateGame() {
  if (!dead) {
    // Update physics
    movePaddle();
    moveBall();
    updateBoxes();
  } else if (!digitalRead(JOY_SW_pin)) {
    ballSpawn();
  }

  // Draw on the display
  drawPaddle();
  drawBall();
  drawScore();
  drawBoxes();
  drawRoof();
}

void updateBoxes() {
  int visible = 0;
  for (int i = 0; i < boxAmountWidth * boxAmountHeight; i++) {
    if (boxes[i].visible) visible++;
    boxes[i].hitBall();
  }

  if (visible == 0) {
    showAllBoxes(); // Delay the show (?)
  }
}

void die() {
  if (score > highScore) highScore = score;
  dead = true;
}

void moveBall() {
  float maxVel = 1.0f;
  if (ballVelX > maxVel) ballVelX = maxVel;
  if (ballVelX < -maxVel) ballVelX = -maxVel;
  
  ballX += ballVelX;
  ballY += ballVelY;

  if (ballX < 0) {
    ballX = 0;
    ballVelX = -ballVelX;
  }

  if (ballX > display.height() - 1) {
    ballX = display.height() - 1;
    ballVelX = -ballVelX;
  }

  if (ballY < roofPadding) {
    ballY = roofPadding;
    ballVelY = -ballVelY;
  }

  if (ballY > display.width()) {
    //ballY = display.width();
    //ballVelY = -ballVelY;
    die();
  }

  if (ballY + ballSize > display.width() - paddlePaddingY && !pastPaddle && ballVelY > 0) {
    if ((ballX + ballSize >= paddleX && ballX < paddleX + paddleWidth)) {
      //ballY = display.width() - paddlePaddingY;
      float posOnPaddle = ((ballX + ballSize / 2.0f) - (paddleX + paddleWidth / 2.0f)) / paddleWidth;
      // posOnPaddle = 1 - pow(1 - (posOnPaddle), 2);
      
      float paddleXSpeedModifier = 2.0f;
      ballVelX = posOnPaddle * paddleXSpeedModifier + ballVelX * abs(posOnPaddle);
      ballVelY = -ballVelY;
    } else {
      pastPaddle = true;
    }
  }
}

void movePaddle() {
  // TODO: Convert to float between 0 and 1, then revert it, then pow it, and then if it
  int deltaX = analogRead(JOY_X_pin) - 1023 / 2.0f;
  paddleX += deltaX * paddleSpeed;

  if (paddleX < 0) {
    paddleX = 0;
  }

  if (paddleX + paddleWidth > display.height()) {
    paddleX = display.height() - paddleWidth;
  }
}

void drawBall() {
  display.fillRect(ballY, ballX, ballSize, ballSize, WHITE);
}

void drawPaddle() {
  display.fillRect(display.width() - paddlePaddingY, paddleX, paddleHeight, paddleWidth, WHITE);
}

void drawBoxes() {
  for (int i = 0; i < boxAmountWidth * boxAmountHeight; i++) {
    drawBox(boxes[i]);
  }
}

void drawBox(Box box) {
  if (!box.visible) return;
  display.fillRect(box.y, box.x, boxHeight, boxWidth, WHITE);
}

void drawRoof() {
  display.drawLine(roofPadding, 0, roofPadding, display.height() - 1, WHITE);
}

void drawScore() {
  display.setRotation(3);

  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println(score);

  // Highscore is right aligned
  int textWidth = 6;
  int highScoreLength = (highScore == 0) ? 1 : (log10(highScore)+1);
  display.setCursor((display.width()) - highScoreLength * textWidth, 0); // The display is still rotated, so we use the width instead
  display.println(highScore);

  display.setRotation(0);
}
