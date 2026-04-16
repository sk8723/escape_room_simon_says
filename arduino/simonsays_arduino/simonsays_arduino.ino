#include "Arduino.h"
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"
#include <Servo.h>

/////////////////////////
// setup
/////////////////////////
// button pins
#define BUTTON_RED      A0 // button associated with red LED
#define BUTTON_YELLOW   A1 // button associated with yellow LED
#define BUTTON_GREEN    A2 // button associated with green LED
#define BUTTON_BLUE     A3 // button associated with blue LED

// LED pins
#define LED_RED         12 // red LED
#define LED_YELLOW      9  // yellow LED
#define LED_GREEN       11 // green LED
#define LED_BLUE        10 // blue LED

// speaker
#define PIN_MP3_TX      2 // Connects to module's RX
#define PIN_MP3_RX      3 // Connects to module's TX

// Create Servo object
#define SERVO_PIN       6 // Servo connected to pin 9

/////////////////////////
// constants
/////////////////////////
// hardware
#define BUTTON_THRESHOLD 2.5

#define SERVO_OPEN 15
#define SERVO_CLOSED 147
#define SERVO_DELAY 20

#define SPEAKER_VOLUME 15

// gameplay
#define SUPER_ROUNDS 3
#define SUB_ROUNDS 4
#define WIN_THRESHOLD (SUPER_ROUNDS*SUB_ROUNDS)
#define MAX_PATTERN_LENGTH (WIN_THRESHOLD)

// times
#define DOUBLE_BUTTON_HOLD_TIME (3UL*1000UL)  // 3 seconds
#define AUTORESET_TIME (3UL*60UL*1000UL)  // 3 minutes
#define HINT_TIME (7UL*1000UL)  // 7 seconds

#define TRANSITION_DELAY 1000 // 1 second

#define FLASH_DELAY 1000 // 1 second

#define AUDIO_COLOR_DELAY 1500 // 1.5 seconds
#define AUDIO_ANIMAL_DELAY 2000 // 2 seconds
#define AUDIO_NOTE_DELAY 1200 // 1.5 seconds
#define AUDIO_END_ROUND_DELAY 7000 // 7 seconds
#define AUDIO_INCORRECT_DELAY 3200 // 3.2 seconds
#define AUDIO_RESET_DELAY 4500 // 4.5 seconds

/////////////////////////
// objects
/////////////////////////
// servo
Servo myServo;

// speaker
SoftwareSerial softwareSerial(PIN_MP3_RX, PIN_MP3_TX);
// create the Player object
DFRobotDFPlayerMini player;

/////////////////////////
// hardware definitions
/////////////////////////
enum audio {
  AUDIO_RED_ANIMAL = 1,     // animals (align with color enum)
  AUDIO_YELLOW_ANIMAL,
  AUDIO_GREEN_ANIMAL,
  AUDIO_BLUE_ANIMAL,
  AUDIO_RED_NOTE,           // notes
  AUDIO_YELLOW_NOTE,
  AUDIO_GREEN_NOTE,
  AUDIO_BLUE_NOTE,
  AUDIO_START,        // rounds
  AUDIO_END_ROUND1,
  AUDIO_END_ROUND2,
  AUDIO_END_ROUND3,
  AUDIO_INCORRECT1,   // incorrect
  AUDIO_INCORRECT2,
  AUDIO_INCORRECT3,
  AUDIO_INCORRECT4,
  AUDIO_INCORRECT5,
  AUDIO_MANUAL_RESET, // reset
  AUDIO_EASTER_EGG    // easter egg
};
#define AUDIO_NOTES_OFFSET (AUDIO_RED_NOTE)
#define AUDIO_ROUNDS_OFFSET (AUDIO_END_ROUND1)
#define AUDIO_INCORRECT_OFFSET (AUDIO_INCORRECT1)


/////////////////////////
// FSM state definitions
/////////////////////////
enum MainState {
  INIT,
  START,
  GAME,
  SUCCESS
};

enum GameState {
  GAME_START,
  GAME_PATTERN,
  GAME_USERINPUT,
  GAME_WAITRELEASE,
  GAME_CORRECT,
  GAME_INCORRECT,
  GAME_ROUNDCOMPLETE  
};

/////////////////////////
// global flags
/////////////////////////
// perception
bool RedPushed = false;
bool YellowPushed = false;
bool GreenPushed = false;
bool BluePushed = false;
bool StartPushed = false;
bool WaitRelease = false;
bool DoubleButtonWaitRelease = false;
bool ButtonMemory[4] = {RedPushed, YellowPushed, GreenPushed, BluePushed};

// gameplay
bool SuccessFlag = false;
bool GameResetFlag = false;

// timers
bool StartWaitFlag = false;
unsigned long StartTimer = 0;
bool DoubleButtonWaitFlag = false;
unsigned long DoubleButtonTimer = 0;
bool AutoResetWaitFlag = false;
unsigned long AutoResetTimer = 0;
bool HintWaitFlag = false;
unsigned long HintResetTimer = 0;

/////////////////////////
// game logic
/////////////////////////
// pattern input definitions
enum color {
  RED = 1,
  YELLOW,
  GREEN,
  BLUE
};

// round type definitions
enum round_type {
  LIGHTS,
  ANIMALS,
  NOTES
};

// game difficulty level definitions
enum difficulty {
  EASY,
  MEDIUM,
  HARD
};

// internal
int pattern[MAX_PATTERN_LENGTH];
int pattern_config[MAX_PATTERN_LENGTH];
int pattern_length = 0;
int userPos = 0;
int userIn = 0;

int round_count = 1;

int difficulty_level = HARD;

/////////////////////////
// Easter egg
/////////////////////////
#define EASTER_EGG_PATTERN_LENGTH 10
int easter_egg_pattern[EASTER_EGG_PATTERN_LENGTH] = {RED, RED, YELLOW, YELLOW, GREEN, BLUE, GREEN, BLUE, RED, BLUE}; // Konami code
int easter_egg_index = 0;
bool easter_egg_wait_release = false;

void setup() {
  // set up serial connection at 9600 Baud
  Serial.begin(9600);
 
  ///// LEDs and buttons
  // set up LED pins
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
 
  // set up button pins
  pinMode(BUTTON_RED, INPUT_PULLUP);
  pinMode(BUTTON_YELLOW, INPUT_PULLUP);
  pinMode(BUTTON_GREEN, INPUT_PULLUP);
  pinMode(BUTTON_BLUE, INPUT_PULLUP);

  ///// servo
  // Attach servo
  myServo.attach(SERVO_PIN);
  setChestPos(0.0); // close chest

  ///// speaker
  // init serial port for DFPlayer Mini
  softwareSerial.begin(9600);
  delay(2000);

  // start communication with DFPlayer Mini
  player.begin(softwareSerial);
  player.volume(SPEAKER_VOLUME); // set volume (0 to 30)

  // test hardware
  bool test_run = false;
  if (test_run) {
    testDisplay();
  }
}

void loop() {
  Perception();
  Planning();
  Action();
}

void Perception() {
  // check button presses
  RedPushed = isButtonPushed(BUTTON_RED);
  YellowPushed = isButtonPushed(BUTTON_YELLOW);
  GreenPushed = isButtonPushed(BUTTON_GREEN);
  BluePushed = isButtonPushed(BUTTON_BLUE);

  StartPushed = (RedPushed && BluePushed); // push red and blue to start
}

////////////////////////////////////////////////////////////////////
// planning
////////////////////////////////////////////////////////////////////
void Planning() {
  fsmMainProcess();
}

void fsmMainProcess() {
  static int state = INIT;

  switch (state) {
    case INIT:
      // reset everything
      systemReset();

      state = START;
      break;
    
    case START:
      // two buttons pressed leads to different outcomes
      if (doubleButtonPushed()) { // start button (red and blue together) must be held down to start game
        if (timer(DoubleButtonWaitFlag, DoubleButtonTimer, DOUBLE_BUTTON_HOLD_TIME)) {
          // record button push memory
          // use -1 to convert color to index
          ButtonMemory[RED-1] = RedPushed;
          ButtonMemory[YELLOW-1] = YellowPushed;
          ButtonMemory[GREEN-1] = GreenPushed;
          ButtonMemory[BLUE-1] = BluePushed;
          
          // raise release flag
          DoubleButtonWaitRelease = true;
        }
      } else if ((getCurrentButton() == -1) && DoubleButtonWaitRelease) { // wait to release both buttons
        // check specific buttons
        // easy mode
        if (ButtonMemory[RED-1] && ButtonMemory[YELLOW-1]) {
          difficulty_level = EASY;
        }
        // hard mode
        if (ButtonMemory[RED-1] && ButtonMemory[BLUE-1]) {
          difficulty_level = HARD;
        }
        // move to GAME no matter what
        state = GAME;
        // lower release flag
        DoubleButtonWaitRelease = false;
      } else { // meaning player stopped holding buttons
        DoubleButtonWaitFlag = false;
      }

      if (getCurrentButton() != -1 && !easter_egg_wait_release) { // button is being pressed
        if (getCurrentButton() == easter_egg_pattern[easter_egg_index]) { // next step in Easter egg sequence
          easter_egg_index += 1;
          if (easter_egg_index == EASTER_EGG_PATTERN_LENGTH) { // end of Easter egg sequence
            player.play(AUDIO_EASTER_EGG);
            easter_egg_index = 0; // reset Easter egg index to prevent multiple sounds playing
          }
        } else { // incorrect Easter egg input
          easter_egg_index = 0; // reset Easter egg index
        }
        easter_egg_wait_release = true;
      }
      if (getCurrentButton() == -1) {
        easter_egg_wait_release = false;
      }
      break;
    
    case GAME:
      // FSM for gameplay
      fsmGame();

      // success
      if (SuccessFlag) {
        state = SUCCESS;
      }

      // manual reset
      if (StartPushed) {
        if (timer(StartWaitFlag, StartTimer, DOUBLE_BUTTON_HOLD_TIME)) { // start button (red and blue together) must be held down to reset game
          WaitRelease = true;
        }
      } else if ((!RedPushed && !BluePushed) && WaitRelease) {
        systemReset();
        state = START;
        player.play(AUDIO_MANUAL_RESET);
        delay(AUDIO_RESET_DELAY);
        WaitRelease = false;
      } else { // meaning player stopped holding buttons
        StartWaitFlag = false;
      }

      // automatic reset
      if (getCurrentButton() == -1) { // meaning no button is being touched
        if (timer(AutoResetWaitFlag, AutoResetTimer, AUTORESET_TIME)) { // game will reset automatically if not touched
          systemReset();
          state = START;
        }
      } else { // meaning player pressed something
        AutoResetWaitFlag = false;
      }
      break;
    
    case SUCCESS:
      // success displays
      successDisplay();

      // automatic reset (nothing can stop this)
      if (timer(AutoResetWaitFlag, AutoResetTimer, AUTORESET_TIME)) { // game will reset automatically if not touched
        systemReset();
        state = START;
        AutoResetWaitFlag = false;
      }
      break;
  }
}

void fsmGame() {
  static int state = GAME_START;

  // check for reset
  if (GameResetFlag) {
    state = GAME_START;
    GameResetFlag = false; // lower flag
  }

  switch (state) {
    case GAME_START:
      // start display
      startDisplay();

      // set pattern (and config) sequence to 0s
      resetPattern();

      state = GAME_PATTERN;
      delay(TRANSITION_DELAY);
      break;
    
    case GAME_PATTERN:
      // update pattern
      if (pattern_length < round_count) {
        pattern[pattern_length] = random(RED, BLUE+1); // red (1) - blue (4)
        pattern_length += 1;
      }

      // reset player position in pattern
      userPos = 0;

      // round type logic
      for (int i = 0; i < pattern_length; i++) {
        pattern_config[i] = (int)((round_count-1)/SUB_ROUNDS); // lights > animals > notes
      }
      // show pattern
      patternDisplay();

      state = GAME_USERINPUT;
      break;
    
    case GAME_USERINPUT:
      // wait for user to press button
      userIn = getCurrentButton(); // record user input
      if (userIn != -1) { // wait for user to press a button
        state = GAME_WAITRELEASE;
        HintWaitFlag = false; // lower hint wait flag when user does something
      } else {
        // if user takes a long time to respond, give hint
        if (timer(HintWaitFlag, HintResetTimer, HINT_TIME)) {
          hintDisplay();
        }
      }
      break;

    case GAME_WAITRELEASE:
      // takes user input
      if (getCurrentButton() == -1) { // wait for release
        if (userIn == pattern[userPos]) { // check if user selected correct button
          state = GAME_CORRECT;
        } else {
          state = GAME_INCORRECT;
        }
      }
      break;
    
    case GAME_CORRECT:
      userPos += 1; // increase player index
      if (userPos == pattern_length) { // check if player has reached end of current sequence
        state = GAME_ROUNDCOMPLETE;
      } else {
        state = GAME_USERINPUT; 
      }
      break;

    case GAME_INCORRECT:
      // incorrect input display
      incorrectDisplay();

      state = GAME_PATTERN;
      delay(TRANSITION_DELAY);
      break;

    case GAME_ROUNDCOMPLETE:
      // round complete display
      if ((round_count%SUB_ROUNDS) == 0) { // only play at the end of a super round
        roundcompleteDisplay();
      }

      // round succession logic
      if (round_count == WIN_THRESHOLD) { // check if current round is final
        SuccessFlag = true;
      } else {
        round_count += 1; // increase round count
      }
      
      state = GAME_PATTERN;
      delay(TRANSITION_DELAY);
      delay(TRANSITION_DELAY); // extra padding
      break;
  }
}

void Action() {
  // button press tracker
  static int last_button = -1;
  int current_button = getCurrentButton();

  // button lights
  setLedStatus(getLedPin(RED), RedPushed);
  setLedStatus(getLedPin(YELLOW), YellowPushed);
  setLedStatus(getLedPin(GREEN), GreenPushed);
  setLedStatus(getLedPin(BLUE), BluePushed);

  // button sounds
  int current_round_type = (int)((round_count-1)/SUB_ROUNDS); // match round to super round
  switch(difficulty_level) {
    case EASY:
      // show lights and play notes for fun
      if (current_button != -1 && current_button != last_button) { // only play sound once (wait for button to release or new button)
        player.play((current_button-1) + AUDIO_NOTES_OFFSET);   
      }
      break;
    case HARD:
      if (current_button != -1 && current_button != last_button) { // only play sound once (wait for button to release or new button)
        switch (current_round_type) {
          case LIGHTS:
            // always show
            break;
          case ANIMALS:
            player.play(current_button);
            break;
          case NOTES:
            player.play((current_button-1) + AUDIO_NOTES_OFFSET);
            break;
        }
      }
      break;
  }

  // update button press tracker
  last_button = current_button;
}

////////////////////////////////////////////////////////////////////
// display functions
////////////////////////////////////////////////////////////////////

void testDisplay() {
  // LEDs
    int test_config[4] = {1,1,1,1};
    LEDconfigDisplay(test_config, AUDIO_COLOR_DELAY);

    // servo
    setChestPos(1.0); // open chest
    setChestPos(0.0); // close chest

    // speaker
    for (int i = AUDIO_RED_ANIMAL; i < AUDIO_MANUAL_RESET + 1; i++) {
      player.play(i);
      delay(AUDIO_COLOR_DELAY);
    }
}

void LEDconfigDisplay(int config[4], int duration) {
  setLedStatus(getLedPin(RED), (config[0] == 1));
  setLedStatus(getLedPin(YELLOW), (config[1] == 1));
  setLedStatus(getLedPin(GREEN), (config[2] == 1));
  setLedStatus(getLedPin(BLUE), (config[3] == 1));
  delay(duration);
  setLedStatus(getLedPin(RED), false);
  setLedStatus(getLedPin(YELLOW), false);
  setLedStatus(getLedPin(GREEN), false);
  setLedStatus(getLedPin(BLUE), false);
}

void startDisplay() {
  // play start audio
  player.play(AUDIO_START);

  // flash lights
  int display_config[4] = {1,1,1,1};
  LEDconfigDisplay(display_config, FLASH_DELAY);

  delay(AUDIO_END_ROUND_DELAY);
}

void patternDisplay() {
  int current_round_type = (int)((round_count-1)/SUB_ROUNDS); // match round to super round
  for (int i = 0; i < pattern_length; i++) {
    switch(difficulty_level) {
      case EASY:
        setLedStatus(getLedPin(pattern[i]), true); // turn on
        player.play((pattern[i]-1) + AUDIO_NOTES_OFFSET); // play notes for fun
        delay(FLASH_DELAY/(current_round_type + 2)); // ranges 2-4, speeds up over time
        setLedStatus(getLedPin(pattern[i]), false); // turn off
        delay(FLASH_DELAY/(current_round_type + 2));
        break;
      case HARD:
        switch (pattern_config[i]) {
          case LIGHTS:
            setLedStatus(getLedPin(pattern[i]), true); // turn on
            delay(FLASH_DELAY/2);
            setLedStatus(getLedPin(pattern[i]), false); // turn off
            delay(FLASH_DELAY/2);
            break;
          case ANIMALS:
            player.play(pattern[i]);
            delay(AUDIO_ANIMAL_DELAY);
            break;
          case NOTES:
            player.play((pattern[i]-1) + AUDIO_NOTES_OFFSET);
            delay(AUDIO_NOTE_DELAY);
            break;
        }
        break;
    }
  }
}

void hintDisplay() {
  int user_color = pattern[userPos];
  int display_config[4] = {0,0,0,0};
  display_config[user_color-1] = 1; // set the current color in the pattern to 1
  LEDconfigDisplay(display_config, FLASH_DELAY);
}

void incorrectDisplay() {
  // grab random audio
  int incorrect_audio_n = random(0, 5) + AUDIO_INCORRECT_OFFSET;
  // play audio
  player.play(incorrect_audio_n);
  delay(AUDIO_INCORRECT_DELAY);
}

void roundcompleteDisplay() {
  // grab correct audio
  int super_round_audio_n = ((int)(round_count/SUB_ROUNDS)-1) + AUDIO_ROUNDS_OFFSET;
  // play audio
  player.play(super_round_audio_n);
  // flash lights
  int display_config[4] = {1,1,1,1};
  LEDconfigDisplay(display_config, FLASH_DELAY);

  delay(AUDIO_END_ROUND_DELAY);
}

void successDisplay() {
  // flash lights in scale pattern
  int display_config[4] = {0,0,0,0};
  for (int i = 0; i < 4; i++) {
    display_config[i] = 1; // only one light at a time
    LEDconfigDisplay(display_config, 250);
    display_config[i] = 0; // reset light for next iteration
  }

  // open chest
  setChestPos(1.0);
}

////////////////////////////////////////////////////////////////////
// systemwide functions
////////////////////////////////////////////////////////////////////

void systemReset() {
  // create new random seed
  randomSeed(millis()); // new random seed

  // close chest
  setChestPos(0.0);

  // reset game logic
  round_count = 1;
  SuccessFlag = false;
  GameResetFlag = true;

  // set pattern (and config) sequence to 0s
  resetPattern();
}

////////////////////////////////////////////////////////////////////
// hardware functions
////////////////////////////////////////////////////////////////////
// pos is a float that ranges from 0 to 1
void setChestPos(float pos) {
  // ensure pos is between 0 and 1
  if (pos < 0) {pos = 0;}
  if (pos > 1) {pos = 1;}

  // setup servo loop
  int startPos = myServo.read();
  int endPos = (int)(abs(SERVO_OPEN - SERVO_CLOSED)*(1-pos)) + SERVO_OPEN; // maps pos to servo tick
  int currentPos = startPos;

  // run servo loop
  while (abs(endPos - currentPos) > 0) {
    myServo.write(currentPos);
    delay(SERVO_DELAY);
    currentPos += (currentPos < endPos) ? 1 : -1;
  }
}

////////////////////////////////////////////////////////////////////
// helper functions
////////////////////////////////////////////////////////////////////

bool timer(bool &waitflag, unsigned long &start, unsigned long duration) {
  if (!waitflag) {
    start = millis();
    waitflag = true;
  }

  if (millis() - start >= duration) {
    waitflag = false;
    return true; // timer finished
  }

  return false; // timer continues
}

bool doubleButtonPushed() {
  int sum_button_push = 0;
  sum_button_push = RedPushed + YellowPushed + GreenPushed + BluePushed;
  return (sum_button_push == 2);
}

void resetPattern() {
  for (int i = 0; i < MAX_PATTERN_LENGTH; i++) {
    pattern[i] = 0;
    pattern_config[i] = 0;
  }
  pattern_length = 0;
  userPos = 0;
}

int getCurrentButton() {
  if (RedPushed) {
    return RED;
  } else if (YellowPushed) {
    return YELLOW;
  } else if (GreenPushed) {
    return GREEN;
  } else if (BluePushed) {
    return BLUE;
  } else {
    return -1; // no buttons are pushed
  }
}

int getLedPin(int color) {
  switch (color) {
      case RED:
        return LED_RED;
      case YELLOW:
        return LED_YELLOW;
      case GREEN:
        return LED_GREEN;
      case BLUE:
        return LED_BLUE;
      default:
        return -1;
    }
}

bool isButtonPushed(int button_pin) {
  return getPinVoltage(button_pin) < BUTTON_THRESHOLD;
}

// Function to turn LED on
void setLedStatus(int led_pin, bool status) {
  // if status true, turn on LED
  if (status) {
    digitalWrite(led_pin, HIGH);
  } else {
    digitalWrite(led_pin, LOW);
  }
}

float getPinVoltage(int pin) {
  return 5 * (float)analogRead(pin) / 1024;
}