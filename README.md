# 🎳 Bowling Xtreme

A roguelike bowling game where every run is different. Buy balls, pins, shoes and powers from the shop between rounds to build crazy scoring combos.

## Download & Play

Head to the [Releases page](../../releases/latest) and download the zip for your platform.

**Mac:** Unzip and double-click `Play Bowling.command`
> First time only: if macOS blocks it, right-click → Open → Open anyway

**Windows:** Unzip and double-click `bowling.exe`

---

## How to Play

### Controls
| Key | Action |
|-----|--------|
| `A` / `D` | Move ball left / right |
| `← / →` | Aim |
| `Space` | Throw ball |
| `B` | Toggle bumpers |
| `N` | Use Duplicate power |
| `V` | Use Swap power |
| `R` | Restart run |
| `M` | Back to menu |

### Scoring
Each shot scores: **(Base + Pin Values) × (Pins Hit + Combo)**

- Hit more pins = bigger multiplier
- Higher value pins (pin 10 = 10pts) = bigger impact
- Strikes give a **+40% bonus**

### Game Structure
- Each **round** has 2 frames, each frame has 2 shots
- Hit the **target score** to pass the round and reach the shop
- Fail to hit the target and it's game over

### The Shop
After each round you earn tokens to spend on:
- **Balls** — special effects on every shot
- **Pins** — one pin per slot gets a special type each frame
- **Shoes** — passive bonuses that change how everything behaves
- **Powers** — run-long upgrades and one-time abilities

---

## Building from Source

Requires [SFML 3](https://www.sfml-dev.org/) and CMake 3.16+.

```bash
# macOS
brew install sfml
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/bowling
```
