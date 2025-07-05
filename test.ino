#define VY_PIN 32
#define VX_PIN 34
#define RX 16
#define TX 17
#define PAYLOAD_SIZE 4
#define AUX_PIN 26

HardwareSerial SerialE32(2);

const uint8_t w = 200;
const uint16_t vx_center = 1820;
const uint16_t vy_center = 1800;

uint16_t joystick_input[] = {0, 0};

struct __attribute__((packed)) MainPayload {
  uint8_t comando; // 100 a 200
  int8_t  vx; // -100...+100
  int8_t  vy; // -100...+100
  uint8_t powerPerc; // 0...100
};

void setup() {
  Serial.begin(115200);
  SerialE32.begin(9600, SERIAL_8N1, RX, TX);
  Serial0.begin(115200);
  pinMode(AUX_PIN, INPUT);
}

uint8_t localBuffer[4];

MainPayload readInputs(uint16_t vx, uint16_t vy) {
  MainPayload payload = {0, 0, 0, 0};

  if(vx > vx_center + w) {
    float vx_percentage = 1 - ((4095 - vx) / (4095 - (vx_center + w)));
    Serial.println(vx_percentage);
    int16_t vx_val = round(100 * vx_percentage);
    payload.vx = vx_val;
  } else if (vx < vx_center - w) {
    float vx_percentage = vx / (vx_center - w);
    int16_t vx_val = round(-100 * vx_percentage);
    payload.vx = vx_val;
  } else {
    payload.vx = 0;
  }

  Serial.println(vx);
  Serial.println(payload.vx);
  delay(2000);
  return payload;
}

void loop() {
  SerialE32.flush();
  joystick_input[0] = analogRead(VX_PIN);
  joystick_input[1] = analogRead(VY_PIN);

  Serial.printf("VX: %d | VY: %d", joystick_input[0], joystick_input[1]);
  MainPayload payload = readInputs(joystick_input[0], joystick_input[1]);

  //if (joystick_input[0] > vx_center + w)
  //else if (joystick_input[0] < vx_center - w)

  //if (joystick_input[1] > vy_center + w)
  //else if (joystick_input[1] < vy_center - w)

  // POPULA O BUFFER
  memcpy(localBuffer, &payload, PAYLOAD_SIZE);

  while(!digitalRead(AUX_PIN)) {
  }
  delay(5);
  SerialE32.write(localBuffer, PAYLOAD_SIZE);

  uint8_t readBuffer[4];
  if (SerialE32.available()) {
    SerialE32.readBytes(readBuffer, PAYLOAD_SIZE);
    Serial.printf("RAW: %02X %02X %02X %02X\n", readBuffer[0], readBuffer[1], readBuffer[2], readBuffer[3]);
  }
  delay(50);
}
