#pragma once

#include <array>
#include <utility>
#include <map>

#include "common.hpp"
#include "tinyECS/components.hpp"
#include "tinyECS/tiny_ecs.hpp"


// System responsible for setting up OpenGL and for rendering all the
// visual entities in the game
class RenderSystem {
	/**
	 * The following arrays store the assets the game will use. They are loaded
	 * at initialization and are assumed to not be modified by the render loop.
	 *
	 * Whenever possible, add to these lists instead of creating dynamic state
	 * it is easier to debug and faster to execute for the computer.
	 */
	std::array<GLuint, texture_count> texture_gl_handles;
	std::array<ivec2, texture_count>  texture_dimensions;

	// Make sure these paths remain in sync with the associated enumerators.
	// Associated id with .obj path
	//const std::vector<std::pair<GEOMETRY_BUFFER_ID, std::string>> mesh_paths = {
	//	std::pair<GEOMETRY_BUFFER_ID, std::string>(GEOMETRY_BUFFER_ID::CHICKEN, mesh_path("chicken.obj"))
	//	// specify meshes of other assets here
	//};

	// Make sure these paths remain in sync with the associated enumerators (see TEXTURE_ASSET_ID).
	const std::array<std::string, texture_count> texture_paths = {
		// Player Animations
		textures_path("player/player_idle-Sheet.png"),
		textures_path("player/player_run-Sheet.png"),
		textures_path("player/player_death-Sheet.png"),
		textures_path("player/player_respawn-Sheet.png"),

		textures_path("particles/player_dash_particle.png"),

		// Tile sheets
		textures_path("environment/floor_tiles.png"),
		textures_path("environment/wall_tiles.png"),
		textures_path("environment/chest.png"),
		textures_path("environment/next_level_entry.png"),
		textures_path("environment/1x1_boxes.png"),
		textures_path("environment/door_tile.png"),

		// Enemy Animations
		textures_path("enemies/ranged/ranged_enemy_idle-Sheet.png"),
		textures_path("enemies/ranged/ranged_enemy_run-Sheet.png"),
		textures_path("enemies/shotgun/shotgun_enemy_idle-Sheet.png"),
		textures_path("enemies/shotgun/shotgun_enemy_run-Sheet.png"),
		textures_path("enemies/melee/melee_enemy_idle-Sheet.png"),
		textures_path("enemies/melee/melee_enemy_run-Sheet.png"),
		textures_path("enemies/dummy_enemy.png"),
		textures_path("enemies/turret_enemy.png"),

		textures_path("enemies/boss_1/boss_1_idle-Sheet.png"),
		textures_path("enemies/boss_1/boss_1_run-Sheet.png"),

		// Projectiles
		textures_path("projectiles/fireball.png"),
		textures_path("projectiles/lightning_bolt.png"),
		textures_path("projectiles/shotgun_pellet.png"),
		textures_path("projectiles/thorn_bomb.png"),
		textures_path("projectiles/thorn.png"),
		textures_path("projectiles/boomerang.png"),
		textures_path("projectiles/magnet.png"),
		textures_path("projectiles/lightning.png"),
		textures_path("projectiles/acid.png"),
		textures_path("projectiles/acid_effect.png"),
		textures_path("projectiles/cutter.png"),

		// Enemy projectiles
		textures_path("projectiles/red_orb.png"),

		// Movement spell icons
		textures_path("spell_icons/dash_spell_icon.png"),
		textures_path("spell_icons/blink_spell_icon.png"),

		// Relic Icons
		textures_path("relics/strength_relic_icon.png"),
		textures_path("relics/time_relic_icon.png"),
		textures_path("relics/speed_relic_icon.png"),
		textures_path("relics/number_relic_icon.png"),

		// Interactables
		textures_path("interactables/spell_item.png"),
		textures_path("interactables/relic_item.png"),
		textures_path("interactables/health_restore.png"),
		textures_path("interactables/fountain-Sheet.png"),
		textures_path("interactables/fountain_empty.png"),
		textures_path("interactables/sacrifice_fountain-Sheet.png"),
		textures_path("interactables/sacrifice_fountain_empty.png"),

		// Shadow
		textures_path("shadow.png"),
		textures_path("enemies/enemy_spawn_indicator.png"),

		// Particle
		textures_path("particles/particle_pixel.png"),

		// Player HUD
		textures_path("player_hud/heart_container-Sheet.png"),
		textures_path("player_hud/life_bar_bottom.png"),
		textures_path("player_hud/life_bar_middle.png"),
		textures_path("player_hud/life_bar_fill.png"),
		textures_path("player_hud/spell_container_ui.png"),

		textures_path("temp_text/E.png"),
		textures_path("temp_text/EQ.png"),
		textures_path("temp_text/LMB.png"),
		textures_path("temp_text/Lshift.png"),

		textures_path("temp_text/TUTORIAL.png"),
		textures_path("temp_text/START.png"),
		
		// NPCs
		textures_path("npcs/old_man/old_man_idle-Sheet.png"),
		textures_path("npcs/shop_keeper/shop_keeper_idle-Sheet.png"),

		// Minimap
		textures_path("minimap/500x500Minimap.png"),
		textures_path("minimap/player_head.png"),

		textures_path("cutscenes/black_pixel.png"),

		textures_path("cutscenes/intro_1.png"),
		textures_path("cutscenes/intro_2.png"),
		textures_path("cutscenes/intro_3.png"),
		textures_path("cutscenes/intro_4.png"),
		textures_path("cutscenes/outro_1.png"),
		textures_path("cutscenes/outro_2.png"),
		// Mark: texture for slider
		textures_path("slider/bar.png"),
		textures_path("slider/slide_block.png"),
	};

	std::array<GLuint, effect_count> effects;
	// Make sure these paths remain in sync with the associated enumerators.
	const std::array<std::string, effect_count> effect_paths = {
		shader_path("coloured"),
		shader_path("egg"),
		shader_path("chicken"),
		shader_path("textured"),
		shader_path("environment"),
		shader_path("vignette"),
		shader_path("particle"),
		shader_path("animated"),
		shader_path("outline"),
		shader_path("animated_outline"),
		shader_path("health_bar"),
		shader_path("tile"),
		shader_path("font"),
		shader_path("minimap"),
	};

	std::array<GLuint, geometry_count> vertex_buffers;
	std::array<GLuint, geometry_count> index_buffers;
	std::array<GLuint, geometry_count> instance_buffers;
	std::array<Mesh, geometry_count> meshes;

private:
	GLuint transform_vbo = 0;
	GLuint color_vbo = 0;

	GLuint m_vao;

public:

	// Initialize the window
	bool init(GLFWwindow* window);

	template <class T>
	void bindVBOandIBO(GEOMETRY_BUFFER_ID gid, std::vector<T> vertices, std::vector<uint16_t> indices);

	void initializeGlTextures();

	void initializeGlEffects();

	void initializeGlMeshes();

	void initializeFonts(GLFWwindow* window, const std::string& font_filename, unsigned int font_default_size);
	GLuint m_font_VAO;
	GLuint m_font_VBO;

	Mesh& getMesh(GEOMETRY_BUFFER_ID id) { return meshes[(int)id]; };

	void initializeGlGeometryBuffers();

	// Initialize the screen texture used as intermediate render target
	// The draw loop first renders to this texture, then it is used for the vignette shader
	bool initScreenTexture();

	// Destroy resources associated to one or all entities created by the system
	~RenderSystem();

	// Draw all entities
	void draw(GAME_SCREEN_ID game_screen);

	mat3 createProjectionMatrix();
	mat3 createScreenMatrix();

	Entity get_screen_state_entity() { return screen_state_entity; }

	// Guo: physics_system needs to get texture dimensions for correct bounding box
	ivec2 getTextureDimensions(int texture_id) { 
		return texture_dimensions[texture_id]; 
	} 

private:
	// Internal drawing functions for each entity type
	void drawTiles(std::vector<Entity> entities, TEXTURE_ASSET_ID texture_asset_id, const mat3& projection);

	void drawEnvironment(const mat3& projection);
	void drawGridLine(Entity entity, const mat3& projection);
	void drawTexturedMesh(Entity entity, const mat3& projection);
	void drawToScreen();
	// NEW
	void drawHealthBars();
	void drawHealthBarSegment(vec2 entityPos, float w, float h, 
                                        vec3 color, float offsetX, float offsetY, float depth,
                                        GLuint program, const mat3& projection);
	void drawParticles(const mat3& projection);

	void drawIconOnInteractable(Entity entity, const mat3& projection);

	void drawShadow(Entity entity, const mat3& projection);
	void drawWithShadow(Entity entity, const mat3& projection);

	void drawPlayerHUD();
	void drawFloorGoals(const mat3 projection);
	void drawHealthPlayerBar();
	void drawSpellUI(TEXTURE_ASSET_ID asset_id, vec2 screen_position);
	void drawMinimap();

	void drawText(std::string text, const glm::vec3& color, Transform trans, const glm::mat3& projection, float alpha = 1.0f, TEXT_PIVOT pivot = TEXT_PIVOT::LEFT);
	std::map<char, Character> m_ftCharacters;

	std::vector<Entity> ySort(std::vector<Entity> entities);
	bool isFrustumCulled(Entity entity);

	// Window handle
	GLFWwindow* window;

	// Screen texture handles
	GLuint frame_buffer;
	GLuint off_screen_render_buffer_color;
	GLuint off_screen_render_buffer_depth;

	Entity screen_state_entity;
};

bool loadEffectFromFile(
	const std::string& vs_path, const std::string& fs_path, GLuint& out_program);
