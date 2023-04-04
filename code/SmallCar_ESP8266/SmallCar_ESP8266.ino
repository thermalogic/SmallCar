#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>

#define MOTOR_GO 1
#define MOTOR_STOP 2
#define MOTOR_BACK 3
#define MOTOR_LEFT 4
#define MOTOR_RIGHT 5
#define MOTOR_UP 6
#define MOTOR_DOWN 7

/* Set these to your desired credentials. */
const char *ssid = "ESP8266-WIFI";
const char *password = "12345678";

IPAddress ip(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

ESP8266WebServer server(80);

StaticJsonDocument<300> sensor_json;
String json;
bool data_ready;

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
<p><button class="button" onclick="send(1)">Go</button></p>
<p><button class="button" onclick="send(2)">STOP</button></p>
<p><button class="button" onclick="send(3)">BACK</button></p>
<p><button class="button"  onclick="send(4)">Turn Left</button></p>
<p><button class="button" onclick="send(5)">Turn Right</button></p>
<p><button class="button" onclick="send(6)">Speed Up</button></p>
<p><button class="button" onclick="send(7)">Speed Down</button></p>
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

void handleRoot()
{
  String s = webpage;
  server.send(200, "text/html", s);
}

void handleMotor()
{
  String state = "S";
  int motor_cmd = server.arg("state").toInt();
  switch (motor_cmd)
  {
  case MOTOR_GO:
    state = "G";
    break;
  case MOTOR_BACK:
    state = "B";
    break;
  case MOTOR_STOP:
    state = "S";
    break;
  case MOTOR_LEFT:
    state = "L";
    break;
  case MOTOR_RIGHT:
    state = "R";
    break;
  case MOTOR_UP:
    state = "U";
    break;
  case MOTOR_DOWN:
    state = "D";
    break;
  default:
    break;
  }
  Serial.println(state);
  server.send(200, "text/plane", state);
}

void handleSensor()
{
  if (data_ready)
  {
    server.send(200, "text/json", json);
  }
  else
  {
    server.send(503, "text/plane", "none data");
  }
}

void setup()
{
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

void loop()
{
  server.handleClient();
  data_ready = false;
  if (Serial.available())
  {
    DeserializationError err = deserializeJson(sensor_json, Serial);
    if (err == DeserializationError::Ok)
    {
      json = "{";
      json += "\"distance\":" + sensor_json["distance"].as<String>();
      json += ", \"left_speed\":" + sensor_json["left_speed"].as<String>();
      json += ", \"right_speed\":" + sensor_json["right_speed"].as<String>();
      json += "}";
      data_ready = true;
    }
    else
    { // Flush all bytes in the "link" serial port buffer
      while (Serial.available() > 0)
      {
        Serial.read();
      };
    }
  }
}
