/*------------------------------------------------------------------------------
  07/01/2018
  Author: Makerbro
  Platforms: ESP8266
  Language: C++/Arduino
  File: webserver_html_js.ino
  ------------------------------------------------------------------------------
  Description: 
  Code for YouTube video demonstrating how to use JavaScript in HTML weppages
  that are served in a web server's response.
  https://youtu.be/ZJoBy2c1dPk

  Do you like my videos? You can support the channel:
  https://patreon.com/acrobotic
  https://paypal.me/acrobotic
  ------------------------------------------------------------------------------
  Please consider buying products from ACROBOTIC to help fund future
  Open-Source projects like this! We'll always put our best effort in every
  project, and release all our design files and code for you to use. 

  https://acrobotic.com/
  https://amazon.com/acrobotic
  ------------------------------------------------------------------------------
  License:
  Please see attached LICENSE.txt file for details.
------------------------------------------------------------------------------*/
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Wire.h>

char index_html[] PROGMEM = R"=====(
<!DOCTYPE HTML>
<html>
<!-- Rui Santos - Complete project details at https://RandomNerdTutorials.com

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files.
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software. -->
    <head>
        <meta charset="utf8">
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <script src="https://code.highcharts.com/highcharts.js"></script>
        <script language="JavaScript" type="text/JavaScript" src="functions.js"></script>

        <style>
            h2 {
                font-size: 2.5rem;
                text-align: center;
            }

            #delay-confirm{
                opacity: 0;
                transition: .5s all ease;
                display: inline-block;
                border-radius:5px;
                display: inline-block;
                padding-top: 5px;
                padding-bottom: 5px;
                border: none;
            }

            .ok{
                color: white;
                background-color: green;
            }

            .no{
                color: black;
                background-color: red;
            }

            /* #delay-confirm span{
                padding: 10px;
            } */

            fieldset{
                border-radius: 5px;
                border: solid;
                border-color: black;
            }

            fieldset span, #delay-confirm{
                display: block;
                margin-top: 5px;
                margin-bottom: 5px;
            }

        </style>
    </head>
    <body>
      <h2>STM32L475VG + ESP8266 Weather Station</h2>
      <div id="chart-temperature" class="container"></div>
      <div id="chart-humidity" class="container"></div>
      <div id="chart-pressure" class="container"></div>
      <div id="altitude" class="container">0m</div>
      <div id="brightness" class="container">0 lux</div>

      <fieldset>
          <legend>Sensors refresh rate</legend>
          <span>
              <input type="radio" name="delay" value="Fast" checked="checked" />Fast
          </span>
          <span>
              <input type="radio" name="delay" value="Medium" />Medium
          </span>
          <span>
              <input type="radio" name="delay" value="Slow" />Slow
          </span>
          <span>
              <input type="radio" name="delay" value="Very slow" />Very slow
          </span>
          <span>
              <input type="radio" name="delay" value="Take a break" />Take a break
          </span>

          <button onclick="sendDelay()"> Set refresh rate</button>
          <button id="delay-confirm">Delay correctly set!</button>
      </fieldset>
    </body>
</html>
)=====";

char functions_js[] PROGMEM = R"=====(
var currentDelay = 500;
var chartT, chartH, chartP;
var temp_interval, hum_interval, press_interval, altitude_interval,
brightness_interval;

function getTemperature(){
    var xhttp = new XMLHttpRequest();

    xhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
            var x = (new Date()).getTime(),
            y = parseFloat(this.responseText);
            console.log("New temperature value: " + y);
            //console.log(this.responseText);
            if(chartT.series[0].data.length > 40) {
                chartT.series[0].addPoint([x, y], true, true, true);
            } else {
                chartT.series[0].addPoint([x, y], true, false, true);
            }
        }
    };

    xhttp.open("GET", "/temperature", true);
    xhttp.send();
    console.log("Sent new temperature value request");
}

function getHumidity(){
    var xhttp = new XMLHttpRequest();

    xhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
            console.log("Getting new humidity value in getHumidity() and plotting");
            var x = (new Date()).getTime(),
            y = parseFloat(this.responseText);
            console.log("New humidity value: " + y);
            //console.log(this.responseText);
            if(chartH.series[0].data.length > 40) {
                chartH.series[0].addPoint([x, y], true, true, true);
            } else {
                chartH.series[0].addPoint([x, y], true, false, true);
            }
        }
    };

    xhttp.open("GET", "/humidity", true);
    xhttp.send();
    console.log("Sent new humidity value request");
};

function getPressure(){
    var xhttp = new XMLHttpRequest();

    xhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
            var x = (new Date()).getTime(),
            y = parseFloat(this.responseText);
            console.log("New pressure value: " + y);
            //console.log(this.responseText);
            if(chartP.series[0].data.length > 40) {
                chartP.series[0].addPoint([x, y], true, true, true);
            } else {
                chartP.series[0].addPoint([x, y], true, false, true);
            }
        }
    };

    xhttp.open("GET", "/pressure", true);
    xhttp.send();
}

function getAltitude(){
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange=function(){
        if(xhttp.readyState == 4 && this.status == 200){
            document.getElementById("altitude").innerHTML =
            ("Altitude: " + xhttp.responseText + "m");
        }
    }

    xhttp.open("GET", "/altitude", true);
    xhttp.send();
}

function getBrightness(){
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange=function(){
        if(xhttp.readyState == 4 && this.status == 200){
            document.getElementById("brightness").innerHTML =
            ("Brightness: " + xhttp.responseText + " lux");
        }
    }

    xhttp.open("GET", "/brightness", true);
    xhttp.send();
}

document.addEventListener("DOMContentLoaded", function(event) {
    chartT = new Highcharts.Chart({
      chart:{ renderTo : 'chart-temperature' },
      title: { text: 'Temperature' },
      series: [{
        showInLegend: false,
        data: []
      }],
      plotOptions: {
          // Try this out with true!!!
        line: { animation: false,
          dataLabels: { enabled: true }
        },
        series: { color: '#059e8a' }
      },
      xAxis: { type: 'datetime',
        dateTimeLabelFormats: { second: '%H:%M:%S' }
      },
      yAxis: {
        title: { text: 'Temperature (°C)' }
        //title: { text: 'Temperature (Fahrenheit)' }
      },
      credits: { enabled: false }
    });

    chartH = new Highcharts.Chart({
      chart:{ renderTo:'chart-humidity' },
      title: { text: 'Humidity' },
      series: [{
        showInLegend: false,
        data: []
      }],
      plotOptions: {
        line: { animation: false,
          dataLabels: { enabled: true }
        }
      },
      xAxis: {
        type: 'datetime',
        dateTimeLabelFormats: { second: '%H:%M:%S' }
      },
      yAxis: {
        title: { text: 'Humidity (%)' }
      },
      credits: { enabled: false }
    });

    chartP = new Highcharts.Chart({
      chart:{ renderTo:'chart-pressure' },
      title: { text: 'Pressure' },
      series: [{
        showInLegend: false,
        data: []
      }],
      plotOptions: {
        line: { animation: false,
          dataLabels: { enabled: true }
        },
        series: { color: '#18009c' }
      },
      xAxis: {
        type: 'datetime',
        dateTimeLabelFormats: { second: '%H:%M:%S' }
      },
      yAxis: {
        title: { text: 'Pressure (hPa)' }
      },
      credits: { enabled: false }
    });

    temp_interval = setInterval(getTemperature, currentDelay);
    hum_interval = setInterval(getHumidity, currentDelay);
    press_interval = setInterval(getPressure, currentDelay);
    altitude_interval = setInterval(getAltitude, currentDelay);
    brightness_interval = setInterval(getBrightness, currentDelay);
});

function sendDelay(){
    var delay;
    var delay_int = 0;
    var xhttp = new XMLHttpRequest();
    var url = "/delay";
    var radiobuttons = document.getElementsByName('delay');

    for(i = 0; i < radiobuttons.length; i++) {
        if(radiobuttons[i].checked)
            delay = radiobuttons[i].value;
    }

    console.log("DELAY VALUE: " + delay);

    switch(delay){
        case "Fast":
            delay_int = 500;
            break;

        case "Medium":
            delay_int = 1000;
            break;

        case "Slow":
            delay_int = 2500;
            break;

        case "Very slow":
            delay_int = 5000;
            break;

        case "Take a break":
            delay_int = 10000;
            break;

        default:
            console.log("There was a problem during the delay switch-case.");
            break;
    }

    if(delay_int == 0)
        return;

    else
        url += ("?delay=" + delay_int);

    console.log("Converted delay from string: " + delay_int);
    console.log("request URL: " + url);

    xhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
            console.log("--- SETDELAY received response: " + this.responseText);
            if(this.responseText == "ok"){
                console.log("Received ok response for delay");
                currentDelay = delay_int;

                // change intervals
                clearInterval(temp_interval);
                temp_interval = setInterval(getTemperature, currentDelay);

                clearInterval(hum_interval);
                hum_interval = setInterval(getHumidity, currentDelay);

                clearInterval(press_interval);
                press_interval = setInterval(getPressure, currentDelay);

                clearInterval(altitude_interval);
                north_interval = setInterval(getAltitude, currentDelay);

                clearInterval(brightness_interval);
                north_interval = setInterval(getBrightness, currentDelay);
            }

            else {
                console.log("error!");
                document.getElementById("delay-confirm").innerHTML = "There \
                was an error during the delay set-up.";
            }

            notifyresult(this.responseText);
        }
    };
    xhttp.open("GET", url, true);
    xhttp.send();
    console.log("Sent delay update request");
}

function notifyresult(result){
    var delay_btn = document.getElementById("delay-confirm");

    if(result == "ok")
        delay_btn.classList.add("ok");
    else
        delay_btn.classList.add("no");

    delay_btn.style.opacity = "1";

    setTimeout(function() {
        delay_btn.style.opacity = "0";
        if(result == "ok")
            delay_btn.classList.remove("ok");
        else
            delay_btn.classList.remove("no");
    }, 3000);
}
)=====";

#define DELAY_FLAG 0x64 // "d" letter
#define I2C_ADDR 0x33

typedef struct {
  float temperature;
  float pressure;
  float humidity;
  float altitude;
  float brightness;
} sensors_data_t;

typedef enum
{
  FAST = 500,
  MEDIUM = 1000,
  SLOW = 2500,
  VERY_SLOW = 5000,
  TAKE_A_BREAK = 10000
} sensors_delay_t;

ESP8266WebServer server(80);
const char* ssid = "Vodafone - Packets Are Coming";
const char* password = "Arouteroficeandfire96!";

uint16_t sensors_delay = FAST;
uint16_t old_sensors_delay = FAST;

sensors_data_t *sensors_data;
uint8_t *sent_data;
bool updating_struct, using_delay;

unsigned long start_time, end_time, loop_delay;

char *getTemperature() {
  char *temp = (char *)malloc(sizeof(char) * 5);
  // active wait, can't use delay() here
  //while(!digitalRead(SSPIN));
  while (updating_struct);
  sprintf(temp, "%.02f", sensors_data->temperature);
  // Serial.print("Sending temperature value to client: ");
  // Serial.println(String(temp));
  return temp;
}

char *getHumidity() {
  char *hum = (char *)malloc(sizeof(char) * 5);
  // active wait, can't use delay() here
  // while(!digitalRead(SSPIN));
  while (updating_struct);
  sprintf(hum, "%.02f", sensors_data->humidity);
  // Serial.print("Sending humidity value to client: ");
  // Serial.println(String(hum));
  return hum;
}

char *getPressure() {
  char *pres = (char *)malloc(sizeof(char) * 5);
  // active wait, can't use delay() here
  // while(!digitalRead(SSPIN));
  while (updating_struct);
  sprintf(pres, "%.02f", sensors_data->pressure);
  // Serial.print("Sending pressure value to client: ");
  //Serial.println(String(pres));
  return pres;
}

char *getAltitude() {
  char *alt = (char *)malloc(sizeof(char) * 5);
  // active wait, can't use delay() here
  // while(!digitalRead(SSPIN));
  while (updating_struct);
  sprintf(alt, "%.02f", sensors_data->altitude);
  // Serial.print("Sending north direction value to client: ");
  // Serial.println(String(n_dir));
  return alt;
}

char *getBrightness() {
  char *brightn = (char *)malloc(sizeof(char) * 5);
  // active wait, can't use delay() here
  // while(!digitalRead(SSPIN));
  while (updating_struct);
  sprintf(brightn, "%.02f", sensors_data->brightness);
  // Serial.print("Sending north direction value to client: ");
  // Serial.println(String(n_dir));
  return brightn;
}

char *getDelay(){
  char *current_delay;

  switch(sensors_delay){
    case FAST:
      current_delay = (char *)malloc(sizeof(char) * 4);
      break;

    case MEDIUM:
    case SLOW:
    case VERY_SLOW:
      current_delay = (char *)malloc(sizeof(char) * 5);
      break;

    case TAKE_A_BREAK:
      current_delay = (char *)malloc(sizeof(char) * 6);
      break;

    default:
      Serial.println("Something wrong is getDelay function.");
      current_delay = (char *)malloc(sizeof(char) * 6);
      sprintf(current_delay, "error");
    break;
  }

  sprintf(current_delay, "%u", sensors_delay);

  return current_delay;
}

bool set_new_delay(const uint16_t new_delay) {
  bool ok = true;

  switch (new_delay) {
    case 500:
      sensors_delay = FAST;
      break;
    case 1000:
      sensors_delay = MEDIUM;
      break;
    case 2500:
      sensors_delay = SLOW;
      break;
    case 5000:
      sensors_delay = VERY_SLOW;
      break;
    case 10000:
      sensors_delay = TAKE_A_BREAK;
      break;
    default:
      Serial.print("There was an error while setting new delay: ");
      Serial.println(new_delay);
      ok = false;
      break;
  }

  return ok;
}

char *setDelay(String delay_) {
  char *result = (char *)malloc(sizeof(char) * 3);
  uint16_t new_delay = strtol(delay_.c_str(), NULL, DEC);

  Serial.print("New delay in setDelay(): ");
  Serial.println(new_delay);

  if(new_delay == sensors_delay)
    sprintf(result, "ok");
  else
  {
    old_sensors_delay = sensors_delay;
    bool answer = set_new_delay(new_delay);
  
    if (answer) {
      sprintf(result, "ok");
    }
    else
      sprintf(result, "no");
  
    Serial.print("Sending back response \"");
    Serial.print(result);
    Serial.println("\" to client.");
  }

  return result;
}

void sendNewDelay() {
  Serial.println("Sending new delay to Arduino Due.");
  using_delay = true;
  Wire.write(sensors_delay);
  using_delay = false;
}

void readSensorData(int bytes_to_read) {
  Serial.println("Receiving data from Arduino Due.");
  
  uint8_t struct_size = sizeof(sensors_data_t);

  if(bytes_to_read == 4){
    using_delay = true;
    
    uint8_t delay_flag = Wire.read();

    if(delay_flag == DELAY_FLAG){
      char *answer = (char *)malloc(sizeof(char) * 3);
  
      for(uint8_t i = 0; i < 3; i++)
        answer[i] = Wire.read();
  
      if(strcmp(answer, "ok\0") == 0){
        Serial.println("New delay confirmed and set up.");
        old_sensors_delay = sensors_delay;
      }
  
      else if(strcmp(answer, "no\0") == 0){
        Serial.println("Something went wrong with the new delay, received a \"no\" answer. Going back");
        sensors_delay = old_sensors_delay;
      }
  
      else{
        Serial.println("Received answer is neither yes nor no. What now?");
        // Assuming no
        sensors_delay = old_sensors_delay;
      }
    }
    else{
      Serial.println("Waiting for a new-delay-set answer, the first byte sent as an answer is not the DELAY_FLAG. Aborting change");
      sensors_delay = old_sensors_delay;  
    }

    using_delay = false;
  }
  else{
    updating_struct = true;
    
    Serial.println("Receiving new data struct.");
  
    if (bytes_to_read != struct_size) {
      Serial.println("Sent data structure size doesn't correspond, thus it hasn't been read.");
      return;
    }
  
    for (uint8_t i = 0; i < sizeof(sensors_data); i++) {
      sent_data[i] = Wire.read();
    }
  
    updating_struct = false;
  
    Serial.print("Temperature: ");
    Serial.print(sensors_data->temperature);
    Serial.println("°C");
  
    Serial.print("(HEX: ");
    Serial.print(sent_data[0], HEX);
    Serial.print(sent_data[1], HEX);
    Serial.print(sent_data[2], HEX);
    Serial.print(sent_data[3], HEX);
    Serial.println(")");
  
    Serial.print("Pressure: ");
    Serial.print(sensors_data->pressure);
    Serial.println(" hPa");
  
    Serial.print("Humidity: ");
    Serial.print(sensors_data->humidity);
    Serial.println("%");
  
    Serial.print("Altitude: ");
    Serial.print(sensors_data->altitude);
    Serial.println("m");
  
    Serial.print("Brightness: ");
    Serial.print(sensors_data->brightness);
    Serial.println(" lux");
    Serial.println();
  }
}


void setup()
{
  Wire.begin(I2C_ADDR);
  Wire.onReceive(readSensorData);
  Wire.onRequest(sendNewDelay);
  
  // pinMode(pin_led, OUTPUT);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  Serial.begin(9600);
  while(!Serial);

  updating_struct = using_delay = false;

  sensors_data = (sensors_data_t *)malloc(sizeof(sensors_data));
  sensors_data->temperature = 0.0;
  sensors_data->humidity = 0.0;
  sensors_data->pressure = 0.0;
  sensors_data->altitude = 0.0;
  sensors_data->brightness = 0.0;

  sent_data = (uint8_t *)sensors_data;

  start_time = end_time = loop_delay = 0;
  
  while(WiFi.status()!=WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  Serial.println("");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  if(MDNS.begin("weatherstation")){
    Serial.println("DNS okay! You can connect to http://weatherstation.local");
  }

  server.on("/", [](){
    server.send_P(200,"text/html", index_html);
  });
  
  server.on("/functions.js", [](){
    server.send_P(200,"text/javascript", functions_js);  
  });
  
  server.on("/temperature", [](){
    char *temp = getTemperature();
    server.send_P(200, "text/plain", temp);
    free(temp);
  });
  server.on("/humidity", [](){
    char *hum = getHumidity();
    server.send_P(200, "text/plain", hum);
    free(hum);
  });
  server.on("/pressure", []() {
    char *pres = getPressure();
    server.send_P(200, "text/plain", pres);
    free(pres);
  });

  server.on("/altitude", [](){
    char *alt = getAltitude();
    server.send_P(200, "text/plain", alt);
    free(alt);
  });

  server.on("/brightness", []() {
    char *brightness = getBrightness();
    server.send_P(200, "text/plain", brightness);
    free(brightness);
  });

  /* server.on("/delay", [](AsyncWebServerRequest * request) {
    if (request->hasParam("delay")) {
      AsyncWebParameter *p = request->getParam("delay");
      Serial.print("Received new delay request with argument: ");
      Serial.println(p->value());
      char * ok = setDelay(p->value());
      request->send_P(200, "text/plain", ok);
      free(ok);
    }
  });*/

  server.on("/getdelay", []() {
    char *current_delay = getDelay();
    server.send_P(200, "text/plain", current_delay);
    free(current_delay);
  });
  
  server.begin();
}

void loop()
{
  server.handleClient();
  MDNS.update();
}

/*void sendHomePage(){
  
  server.send_P(200,"text/html", "");
}

void toggleLED()
{
  digitalWrite(pin_led,!digitalRead(pin_led));
}

void getLEDState()
{
  toggleLED();
  String led_state = digitalRead(pin_led) ? "OFF" : "ON";
  server.send(200,"text/plain", led_state);
}*/
