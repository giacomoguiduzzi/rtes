var currentDelay = 500;
var chartT, chartH, chartP;
var temp_interval, hum_interval, press_interval, north_interval;

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

function getNorth(){
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange=function(){
        if(xhttp.readyState == 4 && this.status == 200){
            document.getElementById("north-direction").innerHTML =
            ("North direction: " + xhttp.responseText + "°");
        }
    }

    xhttp.open("GET", "/north", true);
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
    north_interval = setInterval(getNorth, currentDelay);
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
                notifyok();
                // change intervals
                clearInterval(temp_interval);
                temp_interval = setInterval(getTemperature, currentDelay);

                clearInterval(hum_interval);
                hum_interval = setInterval(getHumidity, currentDelay);

                clearInterval(press_interval);
                press_interval = setInterval(getPressure, currentDelay);

                clearInterval(north_interval);
                north_interval = setInterval(getNorth, currentDelay);
            }

            else {
                document.getElementById("delay-confirm").innerHTML = "There \
                was an error during the delay set-up.";
            }
        }
    };
    xhttp.open("GET", url, true);
    xhttp.send();
    console.log("Sent delay update request");
}

var delay = (function() {
    var timer = 0;
    return function(callback, ms) {
        clearTimeout(timer);
        timer = setTimeout(callback, ms);
    };
})();

function notifyok(){
    document.getElementById("delay-confirm").innerHTML = "Delay correctly set!";
    document.getElementById("delay-confirm").style.visibility = "visible";

    delay(function(){
        var op = 1;  // initial opacity
        var div = document.getElementById("delay-confirm");

        var timer = setInterval(function () {
            if (op <= 0.1){
                clearInterval(timer);
                div.style.visibility = 'none';
            }
            div.style.opacity = op;
            div.style.filter = 'alpha(opacity=' + op * 100 + ")";
            op -= op * 0.1;
        }, 50);
    }, 3000);
}
