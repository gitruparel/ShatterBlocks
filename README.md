# ShatterBlocks

ShatterBlocks is a neon-themed, retro-inspired brick breaker game built from scratch in C using SDL2, SDL_mixer, and SDL_ttf. Smash through rows of animated bricks, rack up your high score, and enjoy synthwave soundscapes and vibrant visual effects!

![Recording 2025-07-24 005531](https://github.com/user-attachments/assets/4a96eb07-58dc-4d39-81da-2448ee17f1cb)


- Colorful, animated bricks with distinct strengths and points
- Fluid paddle and ball movement with bouncy physics
- Eye-catching neon title and particle effects
- Arcade-style synthwave background music and retro sound effects
- Game Over screen with animated overlays, score summary, and restart button
- Keyboard and mouse controls for easy, responsive play
- Streamlined project structure for easy editing and builds

## Gameplay Screenshots
<img width="802" height="632" alt="image" src="https://github.com/user-attachments/assets/412f6051-78b2-4c5e-a638-8d776c2b7f84" />
<img width="802" height="632" alt="image" src="https://github.com/user-attachments/assets/f63bcb84-024f-47b7-8d23-ba6b9a090b27" />


- **Move the paddle:** Left & right arrow keys
- **Quick move:** Hold Ctrl + arrows
- **Pause/Resume:** Spacebar (during game)
- **Restart:** Spacebar or click "Restart" after Game Over
- **Winning:** Break as many bricks as you can—harder bricks score more points!
- **Losing:** Miss the ball
- **Enjoy:** Retro sound FX and synthwave tunes as you play

## Building and Running (Windows, Visual Studio)

### Prerequisites

- **Visual Studio 2019/2022** (Community, Professional, or Enterprise)
- **SDL2**, **SDL2_mixer**, and **SDL2_ttf** development libraries
    - Place all SDL2 `.dll` files next to your built `.exe`
    - Ensure `beon.ttf` font and your `sfx/` audio are in the project directory

### Steps

1. **Clone the Repository:**
    ```sh
    git clone https://github.com/gitruparel/ShatterBlocks.git
    cd ShatterBlocks
    ```

2. **Open the Solution:**
    - Launch Visual Studio.
    - Open `ShatterBlocks.sln`.

3. **Set Up Dependencies:**
    - Download and extract SDL2, SDL2_mixer, and SDL2_ttf.
    - In "Project Properties" → "VC++ Directories":
        - Add `include` and `lib` paths for each library.
        - Under "Linker > Input", add:
            - `SDL2.lib`, `SDL2main.lib`, `SDL2_mixer.lib`, `SDL2_ttf.lib`
    - Copy the `.dll` files to your executable output folder (`Debug/` or `Release/`).

4. **Build the Project:**
    - In Visual Studio, select "Build → Build Solution" (or press `Ctrl+Shift+B`).

5. **Run the Game:**
    - Press `F5` to start playing!

## Folder Structure

```
ShatterBlocks/
├── src/                  # All .c and .h files
├── sfx/                  # Sound effects & music (WAV)
├── beon.ttf              # Font for the neon text
├── .gitignore
├── README.md
├── ShatterBlocks.sln     # Visual Studio Solution
├── Debug/                # Builds (should be ignored in git)
├── Release/              # Builds (should be ignored in git)
└── ...other assets
```

## Credits & License

- Code: [Your Name]
- SFX & Music: By [Author Name] or generated for this project
- Font: "Beon" (free font, check license)
- Built with: SDL2, SDL2_mixer, SDL2_ttf

Released under the MIT License. See [LICENSE](LICENSE).

## Support

- For any issues or suggestions, open an [issue](https://github.com/yourusername/ShatterBlocks/issues).
- Pull requests are welcome!

Enjoy breaking blocks retro-style with *ShatterBlocks*!
