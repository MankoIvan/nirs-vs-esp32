#include <WebServer.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <esp32cam.h>
#include <ESP32Servo.h>

Servo myservo1;  // create servo object to control a servo
Servo myservo2;  // create servo object to control a servo
// twelve servo objects can be created on most boards

int posH = 90;    // variable to store the Horizontal servo position
int posV = 60;    // variable to store the Vertical servo position
bool imageTransactionAvaliable = true;
uint32_t timeStart = 0;
uint32_t movementDetectionAllowedTime = 0;
bool movementDetected = false;


const char* WIFI_SSID = "xxxxxxxxx";
const char* WIFI_PASS = "xxxxxxxxx";



const char* serverName = "http://localhost:3000/coordinates/";
float sensorReadingsArr[10];

WebServer server(80);

static auto loRes = esp32cam::Resolution::find(320, 240);
static auto hiRes = esp32cam::Resolution::find(800, 600);

void
handleBmp()
{
  if (!esp32cam::Camera.changeResolution(loRes)) {
    Serial.println("SET-LO-RES FAIL");
  }

  auto frame = esp32cam::capture();
  if (frame == nullptr) {
    Serial.println("CAPTURE FAIL");
    server.send(503, "", "");
    return;
  }
  Serial.printf("CAPTURE OK %dx%d %db\n", frame->getWidth(), frame->getHeight(),
                static_cast<int>(frame->size()));

  if (!frame->toBmp()) {
    Serial.println("CONVERT FAIL");
    server.send(503, "", "");
    return;
  }
  Serial.printf("CONVERT OK %dx%d %db\n", frame->getWidth(), frame->getHeight(),
                static_cast<int>(frame->size()));

  server.setContentLength(frame->size());
  server.send(200, "image/bmp");
  WiFiClient client = server.client();
  frame->writeTo(client);
}

void
serveJpg()
{
  auto frame = esp32cam::capture();
  if (frame == nullptr) {
    Serial.println("CAPTURE FAIL");
    server.send(503, "", "");
    return;
  }
  Serial.printf("CAPTURE OK %dx%d %db\n", frame->getWidth(), frame->getHeight(),
                static_cast<int>(frame->size()));

  server.setContentLength(frame->size());
  server.send(200, "image/jpeg");
  WiFiClient client = server.client();
  frame->writeTo(client);
}

void
handleJpgLo()
{
  if (!esp32cam::Camera.changeResolution(loRes)) {
    Serial.println("SET-LO-RES FAIL");
  }
  serveJpg();
}

void
handleJpgHi()
{
  if (imageTransactionAvaliable) {
    if (!esp32cam::Camera.changeResolution(hiRes)) {
      Serial.println("SET-HI-RES FAIL");
    }
    serveJpg();
  } else {
    server.send(200, "text/plain", "wait please");
  }
}

void
handleJpg()
{
  server.sendHeader("Location", "/cam-hi.jpg");
  server.send(302, "", "");
}

void
handleMjpeg()
{
  if (!esp32cam::Camera.changeResolution(hiRes)) {
    Serial.println("SET-HI-RES FAIL");
  }

  Serial.println("STREAM BEGIN");
  WiFiClient client = server.client();
  auto startTime = millis();
  int res = esp32cam::Camera.streamMjpeg(client);
  if (res <= 0) {
    Serial.printf("STREAM ERROR %d\n", res);
    return;
  }
  auto duration = millis() - startTime;
  Serial.printf("STREAM END %dfrm %0.2ffps\n", res, 1000.0 * res / duration);
}
/*
void
turnRight()
{
  timeStart = millis();
  imageTransactionAvaliable = false;
  myservo1.attach(12);  // attaches the servo on pin 12 to the servo object
  Serial.println("go right");
  posH -= 30;
  myservo1.write(posH);              // tell servo to go to position in variable 'posH'
  server.send(200, "image/jpeg");
  Serial.println(posH);
  delay(100);
  myservo1.detach();
  delay(200);
  resetCam();
  Serial.println("time required");
  Serial.println(millis() - timeStart);
}

void
turnLeft()
{
  timeStart = millis();
  imageTransactionAvaliable = false;
  myservo1.attach(12);  // attaches the servo on pin 12 to the servo object
  Serial.println("go left");
  posH += 30;
  myservo1.write(posH);              // tell servo to go to position in variable 'posH'
  server.send(200, "image/jpeg");
  Serial.println(posH);
  delay(100);
  myservo1.detach();
  delay(200);
  resetCam();
  Serial.println("time required");
  Serial.println(millis() - timeStart);
}

void
turnUp()
{
  timeStart = millis();
  imageTransactionAvaliable = false;
  myservo2.attach(13);  // attaches the servo on pin 13 to the servo object
  Serial.println("go up");
  posV -= 30;
  myservo2.write(posV);              // tell servo to go to position in variable 'posV'
  server.send(200, "image/jpeg");
  Serial.println(posV);
  delay(100);
  myservo2.detach();
  delay(200);
  resetCam();
  Serial.println("time required");
  Serial.println(millis() - timeStart);
}

void
turnDown()
{
  timeStart = millis();
  imageTransactionAvaliable = false;
  myservo2.attach(13);  // attaches the servo on pin 13 to the servo object
  Serial.println("go down");
  posV += 30;
  myservo2.write(posV);              // tell servo to go to position in variable 'posV'
  server.send(200, "image/jpeg");
  Serial.println(posV);
  delay(100);
  myservo2.detach();
  delay(200);
  resetCam();
  Serial.println("time required");
  Serial.println(millis() - timeStart);
}
*/
void
resetCam()
{
  esp32cam::Camera.end();
  using namespace esp32cam;
  Config cfg;
  cfg.setPins(pins::AiThinker);
  cfg.setResolution(hiRes);
  cfg.setBufferCount(2);
  cfg.setJpeg(80);

  bool ok = Camera.begin(cfg);
  Serial.println(ok ? "CAMERA RESET OK" : "CAMERA RESET FAIL");
  server.send(200, "image/jpeg");
  imageTransactionAvaliable = true;
}

void
cameraAvaliable()
{
  server.send(200, "text/plain", "camera is avaliable");
  getCoordinates();
  moveCam(posH, posV);

}
void
cameraMoveHandler()
{
  server.send(200, "text/plain", "camera moving");
  getCoordinates();
  moveCam(posH, posV);
}
void
moveCam(int horizontalPos,int verticalPos)
{
  imageTransactionAvaliable = false;
  myservo1.attach(12);  // attaches the servo on pin 12 to the servo object
  delay(25);
  myservo2.attach(13);  // attaches the servo on pin 13 to the servo object
  delay(25);
  myservo2.write(verticalPos);              // tell servo to go to position in variable 'verticalPos'
  delay(100);
  myservo1.write(horizontalPos);              // tell servo to go to position in variable 'horizontalPos'
  delay(100);
  myservo1.detach();
  myservo2.detach();
  delay(200);
  resetCam();
}
void
getCoordinates()
{
  HTTPClient http;
  http.begin("http://172.20.10.4:3000/coordinates"); 
  int httpCode = http.GET();
  if (httpCode > 0) { 
    

    String payload = http.getString();
    Serial.println(httpCode);
    Serial.println(payload);
    http.end(); 
    posH = payload.substring(0,payload.indexOf(",")).toInt();
    posV = payload.substring(payload.indexOf(",") + 1,payload.length()).toInt();
  }
  else {
    Serial.println("Error on HTTP request");
  }
}
void
movementDetectedFunc()
{
  HTTPClient http;
  http.begin("http://172.20.10.4:3000/movementdetected"); 
  http.addHeader("Content-Type", "text/plain");       
  int httpResponseCode = http.POST("Movement detected");
  if (httpResponseCode>0) {
    String response = http.getString();
    Serial.println(httpResponseCode);
    Serial.println(response);
  } else {
    Serial.print("Error on sending POST: ");
    Serial.println(httpResponseCode);
  }
  http.end();  //Free resources
}


void
setup()
{

  Serial.begin(115200);
  Serial.println();

  {
    using namespace esp32cam;
    Config cfg;
    cfg.setPins(pins::AiThinker);
    cfg.setResolution(hiRes);
    cfg.setBufferCount(2);
    cfg.setJpeg(80);

    bool ok = Camera.begin(cfg);
    Serial.println(ok ? "CAMERA OK" : "CAMERA FAIL");
  }
  /*
    myservo1.attach(12);  // attaches the servo on pin 12 to the servo object
    myservo2.attach(13);  // attaches the servo on pin 13 to the servo object
    posH = myservo1.read();
    delay(200);
    posV = myservo2.read();
    delay(200);
    myservo1.write(posH);
    delay(200);
    myservo2.write(posV);
    delay(200);
    myservo1.detach();
    delay(200);
    myservo2.detach();
    delay(200);
    resetCam();
  */

  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  Serial.print("http://");
  Serial.println(WiFi.localIP());
  Serial.println("  /cam.bmp");
  Serial.println("  /cam-lo.jpg");
  Serial.println("  /cam-hi.jpg");
  Serial.println("  /cam.mjpeg");

  server.on("/cam.bmp", handleBmp);
  server.on("/cam-lo.jpg", handleJpgLo);
  server.on("/cam-hi.jpg", handleJpgHi);
  server.on("/cam.jpg", handleJpg);
  server.on("/cam.mjpeg", handleMjpeg);
  //server.on("/turn-right", turnRight);
  //server.on("/turn-left", turnLeft);
  //server.on("/turn-up", turnUp);
  //server.on("/turn-down", turnDown);
  server.on("/reset", resetCam);
  server.on("/cameraAvaliable", cameraAvaliable);
  server.on("/cameraMove", cameraMoveHandler);


  server.begin();

  pinMode(15, INPUT);
  pinMode(4, OUTPUT);

  /*
    // Your IP address with path or Domain name with URL path
    http.begin(serverName);

    // Send HTTP POST request
    int httpResponseCode = http.GET();

    String payload = "{}";

    if (httpResponseCode>0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
    }
    else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
    }
    // Free resources
    http.end();

    Serial.println(payload);
     JSONVar myObject = JSON.parse(payload);

     // JSON.typeof(jsonVar) can be used to get the type of the var
     if (JSON.typeof(myObject) == "undefined") {
       Serial.println("Parsing input failed!");
       return;
     }

     Serial.print("JSON object = ");
     Serial.println(myObject);

     // myObject.keys() can be used to get an array of all the keys in the object
     JSONVar keys = myObject.keys();

     for (int i = 0; i < keys.length(); i++) {
       JSONVar value = myObject[keys[i]];
       Serial.print(keys[i]);
       Serial.print(" = ");
       Serial.println(value);
       sensorReadingsArr[i] = double(value);
     }
     Serial.print("1 = ");
     Serial.println(sensorReadingsArr[0]);
     Serial.print("2 = ");
     Serial.println(sensorReadingsArr[1]);
     Serial.print("3 = ");
     Serial.println(sensorReadingsArr[2]);
  */
}

void
loop()
{
  bool isDetected = digitalRead(15);
  if (isDetected && millis() > movementDetectionAllowedTime) {
    movementDetectedFunc();
    digitalWrite(4, HIGH);
    movementDetectionAllowedTime = millis() + 10000;
    movementDetected = true;
    Serial.println("Presence detected");
    delay(100);
    digitalWrite(4, LOW);
  }

  server.handleClient();
}
