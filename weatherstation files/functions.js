var currentDelay = 1000;
var chartT, chartH, chartP;
var temp_interval, hum_interval, press_interval, north_interval, altitude_interval,
brightness_interval;

var light_theme;

function getTemperature(){
    var xhttp = new XMLHttpRequest();

    xhttp.onreadystatechange = function() {
        if (this.readyState === 4 && this.status === 200) {
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
        if (this.readyState === 4 && this.status === 200) {
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
}

function getPressure(){
    var xhttp = new XMLHttpRequest();

    xhttp.onreadystatechange = function() {
        if (this.readyState === 4 && this.status === 200) {
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
        if(xhttp.readyState === 4 && this.status === 200){
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
        if (xhttp.readyState === 4 && this.status === 200) {
            // Light intensity measurement range: 0.045lux-188000lux
            // Prendiamo la metà per riferimento
            if (xhttp.responseText < 94000) {
                light_theme = 1;
            } else {
                light_theme = 0;
            }

            //document.body.style.backgroundImage = "url('/images/sfondo.jpg')";
            document.getElementById("brightness").innerHTML =
                ("Brightness: " + xhttp.responseText + " lux");
        }
    }

    xhttp.open("GET", "/brightness", true);
    xhttp.send();
}

document.addEventListener("DOMContentLoaded", function(event) {
    chartT = new Highcharts.Chart({
        chart:{
            renderTo : 'chart-temperature',
            backgroundColor: 'rgba(0, 50, 173, 0.1)',
        },
        title: {
            text: 'Temperature',
        },
        series: [{
            showInLegend: false,
            }],
        plotOptions: {
          // Try this out with true!!!
            line: {
                animation: true,
                dataLabels: { enabled: true }
            },
            series: { color: '#AC1C1C' }
        },
        xAxis: {
            type: 'datetime',
            dateTimeLabelFormats: { second: '%H:%M:%S' }
        },
        yAxis: {
            title: { text: "Celsius (°C)" }
            //title: { text: 'Temperature (Fahrenheit)' }
        },

        credits: { enabled: false }
    });

    chartH = new Highcharts.Chart({
        chart:{
            renderTo:'chart-humidity',
            backgroundColor: 'rgba(0, 50, 173, 0.1)',
        },
        title: { text: 'Humidity' },
        series: [{
            showInLegend: false,
        }],
        plotOptions: {
            line: {
                animation: true,
                dataLabels: { enabled: true }
            },
            series: { color: '#485E5D' }
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
        chart:{
            renderTo:'chart-pressure',
            backgroundColor: 'rgba(0, 50, 173, 0.1)',
        },
        title: { text: 'Pressure' },
        series: [{
            showInLegend: false,
        }],
        plotOptions: {
            line: {
                animation: true,
                dataLabels: { enabled: true }
            },
            series: { color: '#18009c' }
        },
        xAxis: {
            type: 'datetime',
            dateTimeLabelFormats: { second: '%H:%M:%S' }
        },
        yAxis: {
            title: {
                text: "Pressure (Pa)"
            },
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

    if(delay_int === 0)
        return;

    else
        url += ("?delay=" + delay_int);

    console.log("Converted delay from string: " + delay_int);
    console.log("request URL: " + url);

    xhttp.onreadystatechange = function() {
        if (this.readyState === 4 && this.status === 200) {
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
                altitude_interval = setInterval(getAltitude, currentDelay);

                clearInterval(brightness_interval);
                brightness_interval = setInterval(getBrightness, currentDelay);
            }

            else
                console.log("error!");

            notifyresult(this.responseText);
        }
    };
    xhttp.open("GET", url, true);
    xhttp.send();
    console.log("Sent delay update request");
}

function sendUnits(){
    var tunit, punit;
    var xhttp = new XMLHttpRequest();
    var url = "/units";
    var tempUnit = document.getElementsByName('temp');
    var pressUnit = document.getElementsByName('press');

    for(i = 0; i < tempUnit.length; i++) {
        if(tempUnit[i].checked)
            tunit = tempUnit[i].value;
    }

    console.log("New Temp Unit: " + tunit);

    if(tunit.localeCompare('Celsius', 'en') == 0)
        url += '?temp=Celsius';
    else
        url += '?temp=Fahrenheit';

    for(i = 0; i < pressUnit.length; i++){
        if(pressUnit[i].checked)
            punit = pressUnit[i].value;
    }

    console.log("New Press Unit: " + punit);
    if(punit.localeCompare('Pascal', 'en') == 0)
        url += '&press=Pascal';

    else
        url += '&press=Bar';

    xhttp.onreadystatechange = function() {
        if (this.readyState === 4 && this.status === 200) {
            console.log("--- sendUnits received response: " + this.responseText);
            if(this.responseText == "ok"){
                console.log("Received ok response for units settings");

                switch(tunit){
                    case 'Celsius':
                        chartT.yAxis[0].update({
                            title: {
                                text: "Celsius (°C)"
                            }
                        });
                        break;

                    case 'Fahrenheit':
                        chartT.yAxis[0].update({
                            title: {
                                text: "Fahrenheit (°F)"
                            }
                        });
                        break;

                    default:
                        console.log("ERROR in setUnits temp switch-case.");
                        break;
                }

                switch(punit){
                        case 'Pascal':
                        chartP.yAxis[0].update({
                            title: {
                                text: "Pascal (Pa)"
                            }
                        });
                        break;

                    case 'Bar':
                        chartP.yAxis[0].update({
                            title: {
                                text: "Bar (bar)"
                            }
                        });
                        break;

                    default:
                        console.log("ERROR in setUnits pressure switch-case.");
                        break;
                }
            }

            else
                console.log("error!");

            notifyunits(this.responseText);
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

function notifyunits(result){
    var units_btn = document.getElementById("units-confirm");

    if(result == "ok")
        units_btn.classList.add("ok");
    else
        units_btn.classList.add("no");

    units_btn.style.opacity = "1";

    setTimeout(function() {
        units_btn.style.opacity = "0";
        if(result == "ok")
            units_btn.classList.remove("ok");
        else
            units_btn.classList.remove("no");
    }, 3000);
}
