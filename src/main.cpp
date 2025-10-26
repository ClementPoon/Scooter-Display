#include <Wire.h>
#include <TFT_eSPI.h>
#include <NimBLEDevice.h>
#include <math.h>
#include <Scooter.h>

// The remote service we wish to connect to.
static NimBLEUUID serviceUUID("c32cdd1f-baf2-46bc-87a1-69ab9637bfe0");
// The characteristic of the remote service we are interested in.
static NimBLEUUID charUUID("23680ff2-e66b-4a4e-a051-422d2665c443");

//Define the two CS pins to toggle between the two screens
//These can be any two GPIO pins
#define screen_0_CS  10      
#define screen_1_CS  8
//Rotation values, change as required
//These are for a screen in portrait mode, swap if using landscape
#define rotation_0 1
#define rotation_1 3

//Sensor Variables
int shockSensorBackValue = 0;
int shockSensorFrontValue = 0;
int gForceValue = 100;
float tiltAngleValue = 0.0;
int compassValue = 0; //From 0-360 degrees
// Serial input buffering (used by onReceive callback)
volatile bool serialLineReady = false;
char serialBuf[32];
volatile size_t serialIdx = 0;


//Create TFT Colors
#define TFT_BLACK       0x0000      /*   0,   0,   0 */
#define TFT_NAVY        0x000F      /*   0,   0, 128 */
#define TFT_DARKGREEN   0x03E0      /*   0, 128,   0 */
#define TFT_DARKCYAN    0x03EF      /*   0, 128, 128 */
#define TFT_MAROON      0x7800      /* 128,   0,   0 */
#define TFT_PURPLE      0x780F      /* 128,   0, 128 */
#define TFT_OLIVE       0x7BE0      /* 128, 128,   0 */
#define TFT_LIGHTGREY   0xC618      /* 192, 192, 192 */
#define TFT_DARKGREY    0x7BEF      /* 128, 128, 128 */
#define TFT_BLUE        0x001F      /*   0,   0, 255 */
#define TFT_GREEN       0x07E0      /*   0, 255,   0 */
#define TFT_CYAN        0x07FF      /*   0, 255, 255 */
#define TFT_RED         0xF800      /* 255,   0,   0 */
#define TFT_MAGENTA     0xF81F      /* 255,   0, 255 */
#define TFT_YELLOW      0xFFE0      /* 255, 255,   0 */
#define TFT_WHITE       0xFFFF      /* 255, 255, 255 */ 

TFT_eSPI tft = TFT_eSPI();       // Invoke custom library

//Sprites
TFT_eSprite img = TFT_eSprite(&tft); // Create Sprite object "img" with pointer to "tft" object

//Scooter bitmap images


void toggleScreen(bool screen0, bool screen1) {
  if (screen0 && !screen1) {
    digitalWrite (screen_0_CS, LOW);   // select screen 0
    digitalWrite (screen_1_CS, HIGH);  // deselect screen 1
  }
  else if(screen1 && !screen0) {
    digitalWrite (screen_0_CS, HIGH);  // deselect screen 0
    digitalWrite (screen_1_CS, LOW);   // select screen 1
  }
  else {
    digitalWrite (screen_0_CS, LOW);  // select screen 0
    digitalWrite (screen_1_CS, LOW);  // select screen 1
  }
}

// Update screen 0 content
// Screen 0 has shock sensors (BACK/FRONT), G-FORCE display, Optimal tilt angle calc, Compass (GPS)
/////////////////////////////////////////////////
void updateScreen0() {
  //Select screen 0
  toggleScreen(true, false);   
  //Clear screen 
  img.fillRect(0, 0, 240, 240, TFT_BLACK); // Fill the sprite with black before drawing

  const int cx = 120;
  const int cy = 120;
  const int borderR = 90;
  const int charR = 105;
  const int markInnerR = 100;
  const int markOuterR = 105;
  const int needleBaseR = 81;
  const int needleTipR = 90;

  //Boost angle Vars
  int greenBoostAngle = 40;
  int YellowBoostAngle = 60;
  int redBoostAngle = 80;

  // Draw Compass (GPS):
  img.drawChar(cos(radians(-90 - compassValue)) * charR + cx, sin(radians(-90 - compassValue)) * charR + cy, 'N', TFT_RED, TFT_BLACK, 1.7); // North indicator
  img.drawChar(cos(radians(-270 - compassValue)) * charR + cx, sin(radians(-270 - compassValue)) * charR + cy, 'S', TFT_ORANGE, TFT_BLACK, 1.7); // South indicator
  img.drawChar(cos(radians(0 - compassValue)) * charR + cx, sin(radians(0 - compassValue)) * charR + cy, 'E', TFT_ORANGE, TFT_BLACK, 1.7); // East indicator
  img.drawChar(cos(radians(-180 - compassValue)) * charR + cx, sin(radians(-180 - compassValue)) * charR + cy, 'W', TFT_ORANGE, TFT_BLACK, 1.7); // West indicator

  // Draw line markers
  for (int i = 0; i < 360; i += 18) {
    if (i % 90 != 0) { // Skip N, S, E, W
      int sx = (int)(cos(radians(-i - compassValue)) * markInnerR + cx);
      int sy = (int)(sin(radians(-i - compassValue)) * markInnerR + cy);
      int ex = (int)(cos(radians(-i - compassValue)) * markOuterR + cx);
      int ey = (int)(sin(radians(-i - compassValue)) * markOuterR + cy);
      img.drawLine(sx, sy, ex, ey, TFT_WHITE);
    }
  }

  //Draw Referencce needle (fixed North position)
  img.fillTriangle(
    (int)(cos(radians(-90 + 5)) * needleBaseR + cx), (int)(sin(radians(-90 + 5)) * needleBaseR + cy),
    (int)(cos(radians(-90 - 5)) * needleBaseR + cx), (int)(sin(radians(-90 - 5)) * needleBaseR + cy),
    (int)(cos(radians(270)) * needleTipR + cx), (int)(sin(radians(270)) * needleTipR + cy),
    TFT_RED
  );

  // Draw Bottom Boost Indicator
  img.fillRect(0, 120, 240, 120, TFT_BLACK); // Fill the bottom half with black (placeholder for boost indicator)
  //Draw an arc using the boost value (placeholder logic)
  int boostAngle = map(gForceValue, 0, 100, 0, 80); // Map gForceValue (0-100) to angle (0-180)
  for (int b = 0; b <= 10; b++) {
    for (int a = 0; a <= boostAngle; a++) {
      int x = (int)(cos(radians(90 - a)) * (borderR + 20 + b) + cx);
      int y = (int)(sin(radians(90 - a)) * (borderR + 20 +b) + cy); // Offset down by 60 for bottom half
      // increase radius as 'a' increases so the arc follows the outside
      // extraRadius scales from 0..10 pixels (change 10.0f to tune how much it flares out)
      float extraRadius = (boostAngle > 0) ? ( (float)a / boostAngle ) * 7.0f : 0.0f;
      float r = borderR + 20 + b + extraRadius;
      int xi = (int)(cos(radians(90 - a)) * r + cx);
      int yi = (int)(sin(radians(90 - a)) * r + cy);
      if (a < greenBoostAngle)
        img.drawPixel(xi, yi, TFT_WHITE);
      else if (a < YellowBoostAngle)
        img.drawPixel(xi, yi, TFT_YELLOW);
      else if (a < redBoostAngle)
        img.drawPixel(xi, yi, TFT_RED);
    }
    for (int a = 0; a <= boostAngle; a++) {
      // mirror the arc to the other side by using 90 + a consistently
      float ang = radians(90 + a);
      // increase radius as 'a' increases so the arc follows the outside
      // extraRadius scales from 0..10 pixels (change 10.0f to tune how much it flares out)
      float extraRadius = (boostAngle > 0) ? ((float)a / boostAngle) * 7.0f : 0.0f;
      // make the outermost (max a) slightly thicker
      if (a >= boostAngle - 1) extraRadius += 4.0f;
      float r = borderR + 20 + b + extraRadius;
      int xi = (int)(cos(ang) * r + cx);
      int yi = (int)(sin(ang) * r + cy);
      if (a < greenBoostAngle)
      img.drawPixel(xi, yi, TFT_WHITE);
      else if (a < YellowBoostAngle)
      img.drawPixel(xi, yi, TFT_YELLOW);
      else if (a < redBoostAngle)
      img.drawPixel(xi, yi, TFT_RED);
    }
  }

  // Draw a triangle on the scooter wheels and increase red color from green as shock sensors increase
  // Wheel positions are chosen relative to the scooter bitmap drawn at (80,70) with size 80x110.
  // Adjust leftWheelX/Y and rightWheelX/Y if your bitmap positions differ.
  {
    auto rgbTo565 = [](uint8_t r, uint8_t g, uint8_t b) -> uint16_t {
      return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
    };

    // Clamp raw analog values to expected range then map to 0..100
    int backVal = constrain(shockSensorBackValue, 0, 256);
    int frontVal = constrain(shockSensorFrontValue, 0, 256);
    int backPct = map(backVal, 0, 256, 0, 100);
    int frontPct = map(frontVal, 0, 256, 0, 100);

    // Compute RGB color that interpolates from green (0) to red (100)
    auto interpColor = [&](int pct)->uint16_t {
      pct = constrain(pct, 0, 100);
      uint8_t r = map(pct, 0, 100, 0, 255);
      uint8_t g = map(pct, 0, 100, 255, 0);
      uint8_t b = 0;
      return rgbTo565(r, g, b);
    };

    uint16_t backColor = interpColor(backPct);
    uint16_t frontColor = interpColor(frontPct);

    // Triangle size scales with shock intensity (base 6..20)
    auto triangleSize = [&](int pct)->int {
      return 6 + (pct * 14) / 100;
    };

    // Wheels are vertically aligned (same X), different Y positions
    int wheelX = 130;         // common X for both wheels (adjust if needed)
    int backWheelY = 170;     // Y for back wheel (lower)
    int frontWheelY = 120;    // Y for front wheel (higher)

    // Back wheel triangle rotated 90 degrees (pointing right)
    int sBack = triangleSize(backPct);
    int halfWBack = sBack;            // half-height of the base (vertical half-size)
    int depthBack = sBack;            // how far the triangle extends horizontally (to the right)
    img.fillTriangle(
      wheelX + depthBack, backWheelY,                 // apex (right)
      wheelX, backWheelY - halfWBack,     // top-left
      wheelX, backWheelY + halfWBack,     // bottom-left
      backColor
    );

    // Front wheel triangle rotated 90 degrees (pointing right)
    int sFront = triangleSize(frontPct);
    int halfWFront = sFront;
    int depthFront = sFront;
    img.fillTriangle(
      wheelX + depthFront, frontWheelY,               // apex (right)
      wheelX, frontWheelY - halfWFront,  // top-left
      wheelX, frontWheelY + halfWFront,  // bottom-left
      frontColor
    );
  }


  //Draw Scooter Bitmap in the center
  img.drawBitmap(80, 70, scooterBitmap, 80, 110, TFT_WHITE); // Draw scooter bitmap at (80,65)
  img.pushSprite(0, 0); // Push the sprite to the TFT at coordinates (0,0)
  // Shock Sensors (BACK/FRONT):
  // Analog read from 0 to 4095 (12 bits) from the shock sensors
  // Map to 0 to 100% for display
  // Rand gen values to create gradient effect (increase near wheels and increase when shock detected)
  // Write individual pixels with randnum (1/0)


}
void drawLoadingScreen(int increment) {
  //Select screen 0
  toggleScreen(true, false);   
  //Clear screen 
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK); 
  tft.setTextSize(2);
  tft.setCursor(60, 110);
  tft.println("Loading...");
}

NimBLEClient *pClient = nullptr;
void reconnectToServer(bool reconnect) {
  // Logic to reconnect to BLE server
  // This is a placeholder function and should contain the actual reconnection logic
  if (reconnect) {
    NimBLEDevice::init("");

    NimBLEScan *pScan = NimBLEDevice::getScan();
    NimBLEScanResults results = pScan->getResults(10 * 1000);

    NimBLEUUID serviceUuid("c32cdd1f-baf2-46bc-87a1-69ab9637bfe0");

    for (int i = 0; i < results.getCount(); i++) {
        const NimBLEAdvertisedDevice *device = results.getDevice(i);

        if (device->isAdvertisingService(serviceUuid)) {
            pClient = NimBLEDevice::createClient();
            Serial.println("Connecting to device...");

            if (!pClient) { // Make sure the client was created
                break;
            }

            // inside your loop over results, when you find a device advertising the service:
            // before attempting connection, stop the scan to free the radio
            pScan->stop();

            const NimBLEAdvertisedDevice *device = results.getDevice(i);
            Serial.println("---- candidate device ----");
            Serial.print("Device string: "); Serial.println(device->toString().c_str());
            Serial.print("Address: "); Serial.println(device->getAddress().toString().c_str());
            Serial.print("RSSI: "); Serial.println(device->getRSSI());
            Serial.print("Connectable: "); Serial.println(device->isConnectable() ? "yes" : "no");

            if (!device->isConnectable()) {
              Serial.println("Device not connectable, skipping.");
              continue;
            }

            NimBLEClient *client = NimBLEDevice::createClient();
            if (!client) {
              Serial.println("createClient() failed");
              continue;
            }

            // increase connect timeout (seconds)
            client->setConnectTimeout(10);

            bool connected = false;
            const int maxAttempts = 10;
            for (int attempt = 1; attempt <= maxAttempts && !connected; ++attempt) {
              Serial.printf("Connect attempt %d/%d (via advertised device)...\n", attempt, maxAttempts);
              if (client->connect(device)) {
                Serial.println("Connected (via advertised device).");
                connected = true;
                break;
              }
              Serial.println("connect(device) FAILED, retrying...");
              delay(200); // small backoff
            }

            // fallback: try connect by address if pointer connect failed
            if (!connected) {
              NimBLEAddress addr = device->getAddress();
              Serial.print("Trying fallback connect by address: ");
              Serial.println(addr.toString().c_str());
              if (client->connect(addr)) {
                Serial.println("Connected (via address fallback).");
                connected = true;
              } else {
                Serial.println("Fallback connect by address FAILED.");
              }
            }

            if (connected) {
              pClient = client; // keep client for later use
              // optionally discover services here
              return; // connected - exit function
            }
        }
    }
  }
else {
    Serial.println("Reconnection not requested.");
    NimBLEDevice::deleteClient(pClient);
  }
}

void fetchDataFromBLE() {
  if (pClient && pClient->isConnected()) {
    NimBLERemoteService *pService = pClient->getService(serviceUUID);
    if (pService != nullptr) {
      NimBLERemoteCharacteristic *pCharacteristic = pService->getCharacteristic(charUUID);
      if (pCharacteristic != nullptr) {
        std::string value = pCharacteristic->readValue();
        Serial.print("Characteristic value: ");
        Serial.println(value.c_str());
        // Process the value as needed
      }
    }
  } else {
    Serial.println("Client not connected");
  }
}


void setup() {
  Serial.begin(9600);
  Serial.println("TFT_eSPI test");
  pinMode (screen_0_CS, OUTPUT);
  pinMode (screen_1_CS, OUTPUT);
  //INIT both screens
  toggleScreen(true, false);                               // we need to 'init' both displays
  //Init tft             
  tft.init ();   

  toggleScreen(false, true);                               // we need to 'init' both displays
  tft.init ();
  //Now both displays are inited, we can select one to work with
  digitalWrite (screen_0_CS, HIGH);                              // set both CS pins HIGH, or 'inactive'
  digitalWrite (screen_1_CS, HIGH);
  //Set Rotation and Clear Screen again
  toggleScreen(true, false);                             // select screen 0
  tft.setRotation(rotation_0);                        // rotate as required
  tft.fillScreen(TFT_BLACK);
  toggleScreen(false, true);                             // select screen 1
  tft.setRotation(rotation_1);                        // rotate as required
  tft.fillScreen(TFT_BLACK);

  //Init Sprite
  img.setColorDepth(8);                     // MUST set before creating sprite
  Serial.print("Free heap before create: "); Serial.println(ESP.getFreeHeap());
  #ifdef ESP32
    Serial.print("PSRAM size: "); Serial.println(ESP.getPsramSize());
    Serial.print("Free PSRAM: "); Serial.println(ESP.getFreePsram());
  #endif
  // Do not access internal/private members of the library (img._psram_enable).
  // If PSRAM must be enabled, use the library's public API or configure PSRAM globally.
  bool ok = img.createSprite(240, 240);
  Serial.print("createSprite(240,240) returned: "); Serial.println(ok);
  Serial.print("Free heap after create: "); Serial.println(ESP.getFreeHeap());
  if (!ok) {
    Serial.println("Sprite creation failed - try lower color depth or enable PSRAM.");
  }
  //Initialize BLE Serial
  //reconnectToServer(true);
  
   
 
}
void loop() {
  // Update screen 0 (left Screen)
  // SCREEN 0
  // Shock sensors (BACK/FRONT)
  // G-FORCE display
  // Optimal tilt angle calc
  // Compass (GPS)
  updateScreen0();

  compassValue += 1;
  if (compassValue >= 360) {
    compassValue = 0;
}
gForceValue += 5;
if (gForceValue > 100) {
  gForceValue = 0;
}
shockSensorBackValue += 8;
if (shockSensorBackValue > 256) {
  shockSensorBackValue = 0;
}
shockSensorFrontValue += 8;
if (shockSensorFrontValue > 256) {
  shockSensorFrontValue = 0;
} 

}
