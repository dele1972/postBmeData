# PostBmeData (Solar-Wetterstationsprojekt)

## ToDo

- beim Aufbau die `Dele72.h` beschreiben
  - welche Variablen sind dort hinterlegt?
  - wie kommt man zu den Werten?
- prototyp Schaltung auf einer Lochrasterplatine löten
- Gehäuse mit FreeCad erstellen
  - Temperaturfühler auslagern?
  - Solarpanel als Deckel oder (auch) auslagern?
- Gehäuse drucken

## Zum Projekt

### Ziele

- Auf der Terrasse (und später an anderen Outdoor Locations am Haus) sollen Sensoren platziert werden
  - gesteuert über ESP-8266 Module (12f)
  - W-LAN Anbindung
    - soll aus gegebenen Konfigurationen die mit der stärksten Signalquelle verwenden
  - 'autarker' Strom durch Kombination von Solar und Akku
- Daten sollen an eine Datenbank (bei meinem Webhoster/Domain) gespeichert werden
  - Wetterdaten
    - Temperatur und Luftdruck
      - später auch Bodenfeuchte, Niederschlagmenge, Windstärke, Windrichtung, Helligkeit (Tag/Nacht-Erkennung, Bewölkungsgrad)
    - Stromdaten
      - aktuelle Spannung aus Solarmodul
      - aktuelle Spannung aus Akku (Ladelevel?)
    - W-LAN Daten
      - verwendete SSID
      - Signalstärke

### Aufbau

- Für die Verbindungsdaten habe ich eine eigene Library erstellt (`Dele72.h`)

### Probleme/Auffälligkeiten

- Das Solarmodul liefert im Peak ~6V, damit mehr Strom als gespeichert/verbraucht werden kann
  - jedenfalls nach der aktuellen Schaltung

## Teile

- Solarpanel *SM6150*
  - Hersteller: [Sol-Expert group](https://www.sol-expert-group.de/)
  - U=6V
  - I=150mA
  - Maße
    - 11,9cm
    - 5,9cm
- Akku
  - LiFePo4
  - 18650
    - [Diskussion 26650 Batterie gegen 18650 Batterie](https://de.large.net/news/8bu43pg.html)
  - 3,3V
  - 3300mAh(?) 
- BAT43 Schottky Diode
- LM317-220 Spannungsregler
  - [reichelt](https://www.reichelt.de/de/de/spannungsregler-einstellbar-1-2--37-v-to-220-lm-317-220-sg-p120703.html?PROVID=2788&gclid=Cj0KCQjwxIOXBhCrARIsAL1QFCZ3Ao9AW30IVLPCD0Lk1vRn-jrn7CNYoFOq6578vglwBVS0JN1fmZ0aAiRuEALw_wcB&&r=1)
- fadushin/[solar-esp32](https://github.com/fadushin/solar-esp32)
- Kicad Symbols
  - ADS115: SnapEDA [1085](https://www.snapeda.com/parts/1085/Adafruit%20Industries%20LLC/view-part/)

### Werte der Wiederstände

- **Spannungsteiler**
  - R8 = 220K (rot, rot, schwarz, orange, braun)
  - R9 = 100K (braun, schwarz, schwarz, orange, braun)

- **LM317 Konfiguration**, Uout = 4,114V
  [LM317-Rechner](http://netzmafia.de/skripten/hardware/LM317/LM317.html)
  - "R1" = 527
    - R1 = 470 (gelb, lila, schwarz, schwarz, braun)
    - R2 = 47 (gelb, lila, schwarz, gold, braun)
    - R3 = 10 (braun, schwarz, schwarz, gold, braun)
  - "R2" = 230
    - R5 = 220 (rot, rot, schwarz, schwarz, braun)
    - R4 = 20 (rot, schwarz, schwarz, gold, braun)

- **ESP Handling**
  - Einschalten
    - R6 = 10K (braun, schwarz, schwarz, rot, braun)
  - Modus
    - R7 = 10K (braun, schwarz, schwarz, rot, braun)

## Links

- [meine Testseite zur Anzeige der gesendeten Daten](https://data.lederich.de/home/)
- [Kicad Tutorial](https://youtu.be/AHlyiWntAKU?t=806)

