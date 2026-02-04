Plane War Game README  

  Ⅰ. Project Introduction  
A two-player plane shooting game developed with C++ and EasyX graphics library, featuring multi-level challenges, an item system, a shop system, and cooperative two-player gameplay. Players control planes to dodge enemy attacks, earn scores by shooting, and unlock higher-difficulty levels.  

  Ⅱ. Features  
1.  Game Modes   
   -  Single/Double Player : Supports 1-2 players. Player 1 uses arrow keys, while Player 2 uses WASD keys for movement.  
   -  Endless Mode & Level Mode : Level Mode includes 3 stages to unlock sequentially; Endless Mode offers infinite challenges.  

2.  Core Gameplay   
   -  Movement & Shooting : Player 1 uses arrow keys to move and spacebar to shoot; Player 2 uses WASD for movement and Ctrl for shooting in two-player mode.  
   -  Item System : Randomly dropping items include shields, health packs, and bullet upgrades. Diamonds can be used to buy initial items in the shop.  
   -  Boss Battles : Triggered by reaching specified scores in each level, with Bosses having high HP and special attack patterns.  

3. Progress Saving
   - Automatically saves diamond count, item inventory, and unlocked levels, loading progress on startup.  

Ⅲ. Controls  
| Action          | Player 1          | Player 2          |  
| Move            | Arrow Keys (←→↑↓) | WASD Keys         |  
| Shoot           | Spacebar          | Ctrl Key          |  
| Pause/Resume    | Spacebar          | Spacebar          |  
| Shop Purchase   | Mouse Click       | Mouse Click       |  

Ⅳ. Requirements  
- Development Environment : Visual Studio 2022 + EasyX graphics library (requires configuring include paths and libraries).  
- Runtime Environment : Windows OS with EasyX runtime library installed (or run the compiled .exe directly).  

Ⅴ. File Structure  
- `main.cpp`: Main game logic, including interfaces, object definitions, and game loops.  
- `images/`: Folder for plane, enemy, and item textures (ensure path is correct, default `../images/`).  
- `gamedata.dat`: Auto-generated save file storing player progress.  

Ⅵ. Compilation Guide  
1. Create a new project in Visual Studio and add `main.cpp`.  
2. Configure EasyX library paths:  
   - Include directory: `C:\Program Files (x86)\EasyX\include`  
   - Library directory: `C:\Program Files (x86)\EasyX\lib\VC` (or the library folder for your VS version)  
   - Additional dependencies: `graphics.lib`  
3. Compile in Release mode to generate the executable file.  