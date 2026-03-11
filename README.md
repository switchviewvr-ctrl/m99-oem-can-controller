# Supernova M99 Pro OEM CAN Controller (Arduino)

DIY-Controller zur Steuerung einer **Supernova M99 Pro OEM (MIFA / Grace)** ohne originales E-Bike-System.

Dieses Projekt emuliert die notwendigen CAN-Nachrichten eines E-Bike-Controllers, sodass die Lampe vollständig funktioniert.

Der Controller ermöglicht:

* Abblendlicht
* Fernlicht
* zwei Abblendlicht-Helligkeitsstufen
* Bremslicht
* Rücklichtsteuerung über die Frontlampe

---

# Inhaltsverzeichnis

* [Hintergrund](#hintergrund)
* [Funktionsübersicht](#funktionsübersicht)
* [Systemarchitektur](#systemarchitektur)
* [Hardware](#hardware)
* [Stromversorgung](#stromversorgung)
* [CAN-Parameter](#can-parameter)
* [Verkabelung](#verkabelung)
* [Kabelfarben M99](#kabelfarben-m99)
* [Rücklichtanschluss](#rücklichtanschluss)
* [CAN-Nachrichten](#can-nachrichten)
* [Tastersteuerung](#tastersteuerung)
* [Bremssensor](#bremssensor)
* [Beispielcode](#beispielcode)
* [Typische Fehler](#typische-fehler)
* [Erweiterungen](#erweiterungen)
* [Hinweise](#hinweise)

---

# Hintergrund

Viele OEM-Versionen der **Supernova M99 Pro** starten nicht, wenn sie nur mit Spannung versorgt werden.

Die Firmware erwartet CAN-Kommunikation eines E-Bike-Controllers.

Ohne diese Kommunikation bleibt die Lampe deaktiviert.

Dieses Projekt emuliert die benötigten CAN-Frames mit einem Arduino.

---

# Funktionsübersicht

| Funktion           | Steuerung                   |
| ------------------ | --------------------------- |
| Abblendlicht       | automatisch                 |
| Fernlicht          | kurzer Tastendruck          |
| Abblendlicht Stufe | langer Tastendruck          |
| Bremslicht         | Bremssensor                 |
| Rücklicht          | automatisch über Frontlampe |

---

# Systemarchitektur

```
Brake Sensor ─┐
              │
Button ───────┤
              │
           Arduino
              │
           MCP2515
              │
            CAN Bus
              │
       Supernova M99
              │
          Tail Light
```

---

# Hardware

## Benötigte Komponenten

| Bauteil                  | Beschreibung    |
| ------------------------ | --------------- |
| Arduino Leonardo         | Mikrocontroller |
| MCP2515 Modul            | CAN Controller  |
| DC-DC Buck Converter     | Akku → 5V       |
| Taster                   | Lichtsteuerung  |
| Magnet Bremssensor       | Bremslicht      |
| Supernova M99 Pro OEM    | Frontlicht      |
| Supernova M99 Tail Light | Rücklicht       |

---

# Stromversorgung

Die M99 unterstützt:

```
24V – 60V DC
```

Der Arduino benötigt:

```
5V
```

Verwendung eines DC-DC Step-Down Converters:

```
Akku → StepDown → Arduino 5V
```

---

# CAN Parameter

```
Bitrate: 250 kbit/s
Controller: MCP2515
Quarz: 8 MHz
```

Der MCP2515 benötigt einen **120Ω Terminierungswiderstand**.

Auf den meisten Modulen wird dieser über **Jumper J1** aktiviert.

---

# Verkabelung

## MCP2515 → Arduino Leonardo

Beim Leonardo läuft SPI über den **ICSP Header**.

| MCP2515 | Arduino   |
| ------- | --------- |
| VCC     | 5V        |
| GND     | GND       |
| CS      | Pin 10    |
| SO      | ICSP MISO |
| SI      | ICSP MOSI |
| SCK     | ICSP SCK  |
| INT     | optional  |

---

# Kabelfarben M99

Bei der getesteten M99 OEM Version wurden folgende Kabelfarben festgestellt.

| Farbe   | Funktion     |
| ------- | ------------ |
| Rot     | Versorgung + |
| Schwarz | GND          |
| Gelb    | CAN-H        |
| Blau    | CAN-L        |

```
Gelb = CAN-H
Blau = CAN-L
```

---

# OEM Signaladern

Die Lampe besitzt zusätzliche Signaladern für OEM-Bikes.

Diese werden normalerweise verwendet für:

* Fernlichtschalter
* Bremslichtschalter
* Controllersteuerung

Da dieses Projekt alle Funktionen über CAN steuert, werden diese Leitungen **nicht benötigt**.

---

# Rücklichtanschluss

Das Rücklicht wird **direkt an die Frontlampe angeschlossen**.

Die Frontlampe stellt eine **12V Versorgung** für das Rücklicht bereit.

| Farbe   | Funktion   |
| ------- | ---------- |
| Schwarz | GND        |
| Blau    | Rücklicht  |
| Gelb    | Bremslicht |

Anschluss:

```
Frontlight → Tail Light

Schwarz → Schwarz
Blau → Blau
Gelb → Gelb
```

---

# CAN Nachrichten

Der Arduino sendet regelmäßig CAN-Frames.

## Licht aktivieren

```
CAN ID: 0x400
Data: 01
```

---

## Batterie

```
CAN ID: 0x402
Data: 100 0 0 0
```

Verhindert Leistungsreduzierung.

---

## Geschwindigkeit

```
CAN ID: 0x201
Data: 2 Byte
```

Little Endian.

```
3000 → 30 km/h
500  → 5 km/h
```

Die Geschwindigkeit beeinflusst die Abblendlichthelligkeit.

---

## Frontlicht

```
CAN ID: 0x501
```

Bitstruktur:

```
Bit0 → High Beam
Bit2 → Speed dependent light
```

Beispiele:

```
0x04 → Abblendlicht
0x05 → Fernlicht
```

---

## Bremslicht

```
CAN ID: 0x503
```

```
0x01 → Bremslicht an
0x00 → Bremslicht aus
```

Das Bremslicht wird separat übertragen und beeinflusst das Frontlicht nicht.

---

# Tastersteuerung

Ein einzelner Taster steuert zwei Funktionen.

| Aktion       | Funktion                   |
| ------------ | -------------------------- |
| kurzer Druck | Fernlicht an / aus         |
| langer Druck | Abblendlichtmodus wechseln |

---

# Abblendlichtmodi

Die Helligkeit wird über eine simulierte Geschwindigkeit gesteuert.

```
LOW_MIN = 5 km/h
LOW_MAX = 30 km/h
```

---

# Bremssensor

Der Bremssensor wird über `INPUT_PULLUP` betrieben.

```
pinMode(BRAKE_PIN, INPUT_PULLUP);
```

Je nach Sensortyp muss die Logik angepasst werden.

---

## Normally Open (NO)

```
nicht bremsen → HIGH
bremsen → LOW
```

Code:

```
bool brakeActive = (digitalRead(BRAKE_PIN) == LOW);
```

---

## Normally Closed (NC)

```
nicht bremsen → LOW
bremsen → HIGH
```

Code:

```
bool brakeActive = (digitalRead(BRAKE_PIN) == HIGH);
```

---

# Beispielcode

*(gekürzt – vollständiger Code im Repository)*

```cpp
byte mode = 0x04;

if(highBeam)
  mode |= 0x01;

byte msg501[1] = {mode};
sendFrame(0x501,1,msg501);

bool brakeActive = (digitalRead(BRAKE_PIN) == HIGH);

byte msg503[1] = { brakeActive ? 1 : 0 };
sendFrame(0x503,1,msg503);
```

---

# Typische Fehler

### Lampe startet nicht

* falsche CAN Bitrate
* fehlender Terminierungswiderstand
* CAN-Frames werden nicht regelmäßig gesendet

---

### CAN funktioniert nicht

* CAN-H / CAN-L vertauscht
* keine gemeinsame Masse

---

### Bremslicht funktioniert falsch

Sensorlogik invertieren.

---

# Erweiterungen

Mögliche Erweiterungen:

* Blinker
* automatische Lichtsteuerung
* Display
* eigener Controller
* Integration in E-Bike Systeme

---

# Hinweise

Dieses Projekt ist ein DIY-Projekt.

Die Nutzung im Straßenverkehr erfolgt auf eigene Verantwortung.

Achte auf:

* wasserdichte Gehäuse
* sichere Verkabelung
* stabile Stromversorgung
