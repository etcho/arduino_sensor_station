# arduino_sensor_station
A box with sensors for measuring stuff.

All the information are displayed on a 20x4 lcd display. It has a GPS module, a DHT22 and a barometer (BMP085). It provides:
- Temperature (DHT22)
- Air humidity (DHT22)
- Temperature (BMP085)
- Altitude (BMP085)
- Pressure (BMP085)
- Real time clock (GPS)
- Number of satellites (GPS)
- Altitude (GPS)
- Speed (GPS)
- Compass when in movement (GPS)

There is a button to set/unset the zero for altitude, for easy altitude measuring. It's also displayed the current change of altitude level, showing and arrow up or down if the box is ascending or descending.
