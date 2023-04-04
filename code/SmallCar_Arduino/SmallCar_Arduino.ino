#include "PinDefinitionsAndMore.h" // Define macros for input and output pin etc.
#include <IRremote.hpp>
#include <SoftwareSerial.h>
#include <ArduinoJson.h>

// #define CONFIG_DISTANCE_DEBUG
// #define CONFIG_SPEED_DEBUG
// #define CONFIG_IRREMOTE_DEBUG
// #define CONFIG_SOFTSERIAL_DEBUG

#define DECODE_NEC // Includes Apple and Onkyo
#define IR_RECEIVE_PIN 7

#define LATCH_PIN 13 // Latch pin of 74HC595 is connected to Digital pin 13
#define CLOCK_PIN 4  // Clock pin of 74HC595 is connected to Digital pin 4
#define DATA_PIN 12  // Data pin of 74HC595 is connected to Digital pin 12

byte values = 0; // Variable to hold the pattern of which pins are currently turned HIGH or LOW

// 74hc595 - motor
#define MOTOR_LEFT_FORWARD_PIN_SR 4
#define MOTOR_LEFT_BACK_PIN_SR 3
#define MOTOR_RIGHT_FORWARD_PIN_SR 1
#define MOTOR_RIGHT_BACK_PIN_SR 2
// 74hc595 - led
#define LED_LEFT_PIN_SR 5
#define LED_RIGHT_PIN_SR 6
// 74hc595 - buzzer
#define BUZZER_PIN_SR 0

// LN298N - Arduino
#define LEFT_PWM_PIN 6
#define RIGHT_PWM_PIN 5

// motor action code
const char MOTOR_GO = 'G';
const char MOTOR_BACK = 'B';
const char MOTOR_STOP = 'S';
const char MOTOR_LEFT = 'L';
const char MOTOR_RIGHT = 'R';
const char MOTOR_UP = 'U';
const char MOTOR_DOWN = 'D';

char motor_state = MOTOR_STOP;
char motor_cmd = MOTOR_STOP;

#define SPEED_MAX 250
#define SPEED_MIN 50

// PWM - speed
int step_speed = 10;
int left_speed = 130;
int right_speed = 135;
int turn_speed_diff = 60;

long previousMillis_blink; // for led blink
const int interval_blink = 200;
long previousMillis_buzzer;
const int interval_buzzer = 20;

// softserial - Arduino
#define SS_RX_PIN 10
#define SS_TX_PIN 11
SoftwareSerial espSerial(SS_RX_PIN, SS_TX_PIN);

// Arduino UNO/Pro Mini intercept0，1(pin2,3)
int leftCounter = 0, rightCounter = 0;
unsigned long time = 0, old_time = 0;
unsigned long time1 = 0;
float lv, rv;

// ultrasonic - Arduino
#define TRIG_PIN 8 // Trigger
#define ECHO_PIN 9 // Echo
long duration;
float distance;

// send data to esp8266
const int interval_send_data = 100;
long previousMillis_send_data;
StaticJsonDocument<200> sensor_json;

// 74HC595N
void shift_register_update()
{
  //
  digitalWrite(LATCH_PIN, LOW);
  shiftOut(DATA_PIN, CLOCK_PIN, LSBFIRST, values);
  digitalWrite(LATCH_PIN, HIGH);
};

void shift_register_init()
{
  // Set all the pins of 74HC595 as OUTPUT
  pinMode(LATCH_PIN, OUTPUT);
  pinMode(DATA_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  shift_register_update();
}

void motor_init()
{
  analogWrite(LEFT_PWM_PIN, 0);
  analogWrite(RIGHT_PWM_PIN, 0);

  bitClear(values, 7 - MOTOR_LEFT_FORWARD_PIN_SR);
  bitClear(values, 7 - MOTOR_LEFT_BACK_PIN_SR);
  bitClear(values, 7 - MOTOR_RIGHT_FORWARD_PIN_SR);
  bitClear(values, 7 - MOTOR_RIGHT_BACK_PIN_SR);
  shift_register_update();
}

void motor_go()
{
  // left forward
  analogWrite(LEFT_PWM_PIN, left_speed);
  bitSet(values, 7 - MOTOR_LEFT_FORWARD_PIN_SR);
  bitClear(values, 7 - MOTOR_LEFT_BACK_PIN_SR);
  // right forward
  analogWrite(RIGHT_PWM_PIN, right_speed);
  bitSet(values, 7 - MOTOR_RIGHT_FORWARD_PIN_SR);
  bitClear(values, 7 - MOTOR_RIGHT_BACK_PIN_SR);
}

void motor_back()
{
  analogWrite(LEFT_PWM_PIN, left_speed);
  bitClear(values, 7 - MOTOR_LEFT_FORWARD_PIN_SR);
  bitSet(values, 7 - MOTOR_LEFT_BACK_PIN_SR);

  analogWrite(RIGHT_PWM_PIN, right_speed);
  bitClear(values, 7 - MOTOR_RIGHT_FORWARD_PIN_SR);
  bitSet(values, 7 - MOTOR_RIGHT_BACK_PIN_SR);
}

void motor_turn_left()
{
  // right speed>left speed
  left_speed = right_speed - turn_speed_diff;
  if (left_speed < 0)
  {
    left_speed = 0;
  };
  analogWrite(LEFT_PWM_PIN, left_speed);
  bitSet(values, 7 - MOTOR_LEFT_FORWARD_PIN_SR);
  bitClear(values, 7 - MOTOR_LEFT_BACK_PIN_SR);

  analogWrite(RIGHT_PWM_PIN, right_speed);
  bitSet(values, 7 - MOTOR_RIGHT_FORWARD_PIN_SR);
  bitClear(values, 7 - MOTOR_RIGHT_BACK_PIN_SR);
}

void motor_turn_right()
{
  // left speed>right speed
  analogWrite(LEFT_PWM_PIN, left_speed);
  bitSet(values, 7 - MOTOR_LEFT_FORWARD_PIN_SR);
  bitClear(values, 7 - MOTOR_LEFT_BACK_PIN_SR);
  right_speed = left_speed - turn_speed_diff;
  if (right_speed < 0)
  {
    right_speed = 0;
  }
  analogWrite(RIGHT_PWM_PIN, right_speed);
  bitSet(values, 7 - MOTOR_RIGHT_FORWARD_PIN_SR);
  bitClear(values, 7 - MOTOR_RIGHT_BACK_PIN_SR);
}

void motor_stop()
{
  motor_init();
}

void motor_action(char motor_cmd)
{
  switch (motor_cmd)
  {
  case MOTOR_GO:
    bitSet(values, 7 - LED_LEFT_PIN_SR);
    bitSet(values, 7 - LED_RIGHT_PIN_SR);
    motor_go();
    shift_register_update();
    motor_state = motor_cmd;
    break;
  case MOTOR_STOP:
    bitClear(values, 7 - LED_LEFT_PIN_SR);
    bitClear(values, 7 - LED_RIGHT_PIN_SR);
    motor_stop();
    shift_register_update();
    motor_state = motor_cmd;
    break;
  case MOTOR_BACK:
    previousMillis_blink = 0;
    bitClear(values, 7 - LED_LEFT_PIN_SR);
    bitClear(values, 7 - LED_RIGHT_PIN_SR);
    motor_back();
    shift_register_update();
    motor_state = motor_cmd;
    break;
  case MOTOR_LEFT:
    bitSet(values, 7 - LED_LEFT_PIN_SR);
    bitClear(values, 7 - LED_RIGHT_PIN_SR);
    motor_turn_left();
    shift_register_update();
    motor_state = motor_cmd;
    break;
  case MOTOR_RIGHT:
    bitClear(values, 7 - LED_LEFT_PIN_SR);
    bitSet(values, 7 - LED_RIGHT_PIN_SR);
    motor_turn_right();
    shift_register_update();
    motor_state = motor_cmd;
    break;
  default:
    break;
  };
};

void update_motor_speed(int step_speed_value)
{
  left_speed += step_speed_value;
  right_speed += step_speed_value;

  if (left_speed > SPEED_MAX)
  {
    left_speed = SPEED_MAX;
  }
  else if (left_speed < SPEED_MIN)
  {
    left_speed = SPEED_MIN;
  };
  // right_speed
  if (right_speed > SPEED_MAX)
  {
    right_speed = SPEED_MAX;
  }
  else if (right_speed < SPEED_MIN)
  {
    right_speed = SPEED_MIN;
  };

  analogWrite(LEFT_PWM_PIN, left_speed);
  analogWrite(RIGHT_PWM_PIN, right_speed);
}

void irremote_init()
{
#ifdef CONFIG_IRREMOTE_DEBUG
  Serial.println(F("START " __FILE__ " from " __DATE__ "\r\nUsing library version " VERSION_IRREMOTE));
#endif
  IrReceiver.begin(IR_RECEIVE_PIN, DISABLE_LED_FEEDBACK);
#ifdef CONFIG_IRREMOTE_DEBUG
  Serial.print(F("Ready to receive IR signals of protocols: "));
  printActiveIRProtocols(&Serial);
  Serial.println(F("at pin " STR(IR_RECEIVE_PIN)));
#endif
};

void irremote_cmd()
{
  if (IrReceiver.decode())
  {
#ifdef CONFIG_IRREMOTE_DEBUG
    // Print a short summary of received data
    IrReceiver.printIRResultShort(&Serial);
    if (IrReceiver.decodedIRData.protocol == UNKNOWN)
    {
      Serial.println(F("Received noise or an unknown (or not yet enabled) protocol"));
      IrReceiver.printIRResultRawFormatted(&Serial, true);
    }
    Serial.println();
#endif
    //!!!Important!!! Enable receiving of the next value,
    IrReceiver.resume(); // Enable receiving of the next value
    if (IrReceiver.decodedIRData.command == 0x18)
    {
      motor_cmd = MOTOR_GO;
    }
    else if (IrReceiver.decodedIRData.command == 0x8)
    {
      motor_cmd = MOTOR_LEFT;
    }
    else if (IrReceiver.decodedIRData.command == 0x5A)
    {
      motor_cmd = MOTOR_RIGHT;
    }
    else if (IrReceiver.decodedIRData.command == 0x52)
    {
      motor_cmd = MOTOR_BACK;
    }
    else if (IrReceiver.decodedIRData.command == 0x1C)
    {
      motor_cmd = MOTOR_STOP;
    }
    else if (IrReceiver.decodedIRData.command == 0x15)
    {
      // + speed
      update_motor_speed(step_speed);
    }
    else if (IrReceiver.decodedIRData.command == 0x7)
    {
      // - speed
      update_motor_speed(-step_speed);
    };
  };
  if (motor_cmd != motor_state)
  {
    motor_action(motor_cmd);
  };
}

void softserial_cmd()
{
  // from esp8266 softserial
  String inString = "";
  while (espSerial.available())
  {
    inString += char(espSerial.read());
#ifdef CONFIG_SOFTSERIAL_DEBUG
    Serial.print(inString);
#endif
  };

  if (inString.indexOf(MOTOR_GO) != -1)
  {
    motor_cmd = MOTOR_GO;
  }
  else if (inString.indexOf(MOTOR_STOP) != -1)
  {
    motor_cmd = MOTOR_STOP;
  }
  else if (inString.indexOf(MOTOR_BACK) != -1)
  {
    motor_cmd = MOTOR_BACK;
  }
  else if (inString.indexOf(MOTOR_LEFT) != -1)
  {
    motor_cmd = MOTOR_LEFT;
  }
  else if (inString.indexOf(MOTOR_RIGHT) != -1)
  {
    motor_cmd = MOTOR_RIGHT;
  }
  else if (inString.indexOf(MOTOR_UP) != -1)
  {
    update_motor_speed(step_speed);
  }
  else if (inString.indexOf(MOTOR_DOWN) != -1)
  {
    update_motor_speed(-step_speed);
  };
  if (motor_cmd != motor_state)
  {
    motor_action(motor_cmd);
  };
}

bool speed_detection()
{
  time = millis();
  if (abs(time - old_time) >= 1000)
  {
    detachInterrupt(0);
    detachInterrupt(1);
    lv = (float)leftCounter * 60 / 20;
    rv = (float)rightCounter * 60 / 20;
#ifdef CONFIG_SPEED_DEBUG
    Serial.print("left:");
    Serial.print(lv);
    Serial.print("     right:");
    Serial.println(rv);
#endif
    leftCounter = 0;
    rightCounter = 0;
    old_time = millis();
    attachInterrupt(0, RightCount_CallBack, FALLING);
    attachInterrupt(1, LeftCount_CallBack, FALLING);
    return 1;
  }
  else
    return 0;
}

void LeftCount_CallBack()
{
  leftCounter++;
}

void RightCount_CallBack()
{
  rightCounter++;
}

void distance_detection()
{
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(5);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  pinMode(ECHO_PIN, INPUT);
  duration = pulseIn(ECHO_PIN, HIGH);
  distance = (duration / 2) / 29.1;
#ifdef CONFIG_DISTANCE_DEBUG
  Serial.print(distance);
  Serial.print("cm");
  Serial.println();
#endif
}

void ultrasonic_init()
{
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
}

void led_blink()
{
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis_blink >= interval_blink)
  {
    previousMillis_blink = currentMillis; // Remember the time
    if (bitRead(values, 7 - LED_LEFT_PIN_SR) == 1)
    {
      bitClear(values, 7 - LED_LEFT_PIN_SR);
      bitClear(values, 7 - LED_RIGHT_PIN_SR);
    }
    else if ((bitRead(values, 7 - LED_LEFT_PIN_SR) == 0))
    {
      bitSet(values, 7 - LED_LEFT_PIN_SR);
      bitSet(values, 7 - LED_RIGHT_PIN_SR);
    };
    shift_register_update();
  };
}

void buzzer_action()
{
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis_buzzer >= interval_buzzer)
  {
    previousMillis_buzzer = currentMillis;
    if (bitRead(values, 7 - BUZZER_PIN_SR) == 1)
    {
      bitClear(values, 7 - BUZZER_PIN_SR);
    }
    else if ((bitRead(values, 7 - BUZZER_PIN_SR) == 0))
    {
      bitSet(values, 7 - BUZZER_PIN_SR);
    };
    shift_register_update();
  };
}

void setup()
{
  Serial.begin(9600);

  shift_register_init();
  motor_init();
  irremote_init();
  ultrasonic_init();

  espSerial.begin(9600);
  previousMillis_send_data = 0;
  previousMillis_buzzer = 0;
};

void loop()
{
  bool sd = speed_detection();
  distance_detection();
  if (distance < 30)
  {
    if (motor_state == MOTOR_GO || motor_state == MOTOR_LEFT || motor_state == MOTOR_RIGHT)
    {
      motor_cmd = MOTOR_STOP;
      motor_action(motor_cmd);
    }
  }
  if (motor_state == MOTOR_BACK)
  {
    led_blink();
  };

  irremote_cmd();
  softserial_cmd();

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis_send_data >= interval_send_data)
  {
    previousMillis_send_data = currentMillis;
    sensor_json["distance"] = String(distance, 0);
    sensor_json["left_speed"] = String(lv, 0);
    sensor_json["right_speed"] = String(rv, 0);
    serializeJson(sensor_json, espSerial);
  };

  if (motor_state == MOTOR_GO && distance < 60)
  {
    buzzer_action();
  }
  else
  {
    previousMillis_buzzer = 0;
    if (bitRead(values, 7 - BUZZER_PIN_SR) == 1)
    {
      bitClear(values, 7 - BUZZER_PIN_SR);
    };
    shift_register_update();
  };
}
