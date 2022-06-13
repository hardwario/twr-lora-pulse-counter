<a href="https://www.hardwario.com/"><img src="https://www.hardwario.com/ci/assets/hw-logo.svg" width="200" alt="HARDWARIO Logo" align="right"></a>

# LoRa pulse counter kit firmware

[![Travis](https://img.shields.io/travis/bigclownlabs/bcf-lora-pulse-counter/master.svg)](https://travis-ci.org/bigclownlabs/bcf-lora-pulse-counter)
[![Release](https://img.shields.io/github/release/bigclownlabs/bcf-lora-pulse-counter.svg)](https://github.com/bigclownlabs/bcf-lora-pulse-counter/releases)
[![License](https://img.shields.io/github/license/bigclownlabs/bcf-lora-pulse-counter.svg)](https://github.com/bigclownlabs/bcf-lora-pulse-counter/blob/master/LICENSE)
[![Twitter](https://img.shields.io/twitter/follow/hardwario_en.svg?style=social&label=Follow)](https://twitter.com/hardwario_en)

This repository contains firmware LoRa pulse counter for [Core Module](https://shop.bigclown.com/core-module).

If you want to get more information about Core Module, firmware and how to work with it, please follow this link:

**https://developers.hardwario.com/firmware/basic-overview/**

## Introduction

LoRa pulse counter to be used as a wireles counter. Device running from two AAA batteries. It reports measured data to LoRaWAN network every 15 minutes.

## Hardware

The following hardware components are used for this project:

* **[Sensor Module](https://shop.bigclown.com/sensor-module)**
* **[Core Module](https://shop.bigclown.com/core-module)**
* **[LoRa Module](https://shop.bigclown.com/lora-module)**
* **[Mini Battery Module](https://shop.bigclown.com/mini-battery-module)**


## Buffer
big endian

| Byte   | Name            | Type   | Factor   | Unit
| -----: | --------------- | ------ | -------- | -------
|      0 | HEADER          | uint8  |          |
|      1 | BATTERY         | uint8  | x 0.1    | V
|      2 | ORIENTATION     | uint8  |          |
|  3 - 4 | TEMPERATURE     | int16  | x 0.1    | °C
|  5 - 8 | CHANNEL A COUNT | uint32 |          |
|  9 - 12| CHANNEL B COUNT | uint32 |          |

### Header

* 0 - bool
* 1 - update
* 2 - button click
* 3 - button hold

## AT

```sh
picocom -b 115200 --omap crcrlf  --echo /dev/ttyUSB0
```

## License

This project is licensed under the [MIT License](https://opensource.org/licenses/MIT/) - see the [LICENSE](LICENSE) file for details.

---

Made with &#x2764;&nbsp; by [**HARDWARIO a.s.**](https://www.hardwario.com/) in the heart of Europe.
