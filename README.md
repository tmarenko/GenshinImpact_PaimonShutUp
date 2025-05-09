# GenshinImpact_PaimonShutUp

![GitHub Workflow Status (branch)](https://img.shields.io/github/actions/workflow/status/tmarenko/GenshinImpact_PaimonShutUp/build.yml?branch=main)
![GitHub release (latest by date)](https://img.shields.io/github/downloads/tmarenko/GenshinImpact_PaimonShutUp/total)
![GitHub](https://img.shields.io/github/license/tmarenko/GenshinImpact_PaimonShutUp)

Automatically mutes Paimon when she speaks

## How It Works

This tool automatically lowers the game's volume whenever Paimon starts talking, using official Windows APIs for both screen capture and volume control.
It works by taking screenshots of the game to check if Paimon's dialogue is on the screen.
When Paimon is detected, the tool mutes the game's until the next NPC starts talking.

It doesn't change any game files or interfere with how the game runs, so it doesn't violate the Terms of Service and should not trigger the anti-cheat system.


**Demo**: https://youtu.be/pBpDdLcw6zk

## Installation and usage

1. Download archive from [latest release](https://github.com/tmarenko/GenshinImpact_PaimonShutUp/releases) and extract it
2. Run `GenshinImpact_PaimonShutUp.exe` as an administrator

**Note:** Administrator rights are required for access to [Windows Audio session](https://learn.microsoft.com/en-us/windows/win32/api/audioclient/nn-audioclient-isimpleaudiovolume) controls.

## Configuration

## Edit `settings.cfg` file:

| Setting | Description |
|---------|-------------|
| `language` | Your in-game language (see supported options below) |
| `capture_mode` | Screen capture method |
| `mute_in_overworld` | Toggle muting in open world exploration |
| `ocr_max_errors` | Name recognition tolerance level |

### Supported languages:
- 简体中文 (`language=chi_sim`)
- 繁體中文 (`language=chi_tra`)
- English (`language=eng`)
- 한국어 (`language=kor`)
- 日本語 (`language=jpn`)
- Español (`language=spa`)
- Français (`language=fra`)
- Русский язык (`language=rus`)
- ภาษาไทย (`language=tha`)
- Tiếng Việt (`language=vie`)
- Deutsch (`language=deu`)
- Bahasa Indonesia (`language=ind`)
- Português (`language=por`)

## Troubleshooting

If you encounter issues with the application, try these troubleshooting steps:

**Language Settings**: Ensure the language parameter in `settings.cfg` matches your actual in-game language. Recognition depends on correct language selection.

**Screen Capture Issues**: Try changing the `capture_mode` parameter if the application isn't detecting dialogues properly. Different capture methods work better on certain systems.

**Recognition Sensitivity**: For lower resolution displays or when text detection seems inconsistent, adjust the `ocr_max_errors` value. Higher values make detection more forgiving but may cause false positives.

## Legal Notice

This software is not affiliated with or endorsed by miHoYo/HoYoverse.

Genshin Impact is a trademark or registered trademark of miHoYo/HoYoverse.

This project is a fan-made tool intended for personal use only.

The software operates entirely independently and does not interact with the game
client or servers in any way, ensuring compliance with the Genshin Impact Terms of Service.