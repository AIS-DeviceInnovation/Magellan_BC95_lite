![Library Version](https://img.shields.io/badge/Version-2.0.0-green)

# AIS Library
 
## AIS Magellan SDK for BC95 and BC95-G module Lite version
AIS Magellan Library, a software development kit used on arduino platform, have been developed for 
Magellan IoT Platform.  Support Arduino UNO.

### Example for use the Magellan SDK Lite
#### Call the Magellan library:
```cpp
#include "Magellan_BC95_lite.h"
Magellan_BC95_lite magel;
```
#### Initial Magellan Library:
```cpp
magel.begin();           //init Magellan LIB
```
#### Payload Data: 
Please use the payload in JSON format 

**Example**\
{"Temperature":25,"Humidity":90}

```cpp
payload="{\"Temperature\":"+Temperature+",\"Humidity\":"+Humidity+"}";
```
#### Report Data:
The example code report payload data to Magellan IoT Platform.
```cpp
magel.report(payload);
```
#### Example Magellan payload format
Please the location payload data as below format.\
**Example**
```cpp
payload={"Location":"Latitude,Longitude"}
```
Show battery on dashboard\
Battery is range in 0-100 %.\
**Example**
```cpp
payload={"Battery":100}   
```
Show Lamp status on dashbord\
please use 0 or 1 to send status\
**Example**
```cpp
payload={"Lamp":0} 
payload={"Lamp":1}
```
**Note** In this case, the device has already preload code then you just plug and play the development kit. If you have any questions, please see more details at https://www.facebook.com/AISDEVIO
