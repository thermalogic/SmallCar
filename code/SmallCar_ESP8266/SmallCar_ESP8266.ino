/*
  Small Car: ESP8266-01
       by Cheng Maohua
*/
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>

const char *ssid = "ESP8266-WIFI";
const char *password = "12345678";

IPAddress ip(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

ESP8266WebServer server(80);

StaticJsonDocument<300> sensor_json;
String json;
bool data_ready = false;

const char webpage[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
   html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}
   .button { background-color: #195B6A; border: none; color: white; padding: 3px 10px;
            width:150px; height: 30px;text-decoration: none; font-size: 18px; margin: 2px; cursor: pointer;}
</style>
</head>
<title>Small Car</title>
<body>
<h1 align=center>Small Car</h1>
<p><button class="button" onclick="send('G')">Go</button></p>
<p><button class="button" onclick="send('S')">STOP</button></p>
<p><button class="button" onclick="send('B')">BACK</button></p>
<p><button class="button"  onclick="send('L')">Turn Left</button></p>
<p><button class="button" onclick="send('R')">Turn Right</button></p>
<p><button class="button" onclick="send('U')">Speed Up</button></p>
<p><button class="button" onclick="send('D')">Speed Down</button></p>
<div><h2> 
  Distance(cm): <span id="distance_val">0</span><br>
  </h2>
  <h3>Left Speed:&nbsp<span id="left_speed_val">0</span>
      &nbspRight Speed:&nbsp<span id="right_speed_val">0</span><br>
  </h3>
  <h2> Motor State: <span id="state">NA</span> </h2>
</div>
<script>
function send(motor_cmd) 
{
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("state").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "motor_set?state="+motor_cmd, true);
  xhttp.send();
}

setInterval(function() 
{
  getData();
}, 1000); 

function getData() {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var dataobj=JSON.parse(this.responseText);
      document.getElementById("distance_val").innerHTML = dataobj.distance;
      document.getElementById("left_speed_val").innerHTML = dataobj.left_speed;
      document.getElementById("right_speed_val").innerHTML = dataobj.right_speed;
    }
  };
  xhttp.open("GET", "data_read", true);
  xhttp.send();
}
</script>
</body>
</html>
)=====";

void handleRoot() {
  String s = webpage;
  server.send(200, "text/html", s);
}

void handleMotor() {
  String motor_cmd = server.arg("state");
  Serial.println(motor_cmd);
  server.send(200, "text/plane", motor_cmd);
}

void handleSensor() {
  if (data_ready) {
    server.send(200, "text/json", json);
  } else {
    server.send(503, "text/plane", "none data");
  }
}

void setup() {
  Serial.begin(9600);
  Serial.println();
  Serial.println("Configuring access point...");

  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(ip, gateway, subnet);

  server.on("/", handleRoot);
  server.on("/motor_set", handleMotor);
  server.on("/data_read", handleSensor);
  server.begin();

  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
  if (Serial.available()) {
    DeserializationError err = deserializeJson(sensor_json, Serial);
    if (err == DeserializationError::Ok) {
      json = "{";
      json += "\"distance\":" + sensor_json["distance"].as<String>();
      json += ", \"left_speed\":" + sensor_json["left_speed"].as<String>();
      json += ", \"right_speed\":" + sensor_json["right_speed"].as<String>();
      json += "}";
      data_ready = true;
    } else {  // Flush all bytes in the "link" serial port buffer
      while (Serial.available() > 0) {
        Serial.read();
      };
      data_ready = false;
    }
  }
}
