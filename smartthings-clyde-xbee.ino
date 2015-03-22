/*
 * Clyde XBee (SmartThings)
 * ------------
 *
 * Control Clyde's task and ambient lights via a XBee paired 
 * with SmartThings hub.
 *
 * This project uses a modified version of Andrew Rapp's XBee library.
 * --> https://github.com/kwasib/xbee-arduino
 *
 * March 18, 2015
 *
 */

#include <Wire.h>
#include <EEPROM.h>
#include <Clyde.h>
#include <SerialCommand.h>
#include <SoftwareSerial.h>
#include <MPR121.h>
#include <XBee.h>

XBee xbee = XBee();
XBeeResponse response = XBeeResponse();
// create response and command objects we expect to handle 
ZBExpRxResponse rx = ZBExpRxResponse();
XBeeAddress64 switchLongAddress;
uint16_t switchShortAddress;

double hue;
double saturation;
double brightness = 255.00;
            
void setup() {  
  Wire.begin();

  Serial.begin(9600);
  Serial1.begin(9600);
  xbee.setSerial(Serial1);
  
  Clyde.begin();
  Serial.println("started");
}

void loop() {
  //update the lights
  Clyde.updateAmbientLight();
  Clyde.updateWhiteLight();

    xbee.readPacket();

    if (xbee.getResponse().isAvailable()) {
    
      if (xbee.getResponse().getApiId() == ZB_EXPLICIT_RX_RESPONSE) {
        xbee.getResponse().getZBExpRxResponse(rx);
        
        uint16_t clusterId = (rx.getClusterId());
 
        if (clusterId == 0x13){
          Serial.println("*** Device Announce Message");
          switchLongAddress = rx.getRemoteAddress64();
          switchShortAddress = rx.getRemoteAddress16();
        }
        
        if (clusterId == 0x0006){ // On/Off Switch Message
          if (rx.getRFDataLength() == 3) {
            if (rx.getRFData()[2] == 0x01) {
              Clyde.setWhite(0); //On
            } else {
              Clyde.setWhite(255);
              Clyde.setAmbient(RGB(0,0,0));
            }
          } 
        } else if (clusterId == 0x0008){ // Level Control Messge
          int w = 255 - rx.getRFData()[3];
          
          Clyde.fadeWhite(w, 0.1f);
        } else if (clusterId == 0x0300){ // Color Control Message
          
          if (rx.getRFData()[2] == 0x00){ //Hue
           
            hue = rx.getRFData()[3];
            
            Serial.print("Hue:");
            Serial.println(hue);
            delay(2000);
          } else if (rx.getRFData()[2] == 0x03){ //Saturation           
           
            saturation = rx.getRFData()[3];

            RGB rgb;
            HSVToRgb(hue, saturation,brightness,rgb);
            Clyde.setAmbient(rgb); 

            Serial.print("Saturation:");
            Serial.println(saturation);
            Serial.println();
            Serial.println("RGB: ");
            Serial.println(rgb.r);
            Serial.println(rgb.g);
            Serial.println(rgb.b);         }
        }
      }
      else if (xbee.getResponse().getApiId() == ZB_TX_STATUS_RESPONSE) {
        ZBTxStatusResponse txStatus;
        xbee.getResponse().getZBTxStatusResponse(txStatus);
        
        Serial.print("Status Response: ");
        Serial.println(txStatus.getDeliveryStatus(), HEX);
        Serial.print("To Frame ID: ");
        Serial.println(txStatus.getFrameId());
      }
      else {
        Serial.print("Got frame type: ");
        Serial.println(xbee.getResponse().getApiId(), HEX);
      }
    }
    else if (xbee.getResponse().isError()) {
      Serial.println("***ERROR Reading Packet***");
      Serial.println(xbee.getResponse().getErrorCode(),DEC);
    }
}

/**
 * Converts an HSV color value to RGB. Assumes h, s,
 * and v are contained in the set [0, 255] and
 * returns r, g, and b in the set [0, 255].
 */
void HSVToRgb(double h, double s, double v, RGB& rgb) {
    double r, g, b;
    h /= 255;
    s /= 255;
    v /= 255;

    // Make sure our arguments stay in-range
    h = max(0, min(1.0, h));
    s = max(0, min(1.0, s));
    v = max(0, min(1.0, v));
        
    int i = int(h * 6);
    double f = h * 6 - i;
    double p = v * (1 - s);
    double q = v * (1 - f * s);
    double t = v * (1 - (1 - f) * s);

    switch(i % 6){
        case 0: r = v, g = t, b = p; break;
        case 1: r = q, g = v, b = p; break;
        case 2: r = p, g = v, b = t; break;
        case 3: r = p, g = q, b = v; break;
        case 4: r = t, g = p, b = v; break;
        case 5: r = v, g = p, b = q; break;
    }

    rgb.r = r * 255;
    rgb.g = g * 255;
    rgb.b = b * 255;
}
