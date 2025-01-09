# Arduino-Splunk-radar
Use of motion sensors with Arduino boards that transmit their data to Splunk for use as section radar data to measure vehicle speed.

# Prerequisites

## Hardware

- 2 x Arduino R4 Wifi

![Arduino R4](https://www.algosecure.fr/blog/img/2024-11-20_02-arduino_r4_wifi.png)

- at least 6 x connector wires

![Wires](https://www.algosecure.fr/blog/img/2024-11-20_03-fils.jpg)

- 2 x HC-SR501 sensors

![HC-SR501](https://external-content.duckduckgo.com/iu/?u=https%3A%2F%2Fwww.javanelec.com%2FCustomAjax%2FGetAppDocument%2Fdb88d946-6fec-4899-b6b3-3fbb62937411%3Ftype%3D2&f=1&nofb=1&ipt=ac269f08d3ad4bc7a556ff21d047584fb203a28ebdbd4d480f01b03de9f31c40&ipo=images)

- 1 x Splunk server that meets the editor hardware requirements

- 1 x WiFi network with /24 netmask or more (_necessary to have unique sensor host values_)

## Software

- Splunk :)
- Arduino IDE 

# Splunk configuration

## Index

Create a destination index for the data (through `indexes.conf` file in CLI or _Settings_ > _Indexes_ in WebUI)

![Index](https://www.algosecure.fr/blog/img/2024-11-20_07-index.png)

## HTTP Event Collector

Create an HTTP Event Collector input to receive data (through `inputs.conf` file in CLI or _Settings_ > _Data Inputs_ in WebUI)

> [!IMPORTANT]
> **Note the destination sourcetype as it will be configured later for time extraction**

![HEC](https://www.algosecure.fr/blog/img/2024-11-20_08-inputs.png)

> [!IMPORTANT]
> **Note the token that is generated as it will be used in the Arduino code to authenticate !**
> Otherwise no worry, it can be retreived in `inputs.conf` or _Settings_ > _Data Inputs_ in WebUI

## Data time extraction

Configure following settings in `props.conf` and `transforms.conf` files with correct permissions.

> <ins>_props.conf_</ins> :
```bash
[<your_sourcetype>]
category = Custom
pulldown_type = 1
TRANSFORMS-arduino-r4 = set-arduino-time
```

> <ins>_trasnforms.conf_</ins> :
```bash
[set-arduino-time]
INGEST_EVAL = _time=strptime(mvindex(split(_raw," -"),0),"%d-%m-%Y %H:%M:%S.%3Q")
```

> [!NOTE]
> Don't forget to set metadata if necessary and restart Splunk service or make a debug/refresh to apply changes.

# Connect each sensor to a different Arduino

![Connections](https://www.algosecure.fr/blog/img/2024-11-20_05-broches.png)

| Sensor pin | Arduino pin |
|---|---|
|VCC|5V|
|OUT|D2 (your choice, need to be changed in code if necessary)|
|GND|GND|

![Connections2](https://www.algosecure.fr/blog/img/2024-11-20_06-branchements.jpg)

# Upload code

Create a new project in Arduino IDE with the two files of this repo : `arduino_r4_radar.ino` and `arduino_secrets.sh`.

Set the values in `arduino_secrets.h` for your environment 

Adapt if necessary the following values in `arduino_r4_radar.ino` :
- sensorPin (_corresponding to the Arduino pin on which sensor is connected for OUT sensor pin_)
- port (_for Splunk HEC port if you changed it, 8088 by default_)
- setServer (_for NTP server_)
- Timezone name (**_don't forget to change also it anywhere else in the file !_**)
- your timezone in the line with `setLocation` command
  
Connect each Arduino board one by one and upload the code on it.

# Results

The following results should be displayed on Arduino serial monitor if all settings and wirings are correct and if a motion is detected by the sensor :

![serial_results](https://www.algosecure.fr/blog/img/2024-11-20_09-resultats_carte.jpg)

Then, if you search your data in Splunk for the corresponding index and sourcetype, you should get these kind of data with the correct timestamp :

![splunk_results](https://www.algosecure.fr/blog/img/2024-11-20_10-resultats_splunk.jpg)

Congrats :+1: ! You can then Splunk it and use these data to calculate the speed and make dashboards on it !

> [!TIP]
> **_Example of SPL request_** (_specific to my environment but in case this can help you to begin_) :

```bash
index=arduino
| eval position=if(host="sensor163","L","R")
| transaction maxevents=2 maxpause=2 mvlist=true keeporphans=false maxspan=1s
| where mvindex(host,0)!=mvindex(host,1)
| table _time,_raw,duration,host,position
| eval distance=3.4, direction=if(mvindex(position,0)="L","out","in")
| eval ms=distance/duration
| eval kmh=round(ms*3.6,0)
| where kmh<75 AND kmh>15
| eval kmh=kmh-5
| eval result=if(kmh-30<=0,0,kmh-30)
| eval fine=if(result>0,135,0)
| eval license_points=case(result<5,0,result>=5 AND result<20,1,result>=20 AND result<30,2,result>=30 AND result<40,3,result>=40 AND result<50,4,true(),0)
| eval result=case(result>0 AND result<5,"Less than 5 km/h",result>=5 AND result<20,"Beetween 5 and 19 km/h",result>=20 AND result<30,"Beetween 20 and 29 km/h",result>=30 AND result<40,"Beetween 30 and 39 km/h",result>=40 AND result<50,"Beetween 40 and 49 km/h",true(),"OK")
| table _time, direction, kmh, result, fine, license_points
```



