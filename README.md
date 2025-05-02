# Cleanse the Corruption - Team 25

Members:
Munn Chai, Kevin Guo, Kevin Shaw, Jason Xu, Mark Zhu

## Game Summary

Our game is a 2d topdown roguelike shooter, where you play as a wizard defeat the Grand Evil Wizard to cleanse the corruption that has taken control over every other wizard.

## Controls

- **WASD**: Move the player character
- **WASD + LShift**: Make the player character dash in the direction specified
- **E**: Interact with doors, items, relics, heath packs
- **Q**: Interact with relics to apply them to your movement spell
- **LMB**: The player character shoots a projectile towards the mouse cursor
- **Mouse Movement**: Move the mouse around the screen to move the camera slightly in that direction


### Milestone 4 Notes
- There is a bug report and test plan present in the doc folder. 

### Milestone 4 Game Balance Notes
- **Interactables**: In a playthrough, we noticed that players were far too strong after getting lucky and obtaining numerous relics, especially the Numbers(+1 Projectile) relic, and were just deleting the enemies.
  - Interactables' drop rates from all enemies lowered to 30% from 50%
  - Per drop, Numbers relic and Health Pack rate has been lowered to 10% each from 25%, with the rest of the 80% of drops being random **other** relics
  - Cooldown relic's cooldown reduction reduced from 0.1 to 0.02
  - Strength relic's damage buff reduced from 2 to 1
- **Boss1**: Bosses did not feel like they scaled well relative to the relic buffs that players obtained. We wanted to make sure that buffed up players would not delete the bosses while also not being too challenging for slightly weaker players. As devs, we are able to consistently kill the current Boss1 with basic fireball under 1 minute, so we believe that it should be a reasonable experience for the player.
  - Health tripled
  - Hitbox has been enlarged
- **Boss2**: The boss feels very weak after one of the twins dies, we want to buff the boss such that the remaining twin goes into rampage mode after the other twin dies.
  - The remaining twin's attack cooldown is reduced to ~1/10 of original cooldown during rampage mode. 
- **Melee Enemy**: Melee enemies were too strong for a basic enemy, running much faster than player and dealing lots of damage per hit at the same time.
  - Speed decreased to PLAYER_SPEED +5 from +15
  - Attack range decreased to 50 from 100
- **Fireball**: Although we don't think the starting spell should be too strong, the old fireball just felt very underwhelming and unsatisfying to use.
  - Speed increased from 350 to 850
  - Damage decreased from 10 to 5
- **Shotgun**: Changing to be more like a real shotgun.
  - Pellet number increase from 3 to 7
  - Damage per pellet decreased from 6 to 2
  - Increased cooldown from 1.0 to 1.1
- **Thorn Bomb**: Similar to Boomerang, the spell felt pretty unsatisfying to use since the range made it hard to control.
  - Speed increased from 500 to 550
  - Lifetime increased from 0.75 to 0.85
  - Deceleration decreased from 550 to 500
- **Boomerang**: The range on the spell was extremely short and strict, making it hard to use and not very rewarding.
  - Speed increased from 1000 to 1100 to "increase range"
  - Damage decreased from 20 to 10
- **Magnet**: This was by far the most overpowered spell in our game. Once the player obtained a few buffs, this spell would basically guarantee that all enemies are killed almost instantly.
  - Damage decreased from 5 to 2
  - Cooldown increased from 0.25 to 0.4

### Milestone 4 Features:
- **Game Balance (Basic)**: We took input from previous play-tests as well as how we felt about the state of the game to balance a lot of the properties in our game. Documented in the Game Balance Notes above.
- **Reloadability (Basic)**: We imported json library (imported from https://github.com/nlohmann/json) to store our highest score, keybinding and audio intensity, which also include the functions that changing keybind (See reloadability.hpp) . The setting will be persisted each time when you access setting. Every time when you open the game the setting and highest score will be loaded from folder "persistance" in data folder (See reloadability.hpp and reloadability.cpp)
- **Story Elements (Basic)**: We added an intro cutscene, and an ending cutscene for when the player beats the game. Additionally, there are a couple NPCs who have dialogue when you interact with them. 

### Milestone 3 Notes

- To improve our gameplay for Milestone 3, we created better room generation, to make each floor in our game feel like a maze of rooms. Additionally, we added "goals" for each floor, which give you additional rewards if you complete them. Finally, we implemented 2 boss enemies, which make the game much more challenging, but also more rewarding to play. 

### Milestone 3 Features:
- **BSP Map Generation (Advanced)**: We used Binary Space Partitioning to procedurally generate the floors in our game. We customized the algorithm so we insert custom designed rooms into the map, while still having random generation of the layouts of each floor.
- **External Library Integration (Basic)**: We implemented the FreeType library into our game, allowing us to easily render text onto the screen for things like the tutorial, or for our menus. 
- **Basic Asset Integration (Basic)**: All of our assets were hand drawn using Aseprite, using a specific colour palette to make the sprites cohesive to each other. 

### Milestone 2 Notes

- None :)

### Milestone 2 Features

- **Improved Game AI**: State driven behaviour. To make the game more interesting, enemies move faster and can also shoot at the player while moving. When they their HP gets low, or if the player gets too close to them, they will flee for a certain distance to maintain their range. (See ai_system.cpp and enemy_types/basic_ranged_enemy.cpp)
- **Sprites and Animations**: We have added new sprites to our game, as well as creating sprite sheets for the enemy and player to animate their sprites.
- **Mesh-based Collisions**: We implemented Mesh Based collisions for the Player, Enemies, and Projectiles. (See physics_system.cpp::43, polygonsCollide)
- **Tutorial Room**: There is an optional tutorial room that shows the controls, as well as giving the player a chance to try some of the actions.

### Milestone 2 Creative Elements

- **Particle System**: We created a particle system which uses instanced rendering to efficiently draw many particles at once, and very basic physics to move the particles around. (see render_system.cpp::715 for rendering, and most particle functionality can be found in components.hpp::523)

### Milestone 1 Notes

The player currently does not die when their health reaches 0, and there is no reset button at the moment. To reset the game, simply re-run the project.

### Milestone 1 Features

- **Textured Geometry**: We simply rendered sprites using the drawTexturedMesh function from the A1 template, but we also implemented instanced rendering for the floor and wall tiles (which can be found in **render_system.cpp:19**). The draw order of the sprites is as follows (from first to last):

  1.  Floors and walls
  2.  Particles\* (see Milestone 2 Features)
  3.  Y-sorted Entities
  4.  Projectiles
  5.  Health bars

  Y-sorted entities include the player and enemies. When comparing 2 y-sorted entities, the entity with the higher y-position will be drawn first, so entities with lower y-positions will be drawn on top of them.

- **Basic 2D Transformation**: We use translations to move the players, projectiles, and enemies around. We also use rotations to rotate the enemy to "look" at the player. These transformations are simply put in a transformation matrix, then applied to each vertex in the vertex shader.
- **Linear Interpolation**: We used a lerp function to smoothly move the camera to a position towards a midpoint between the player character and the mouse position. This allows the player to "look" in a direction that they would like, by moving their mouse in that direction. The lerp function can be found in **common.cpp:64**, and the usage can be found in **camera_system.cpp:32**.
- **Keyboard/Mouse Control**: These are the keybinds and mouse actions which we implemented:
  - **WASD**: These keys move the player in the up, left, down, and right directions, respectively.
  - **LShift**: This key casts a Dash spell, which applies a large acceleration to the player, moving them in the direction they specify with WASD.
  - **LMB**: This key casts a Projectile spell, which instantiates a projectile which moves in the direction of the mouse cursor.
  - **RMB**: Thsi key will spawn an enemy in a location relative to the mouse position. (this is mostly for testing purposes, and will not be included in future milestones)
- **Random/Hard-coded Action**: Enemies will randomly spawn at a fixed interval, in a random location.
- **Well-Defined Game Space Boundaries**: There are 4 walls which spawn in a square around the player. The player cannot move through these walls.
- **Simple Collision Detection/Resolution**: The following collisions currently exist in the game:
  - **Player/Enemy & Wall**: Players and enemies will be stopped when they collide with a wall, and they cannot move through it.
  - **Player/Enemy & Projectile**: Enemies will collide with projectiles shot by the player and take damage, and players will collide with projectiles show by the enemy and take damage. The projectile will disappear in either of these collisions.
  - **Projectile & Wall**: When a projectile collides with a wall, the projectile will disappear.

### Milestone 1 Creative Elements

- **Camera Controls**: Rather than having a static camera which follows the player, we have a camera which interpolates to a position between the mouse position and the player position.
- **Audio Feedback**: We implemented audio feedback for the following interactions:
  - Player is hit by an enemy projectile
  - Enemy is hit by a player projectile
  - An enemy spawned
  - Background music
