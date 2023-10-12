# Famulator: A Famicom/NES Emulator

<img src ="./image/smb_grid.png" width=656>
<img src ="./image/dragon_quest_2_play.png" width=656>

## Overview
- A NES Emulator
- Written in C++
- Requires GLFW and OpenAL
- Supports mapper 0, 1, 2, 3, 4, 16, 19 (200+ games)

## Features
- Game pad support
- A, S, W, D -> cross button
- K, L -> B and A buttons
- B, N -> select and start buttons
- R -> reset button
- P -> Show tile pattern
- G -> Show tile and sprite grid
- F1 -> Save emulator state
- F2 -> Load emulator state
- Five audio channels supoprted
- No multi player supoprt yet

## Build
- `$ make`
    - Builds nes
- `$ make test`
    - Builds nes and runs test

## Play
- `$ ./nes your_game.nes`

## Platforms
- MacOS with clang

## License
- MIT License

## Under development
- Save/load emulator status
- Switching to SDL2
- More mappers

<img src ="./image/super_mario_bros.png" width=164> <img src ="./image/dragon_quest_2.png" width=164>
<img src ="./image/donkey_kong.png" width=164>
<img src ="./image/ice_climber.png" width=164>
<img src ="./image/golf.png" width=164>
<img src ="./image/xevious.png" width=164>
<img src ="./image/galaxian.png" width=164>
<img src ="./image/star_force.png" width=164>
<img src ="./image/star_luster.png" width=164>
<img src ="./image/mach_rider.png" width=164>
<img src ="./image/portpia_renzoku_satsujin_jiken.png" width=164>
