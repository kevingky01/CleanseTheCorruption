#pragma once

#include "common.hpp"
#include "tinyECS/tiny_ecs.hpp"
#include "render_system.hpp"
#include "spells.hpp"
#include <functional> // for function callbacks
#include "enemy_types/enemy_components.hpp"


Entity createPlayer(RenderSystem* renderer, vec2 position);
 
Entity createRandomEnemy(RenderSystem* renderer, vec2 position);

Entity createEnemy(RenderSystem* renderer, vec2 position, ENEMY_TYPE type);

Entity createProjectile(RenderSystem* renderer, vec2 spawn_position, vec2 direction, float speed, PROJECTILE_SPELL_ID spell_id, Entity entity_type, ParticleEmitter particle_emitter);
 
Entity createCamera(RenderSystem* renderer, vec2 position);

Entity createTimer(float duration, std::function<void()> callable, bool is_looping = false);

Entity createTween(float duration, std::function<void()> callable, float* f_value, float from, float to, std::function<float(float, float, float)> interp_func = lerp);

Entity createTween(float duration, std::function<void()> callable, vec2* v2_value, vec2 from, vec2 to, std::function<float(float, float, float)> interp_func = lerp);

Interactable buildInteractableComponent(INTERACTABLE_ID id, int spell_id, int heal_amount, int relic_id);

Entity createInteractableDrop(RenderSystem* renderer, vec2 position, Interactable interactable);

Entity createProjectileSpellDrop(RenderSystem* renderer, vec2 position, PROJECTILE_SPELL_ID spell_id);

Entity createMovementSpellDrop(RenderSystem* renderer, vec2 position, MOVEMENT_SPELL_ID spell_id);

Entity createHealthRestore(RenderSystem* renderer, vec2 position, int heal_amount);

Entity createNextLevelEntry(RenderSystem* renderer, vec2 position);

void update_window_caption(); 

int getFPS();
Entity createNextLevelEntry(RenderSystem* renderer, vec2 position, GAME_SCREEN_ID game_screen_id);

Entity createFloorDecor(RenderSystem* renderer, vec2 position, TEXTURE_ASSET_ID asset_id);

Entity createRandomRelic(RenderSystem* renderer, vec2 position);

Entity createRelic(RenderSystem* renderer, vec2 position, RELIC_ID relic_id);

Entity createChestWithRandomLoot(RenderSystem* renderer, vec2 position);

Entity createChest(RenderSystem* renderer, vec2 position, INTERACTABLE_ID type);

Entity createChest(RenderSystem* renderer, vec2 position, Interactable interactable);

Entity createEnemySpawnIndicator(RenderSystem* renderer, vec2 position, ENEMY_TYPE enemy_type);

Entity createBossMinionSpawnIndicator(RenderSystem* renderer, vec2 position);

Entity createHealingFountain(RenderSystem* renderer, vec2 position, int heal_amount);

Entity createSacrificeFountain(RenderSystem* renderer, vec2 position);

Enemy* buildEnemyOfType(ENEMY_TYPE type, Entity& e);
Entity createDestructableBox(RenderSystem* renderer, vec2 position);

Entity createGoalManager();
Entity createDisplayableText(std::string text_info, vec3 color, vec2 scaling, float rotation, 
	vec2 translation, bool in_screen, bool middle_align);


Entity createTextPopup(std::string text, vec3 color, float alphaa, vec2 scale, float rotation, vec2 translation, bool in_screen, bool is_moving = true);

Entity createAnnouncement(std::string text, vec3 color, float alpha);

Entity createNPC(RenderSystem* renderer, vec2 position, NPC_NAME npc_name);

Entity createDialogue(std::string text, vec3 color, vec2 translation, vec2 scale = vec2(0.5), bool in_screen = false, float alpha = 0);

Entity createMinimap();

Entity createBackgroundImage(RenderSystem* renderer, vec2 position, TEXTURE_ASSET_ID asset_id, vec2 scale = vec2(1, 1));

Entity createCutsceneDialogue(RenderSystem* renderer, std::vector<std::string> texts);
Entity createMinimap();

Entity createBar(RenderSystem* renderer, vec2 pos);

Entity createSlideBlock(RenderSystem* renderer, vec2 pos);

void createSlider(RenderSystem* renderer, vec2 pos, float percentage);
