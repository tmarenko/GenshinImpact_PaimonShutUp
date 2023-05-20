# GenshinImpact_PaimonShutUp

![GitHub Workflow Status (branch)](https://img.shields.io/github/actions/workflow/status/tmarenko/GenshinImpact_PaimonShutUp/build.yml?branch=main)
![GitHub release (latest by date)](https://img.shields.io/github/downloads/tmarenko/GenshinImpact_PaimonShutUp/total)
![GitHub](https://img.shields.io/github/license/tmarenko/GenshinImpact_PaimonShutUp)

Automatically mutes Paimon when she speaks

#### Watch how it works: https://youtu.be/pBpDdLcw6zk

## Installation and usage

Download archive from
[latest release](https://github.com/tmarenko/GenshinImpact_PaimonShutUp/releases)
, extract it and run `GenshinImpact_PaimonShutUp.exe` with administrator privileges. 
Administrator rights needed for access to 
[Windows Audio session](https://learn.microsoft.com/en-us/windows/win32/api/audioclient/nn-audioclient-isimpleaudiovolume).

## It's not working! What to do?

Read [configuration section](#configuration) below and check the following steps:
* Make sure that `language` parameter matches in-game language
* Change `capture_mode` parameter to a different capture mode
* If you are playing on a low screen resolution, try increasing `ocr_max_errors` parameter

## Configuration

### Edit `settings.cfg` file to:
* setup in-game language
* change screen capture modes
* disable mute in the overworld
* adjust name recognition to be more or less strict

### Supported languages:
* 简体中文 (`language=chi_sim`)
* 繁體中文 (`language=chi_tra`)
* English (`language=eng`)
* 한국어 (`language=kor`)
* 日本語 (`language=jpn`)
* Español (`language=spa`)
* Français (`language=fra`)
* Русский язык (`language=rus`)
* ภาษาไทย (`language=tha`)
* Tiếng Việt (`language=vie`)
* Deutsch (`language=deu`)
* Bahasa Indonesia (`language=ind`)
* Português (`language=por`)
