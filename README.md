# HB-CDL-6-Pinpad
ein Pinpad zur Einbindung in Homematic

Im Gegensatz zum HB-CDL-6 (https://github.com/trilu2000/HB-CDL-6) mit Touchscreen habe ich diesmal versucht nur Standardbauteile zu verwenden um einen einfachen Nachbau zu gewährleisten.
Herausgekommen ist dabei das HB-CDL-6-Pinpad.

![Gerätebild](/Pictures/pinpad-small.png)

Grundsätzlich verhält sich das Gerät wie ein 6 Fach Wandtaster, nur das die Tasten durch einen Pincode ersetzt werden.
Über die CCU Oberfläche hat man die Möglichkeit 12 verschedene Pin's den jeweiligen Kanälen zuzuordnen.

Selbst das Anlernen des Geräts wird über die Pin-Tastatur gestartet.

## Bauteile für den Nachbau
* HMSensor-CR2032 ohne CR2032 Batteriehalter https://asksinpp.de/Projekte/psi/HMSensor/#hmsensor-cr2032
* 3x4 Keypad https://www.ebay.de/itm/322734499078
* Batteriefedern https://www.ebay.de/itm/134133246378
* Batterie Federplatte https://www.ebay.de/itm/134128858997
* Gedruckte Gehäuseteile (im STL Unterordner) 

## Funktionsweise
Das Gerät hat 6 Schaltkanäle und kann 12 Pins verwalten. Die Einzelnen Pins können einem oder mehreren Kanälen zugeordnet werden.
Die Pineingabe folgt diesem Muster:
<Kanal> * <PIN> #

Kanal kann 1 bis 6 sein, es lassen sich im Programmcode aber auch Sonderkanäle für diverse Aktionen einbauen.
Derzeit gibt es nur einen Sonderkanal:
20 * Master PIN # startet das Pairing.

Die PIN's können bis zu 8 Stellen lang sein. Es besteht auch die Möglichkeit Kanäle einem leeren PIN zuzuweisen.
Eine mögliche Anwendung wäre das Schliessen der Garage.
Dazu würde man dann folgendes Muster eintippen:
<Kanal> * #
<Kanal> #
  
Pin 0 ist definiert als Master PIN. Dieser PIN ist im Programmcode definiert als: 
```
const uint8_t master_pwd[9] = { 0xff,'1','2','3','4','5','6','7','8', };
```
Der Master PIN kann beim kompilieren oder später über die CCU Oberfläche geändert werden.

Da es sich um ein Homebrew Gerät handelt, braucht es Jérôme's addon zur Einbindung in die CCU.

 https://github.com/jp112sdl/JP-HB-Devices-addon
 
## Pinpad

Es gibt verschiedene Versionen des Pinpads mit unterschiedlichen Pinbelegungen.

 Das von mir verwendete hat folgendes Pinout:

![Keypad Pinout](/Pictures/keypad-pinout.jpg)

Im Programmcode werden die Pins den Eingängen der HM Sensor Platine zugewiesen:
```
#define C2_PIN A0
#define R1_PIN A1
#define C1_PIN A2
#define R4_PIN A3
#define C3_PIN A4
#define R3_PIN A5
#define R2_PIN 3
```

## Bilder zum Aufbau

![Pinpad exploded view](/Pictures/exploded-view.png)


