#include <Wire.h>
#include <TFT_eSPI.h>
#include <NimBLEDevice.h>
#include <math.h>

// The remote service we wish to connect to.
static NimBLEUUID serviceUUID("c32cdd1f-baf2-46bc-87a1-69ab9637bfe0");
// The characteristic of the remote service we are interested in.
static NimBLEUUID charUUID("23680ff2-e66b-4a4e-a051-422d2665c443");

//Define the two CS pins to toggle between the two screens
//These can be any two GPIO pins
#define screen_0_CS  10      
#define screen_1_CS  15
//Rotation values, change as required
//These are for a screen in portrait mode, swap if using landscape
#define rotation_0 1
#define rotation_1 3

//Sensor Variables
int shockSensorBackValue = 0;
int shockSensorFrontValue = 0;
int gForceValue = 0;
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
  img.fillRect(0, 0, 230, 230, TFT_BLACK); // Fill the sprite with black before drawing

  const int cx = 115;
  const int cy = 115;
  const int borderR = 95;
  const int charR = 105;
  const int markInnerR = 100;
  const int markOuterR = 110;
  const int needleBaseR = 82;
  const int needleTipR = 91;

  // Draw Compass (GPS):
  img.drawCircle(cx, cy, borderR, TFT_WHITE); // Border circle
  img.drawChar(cos(radians(-90 - compassValue)) * charR + cx, sin(radians(-90 - compassValue)) * charR + cy, 'N', TFT_ORANGE, TFT_BLACK, 2); // North indicator
  img.drawChar(cos(radians(-270 - compassValue)) * charR + cx, sin(radians(-270 - compassValue)) * charR + cy, 'S', TFT_ORANGE, TFT_BLACK, 2); // South indicator
  img.drawChar(cos(radians(0 - compassValue)) * charR + cx, sin(radians(0 - compassValue)) * charR + cy, 'E', TFT_ORANGE, TFT_BLACK, 2); // East indicator
  img.drawChar(cos(radians(-180 - compassValue)) * charR + cx, sin(radians(-180 - compassValue)) * charR + cy, 'W', TFT_ORANGE, TFT_BLACK, 2); // West indicator

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

  // Draw compass needle (rotates with compassValue)
  img.fillTriangle(
    (int)(cos(radians(-90 + 5 - compassValue)) * needleBaseR + cx), (int)(sin(radians(-90 + 5 - compassValue)) * needleBaseR + cy),
    (int)(cos(radians(-90 - 5 - compassValue)) * needleBaseR + cx), (int)(sin(radians(-90 - 5 - compassValue)) * needleBaseR + cy),
    (int)(cos(radians(270 - compassValue)) * needleTipR + cx), (int)(sin(radians(270 - compassValue)) * needleTipR + cy),
    TFT_RED
  ); // North (red)

  img.pushSprite(5, 5); // Push the sprite to the TFT at coordinates (0,0)
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
  bool ok = img.createSprite(230, 230);
  Serial.print("createSprite(240,240) returned: "); Serial.println(ok);
  Serial.print("Free heap after create: "); Serial.println(ESP.getFreeHeap());
  if (!ok) {
    Serial.println("Sprite creation failed - try lower color depth or enable PSRAM.");
  }
  //Initialize BLE Serial
  //reconnectToServer(true);
  // Register Serial receive callback once (do NOT register inside loop)
  Serial.onReceive([]() {
    while (Serial.available()) {
      int c = Serial.read();
      if (c == '\n' || c == '\r' || serialIdx >= sizeof(serialBuf) - 1) {
        serialBuf[serialIdx] = '\0';
        serialIdx = 0;
        serialLineReady = true;
      } else {
        serialBuf[serialIdx++] = (char)c;
      }
    }
  });
   
 
}
void loop() {
  // Update screen 0 (left Screen)
  // SCREEN 0
  // Shock sensors (BACK/FRONT)
  // G-FORCE display
  // Optimal tilt angle calc
  // Compass (GPS)
  updateScreen0();
  // If a full serial line is available, parse it here (safe place for blocking calls)
  if (serialLineReady) {
    noInterrupts();
    char localBuf[32];
    strncpy(localBuf, serialBuf, sizeof(localBuf));
    serialLineReady = false;
    interrupts();
    int v = atoi(localBuf);
    compassValue = v;
    Serial.println(compassValue);
  }
}
