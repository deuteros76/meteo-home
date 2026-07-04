---
layout: page
title: Hardware
---
The hardware has been designed in Kicad. All the source files are available under the Kicad directory of this repository.

![](pics/meteo-home-kicad.png)

### Hardware components
- {:  .list} Wemos D1 Mini Lite (the board is designed to use this microcontroller)
- {:  .list} One or more sensors (DHT22, BMPP180, SGP-30)
- {:  .list} Wires, tools...

Below the current BOM is shown. There are no sensors included, as the idea is to plug any of the supported and are not considered part of the hardware design:

|#  |Reference|Qty|Value                     |Footprint                                                     |
|---|---------|---|--------------------------|--------------------------------------------------------------|
|1  |D1       |1  |Red                       |LEDs:LED_D5.0mm                                               |
|2  |D2       |1  |Yellow                    |LEDs:LED_D5.0mm                                               |
|3  |D3       |1  |Green                     |LEDs:LED_D5.0mm                                               |
|4  |R1, R2   |2  |160                       |Resistors_THT:R_Axial_DIN0207_L6.3mm_D2.5mm_P7.62mm_Horizontal|
|5  |R3       |1  |90                        |Resistors_THT:R_Axial_DIN0207_L6.3mm_D2.5mm_P7.62mm_Horizontal|
|6  |R4       |1  |10K                       |Resistors_THT:R_Axial_DIN0207_L6.3mm_D2.5mm_P7.62mm_Horizontal|
|8  |U1       |1  |WeMos_D1_mini             |wemos-d1-mini:wemos-d1-mini-with-pin-header                   |



![Real hardware](pics/meteo-home-wemos.png)
