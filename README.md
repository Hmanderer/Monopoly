# Monopoly

A desktop Monopoly-style board game written in C++ using SDL3 and SDL3_ttf.

## Features

- 2–8 players
- Property buying and trading
- Rent, hotels and property groups
- Cards and auctions
- Bankruptcy and player elimination
- Save/load support
- Pause menu

## Requirements

- Windows 10/11
- Visual Studio with C++20 support
- Desktop development with C++ workload

The repository includes the x64 SDL3 and SDL3_ttf headers/libraries required to build the project.

## Build

1. Clone the repository.
2. Open `Monopoly.slnx` in Visual Studio.
3. Select `x64` and `Debug` or `Release`.
4. Build the solution.
5. Run `Monopoly.exe` from the generated output folder.

Required runtime DLLs and the font are copied automatically after a successful build.

## Controls

- `Esc` — pause menu / cancel
- `F5` — save game
- `F9` — load game
- Mouse — interact with the game UI
- Keyboard — enter text when prompted

## Project structure

```text
Monopoly/
├── Monopoly/
│   ├── assets/
│   │   └── fonts/
│   ├── src/
│   ├── Monopoly.vcxproj
│   └── Monopoly.vcxproj.filters
├── dependencies/
│   ├── SDL3/
│   └── SDL3_ttf/
├── Monopoly.slnx
├── README.md
└── .gitignore
```

## Dependencies

- SDL3
- SDL3_ttf

Their licenses are included in the corresponding `dependencies` directories.
