#include <LDateTime.h>
#include <LGPS.h>
#include <LDateTime.h>
#include <LBattery.h>
#include <Wire.h>
#include <SeeedOLED.h>
#include "WatchIcons.h"

datetimeInfo t;
unsigned int seconds;
boolean gpsOn = false;
gpsSentenceInfoStruct info;
char format[60];
const int gpsOnPeriod=300;   //adjust this for how long in seconds to keep gps on when on battery
const int gpsOffPeriod=3600; //adjust this for how long in seconds to keep gps off when on battery

void setup() {
  Wire.begin();
  SeeedOled.init();
  SeeedOled.clearDisplay();
  LGPS.powerOn(); //turn on gps for first sync
  gpsOn = true;
}

void printTime() {
  char time_buffer[17];
  LDateTime.getTime(&t);
  sprintf(time_buffer,"%02d:%02d:%02d GMT",t.hour,t.min,t.sec);
  SeeedOled.setTextXY(0,0);
  SeeedOled.putString(time_buffer);
}

boolean printGPGGA(char* str, char* GPS_formatted)
{
    char latitude[20];
    char lat_direction[1];
    char longitude[20];
    char lon_direction[1];
    char buf[20];
    char time[30];
    const char* p = str;
    p = nextToken(p, 0); // GGA
    p = nextToken(p, time); // Time
    p = nextToken(p, latitude); // Latitude
    p = nextToken(p, lat_direction); // N or S?
    p = nextToken(p, longitude); // Longitude
    p = nextToken(p, lon_direction); // E or W?
    p = nextToken(p, buf); // fix quality
    if (buf[0] == '1')
    {
      // GPS fix
      p = nextToken(p, buf); // number of satellites
      SeeedOled.setTextXY(4,0);
      SeeedOled.putString(buf);
      int sats = atoi(buf);
      if (sats == 1)
      {
        SeeedOled.putString(" satellite     ");
      }
      else if (sats > 2 && sats < 10)
      {
        SeeedOled.putString(" satellites    ");        
      }
      else
      {
        SeeedOled.putString(" satellites   ");
      }
      
      const int coord_size = 11;
      char lat_fixed[coord_size],lon_fixed[coord_size];
      convertCoords(latitude,longitude,lat_fixed, lon_fixed,coord_size);
        
      SeeedOled.setTextXY(2,0);
      SeeedOled.putString(lat_fixed);
      SeeedOled.putString(lat_direction);

      SeeedOled.setTextXY(3,0);
      SeeedOled.putString(lon_fixed);
      SeeedOled.putString(lon_direction);

      return true;
    }
    else
    {
      SeeedOled.setTextXY(4,0);
      SeeedOled.putString("Acquiring GPS...");
      return false;
    }
}

void convertCoords(const char* latitude, const char* longitude, char* lat_return, char* lon_return, int buff_length)
{
    char lat_deg[3];
    strncpy(lat_deg,latitude,2);      //extract the first 2 chars to get the latitudinal degrees
    lat_deg[2] = 0;                   //null terminate
     
    char lon_deg[4];
    strncpy(lon_deg,longitude,3);      //extract first 3 chars to get the longitudinal degrees
    lon_deg[3] = 0;                    //null terminate
     
    int lat_deg_int = arrayToInt(lat_deg);    //convert to integer from char array
    int lon_deg_int = arrayToInt(lon_deg);
     
    // must now take remainder/60
    //this is to convert from degrees-mins-secs to decimal degrees
    // so the coordinates are "google mappable"
     
    float latitude_float = arrayToFloat(latitude);      //convert the entire degrees-mins-secs coordinates into a float - this is for easier manipulation later
    float longitude_float = arrayToFloat(longitude);
     
    latitude_float = latitude_float - (lat_deg_int*100);      //remove the degrees part of the coordinates - so we are left with only minutes-seconds part of the coordinates
    longitude_float = longitude_float - (lon_deg_int*100);
     
    latitude_float /=60;                                    //convert minutes-seconds to decimal
    longitude_float/=60;
     
    latitude_float += lat_deg_int;                          //add back on the degrees part, so it is decimal degrees
    longitude_float+= lon_deg_int;
     
    snprintf(lat_return,buff_length,"%3.6f",latitude_float);    //format the coordinates nicely - no more than 6 decimal places
    snprintf(lon_return,buff_length,"%3.6f",longitude_float);
}
 
int arrayToInt(const char* char_array)
{
    int temp;
    sscanf(char_array,"%d",&temp);
    return temp;
}
 
float arrayToFloat(const char* char_array)
{
    float temp;
    sscanf(char_array, "%f", &temp);
    return temp;
}
 
const char *nextToken(const char* src, char* buf)
{
    int i = 0;
    while (src[i] != 0 && src[i] != ',')
    i++;
    if (buf)
    {
        strncpy(buf, src, i);
        buf[i] = 0;
    }
    if (src[i])
    i++;
    return src + i;
}

void displayIcons()
{
  SeeedOled.setTextXY(7,0);
  switch (LBattery.level()) {
    case 0:
      SeeedOled.drawBitmap(BatteryEmpty,16);
      break;
    case 33:
      SeeedOled.drawBitmap(Battery33,16);
      break;
    case 66:
      SeeedOled.drawBitmap(Battery66,16);
      break;
    case 100:
      SeeedOled.drawBitmap(BatteryFull,16);
      break;
    default: 
      SeeedOled.drawBitmap(BigSpace,16);
    break;
  }
  SeeedOled.drawBitmap(LittleSpace,4);
  if (LBattery.isCharging()) {
    SeeedOled.drawBitmap(BatteryCharging,16);
  }
  else {
    SeeedOled.drawBitmap(BigSpace,16);
  }
}

void loop() {
  printTime();
  displayIcons();
  if (gpsOn)
  {
    LGPS.getData(&info);
    printGPGGA((char*)info.GPGGA,format);
    if (seconds > gpsOnPeriod)
    {
      seconds = 0;
      if (LBattery.isCharging() == false)
      {
        LGPS.powerOff();
        gpsOn = false;
        SeeedOled.setTextXY(4,0);
        SeeedOled.putString("GPS powered off ");
      }
    }
  }
  else
  {
    if (seconds > gpsOffPeriod)
    {
      seconds = 0;
      LGPS.powerOn();
      gpsOn = true;
    }
  }
  seconds++;
  
  delay(1000);
}
