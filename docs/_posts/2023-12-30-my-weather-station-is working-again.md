---
layout: post
title:  "My weather station is working again"
date:   2023-12-30 09:00:00 +0100
categories: meteo-home
image: /assets/images/has-meteohome-weather-ext.png
---


Following the resolution of certain power source challenges stemming from the batteries initially employed, I am happy to say that my external solar-powered weather station has been successfully restored to full functionality. This particular (MeteoHome based) weather station is equipped with a  DHT22 sensor to get both temperature and humidity levels. Additionally, it features a BMP180 sensor,  dedicated to monitoring atmospheric pressure with quite good accuracy.

The story of this gadget is that I had it working continuously for a long time, approximately two years, until [it stopped working](/es/130-recuperando-mi-metehome), and I wasn't very clear why. It spent another few years forgotten, and recently I took the opportunity to move the components to a [MeteoHome](https://github.com/deuteros76/meteo-home) PCB. Everything was working until I left it on the terrace with the solar panel, and during the night, it stopped working.

Clearly, something was happening with the recharge process. One of the problems I had was that the battery charge controller seemed to be malfunctioning. This puzzled me until I discovered that it was probably the USB cable that wasn't working because it was likely severed. Once this was resolved, I realized that, this time, I had placed rechargeable AA batteries in the same controller, which was intended for 18650 batteries.

Connecting a battery charger designed for 18650 batteries to AA batteries is not recommended and can be dangerous. 18650 and AA batteries are different types and have different electrical characteristics. In summary, they have different voltages and capacities. 18650 batteries typically have a nominal voltage of 3.7 V, while AA batteries have a voltage of 1.5 V. Additionally, their energy storage capacities also differ. Connecting a charger designed for the voltage and capacity of one type of battery to another can cause overcharging, overheating, and damage to the batteries. If you connect a 18650 battery charger to AA batteries, the charging current may be too high for the AA batteries, resulting in excessive heat, damage, overcharging, short circuits, or even the release of hazardous gases, as in the case of lithium-ion batteries.
