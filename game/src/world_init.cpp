#pragma once

#include "world_init.hpp"
#include "tinyECS/registry.hpp"
#include "tinyECS/components.hpp"
#include "spells.hpp"
#include "render_system.hpp"
#include <iostream>
#include <enemy_types/enemy_components.hpp>
#include "dialogue/dialogue.hpp"
#include <map>
#include <vector>



Interactable buildInteractableComponent(INTERACTABLE_ID id, int spell_id, int heal_amount, int relic_id) {
	Interactable interactable = Interactable();
	interactable.interactable_id = id;
	interactable.spell_id = spell_id;
	interactable.heal_amount = heal_amount;
	interactable.relic_id = relic_id;

	return interactable;
}


Entity createPlayer(RenderSystem* renderer, vec2 position)
{
	// reserve an entity
	auto entity = Entity();

	// Player
	Player& player = registry.players.emplace(entity);
	// initialize player properties... eg. health or whatever

	// store a reference to the potentially re-used mesh object
	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	// Initialize Position
	auto& transform_comp = registry.transforms.emplace(entity);
	transform_comp.angle = 0.f;
	transform_comp.position = position;

	// Initialize Motion
	auto& motion = registry.motions.emplace(entity);
	motion.velocity = { 0, 0 };
	
	// Creating player hitbox
	Hitbox player_hitbox;
	player_hitbox.layer = (int)COLLISION_LAYER::PLAYER;
	player_hitbox.mask = (int)COLLISION_MASK::WALL | (int)COLLISION_MASK::E_PROJECTILE | (int)COLLISION_MASK::ENEMY;
	player_hitbox.hitbox_scale = vec2(10, 20); // Munn: This is manually set for now since the player's texture has extra pixels on every side
	registry.hitboxes.emplace(entity, player_hitbox);

	// Munn: We will most likely need to scale pixel textures up to fit properly in the window 
	transform_comp.scale = vec2(1, 1);
 

	registry.renderRequests.insert(
		entity,
		{
			TEXTURE_ASSET_ID::PLAYER_IDLE, 
			EFFECT_ASSET_ID::ANIMATED,
			GEOMETRY_BUFFER_ID::ANIMATED_SPRITE
		}
	);
	
	// Create animation manager
	AnimationManager& animation_manager = registry.animation_managers.emplace(entity);

	// Create Idle and Run animations
	Animation idle_animation = Animation(TEXTURE_ASSET_ID::PLAYER_IDLE, 4, true); // Asset id, num_frames, is_looping
	idle_animation.h_frames = 4;
	idle_animation.v_frames = 1;
	idle_animation.frame_rate = ANIMATION_FRAME_RATE / 2;
	Animation run_animation = Animation(TEXTURE_ASSET_ID::PLAYER_RUN, 4, true);
	run_animation.h_frames = 4;
	run_animation.v_frames = 1;
	run_animation.frame_rate = ANIMATION_FRAME_RATE / 2;
	Animation death_animation = Animation(TEXTURE_ASSET_ID::PLAYER_DEATH, 8, false);
	death_animation.h_frames = 4;
	death_animation.v_frames = 2;
	death_animation.frame_rate = ANIMATION_FRAME_RATE / 2;
	death_animation.is_interruptable = false;
	Animation respawn_animation = Animation(TEXTURE_ASSET_ID::PLAYER_RESPAWN, 24, false);
	respawn_animation.h_frames = 4;
	respawn_animation.v_frames = 6;
	respawn_animation.frame_rate = 8;
	respawn_animation.is_interruptable = false;

	// Insert animations into animation_manager
	animation_manager.animations.insert(std::pair(TEXTURE_ASSET_ID::PLAYER_IDLE, idle_animation));
	animation_manager.animations.insert(std::pair(TEXTURE_ASSET_ID::PLAYER_RUN, run_animation));
	animation_manager.animations.insert(std::pair(TEXTURE_ASSET_ID::PLAYER_DEATH, death_animation));
	animation_manager.animations.insert(std::pair(TEXTURE_ASSET_ID::PLAYER_RESPAWN, respawn_animation));

	// Play idle animation
	animation_manager.transition_to(animation_manager.animations[TEXTURE_ASSET_ID::PLAYER_IDLE]);

	//std::cout << "Initializing with id: " << (int)idle_animation.animation_id << std::endl;

	// Create spell slots
	SpellSlotContainer& spellSlotContainer = registry.spellSlotContainers.emplace(entity);

	SpellSlot damageSpellSlot = SpellSlot();
	damageSpellSlot.spell_type = SPELL_TYPE::PROJECTILE;
	damageSpellSlot.spell_id = (int)PROJECTILE_SPELL_ID::FIREBALL; // swap out for different spells

	spellSlotContainer.spellSlots.push_back(damageSpellSlot);

	SpellSlot movementSpellSlot = SpellSlot();
	movementSpellSlot.spell_type = SPELL_TYPE::MOVEMENT;
	movementSpellSlot.spell_id = (int)MOVEMENT_SPELL_ID::DASH;

	spellSlotContainer.spellSlots.push_back(movementSpellSlot);

	// New
	Health& health = registry.healths.emplace(entity);
	health.maxHealth = PLAYER_MAX_HEALTH;
	health.currentHealth = PLAYER_MAX_HEALTH;

	ParticleEmitter footstep_particle_emitter = ParticleEmitter();
	footstep_particle_emitter.setNumParticles(4);
	footstep_particle_emitter.setParentEntity(entity);
	footstep_particle_emitter.setInitialPosition(vec2(0, 10 * PIXEL_SCALE_FACTOR));
	footstep_particle_emitter.setRandomPositionRange(vec2(-3 * PIXEL_SCALE_FACTOR, 0), vec2(3 * PIXEL_SCALE_FACTOR, 0));
	footstep_particle_emitter.setInitialColor(vec4(1, 1, 1, 1));
	

	ParticleEmitter dash_particle_emitter = ParticleEmitter();
	dash_particle_emitter.setNumParticles(8);
	dash_particle_emitter.setParentEntity(entity);
	dash_particle_emitter.setInitialColor(vec4(0.75, 0.75, 1, 0.25));
	dash_particle_emitter.setTextureAssetId(TEXTURE_ASSET_ID::PLAYER_DASH_PARTICLES);

	ParticleEmitterContainer& particle_emitter_container = registry.particle_emitter_containers.emplace(entity);
	particle_emitter_container.particle_emitter_map.insert(std::pair<PARTICLE_EMITTER_ID, ParticleEmitter>(PARTICLE_EMITTER_ID::PLAYER_FOOTSTEPS, footstep_particle_emitter));
	particle_emitter_container.particle_emitter_map.insert(std::pair<PARTICLE_EMITTER_ID, ParticleEmitter>(PARTICLE_EMITTER_ID::DASH_TRAIL, dash_particle_emitter));


	// // Store the mesh based scale of player
	// CollisionMesh& colMesh = registry.collisionMeshes.emplace(entity);
	// 	colMesh.local_points = {
	// 	vec2(0,-16),
	// 	vec2(16,-8),
	// 	vec2(8,-4),
	// 	vec2(8,0),
	// 	vec2(12,0),
	// 	vec2(12,8),
	// 	vec2(8,16),
	// 	vec2(4,16),
	// 	vec2(4,8),
	// 	vec2(-4,8),
	// 	vec2(-4,16),
	// 	vec2(-8,16), 
	// 	vec2(-8,8),
	// 	vec2(-12,8),
	// 	vec2(-12,0),
	// 	vec2(-8,0),
	// 	vec2(-8,-4),
	// 	vec2(-16,-8),
		

    // };
		CollisionMesh& colMesh = registry.collisionMeshes.emplace(entity);
		colMesh.local_points = {
   		 vec2(   0, -10), 
   		 vec2(  +5,  -5), 
   		 vec2(  +5,  +5), 
    	vec2(   0, +10), 
    	 vec2(  -5,  +5), 
    	vec2(  -5,  -5) 
		

    };


	return entity;
}

Entity createRandomEnemy(RenderSystem* renderer, vec2 position) {
	float random_n = uniform_dist(rng);
	ENEMY_TYPE type;

	if (random_n < 0.5) {
		type = BASIC_RANGED_ENEMY;
	}
	else {
		type = BASIC_MELEE_ENEMY;
	}

	return createEnemy(renderer, position, type);
}

Enemy* buildEnemyOfType(ENEMY_TYPE type, Entity& e) {

	switch (type) {
	case BASIC_RANGED_ENEMY:
		return new BasicRangedEnemy(e);
	case SHOTGUN_RANGED_ENEMY:
		return new ShotgunRangedEnemy(e);
	case TOWER_ENEMY:
		return new TowerEnemy(e);
	case BASIC_MELEE_ENEMY:
		return new MeleeEnemy(e);
	case DUMMY_ENEMY:
		return new DummyEnemy(e);
	case BOSS_1:
		return new Boss_1(e);
	
	case BOSS_2:
		return new Boss_2(e); 
	
	

	default:
		return new MeleeEnemy(e);
	}
}

Entity createEnemy(RenderSystem* renderer, vec2 position, ENEMY_TYPE type)
{
	// initialize new entity
	auto entity = Entity();


	// // position
	// auto& transform_comp = registry.transforms.emplace(entity);
	// transform_comp.angle = 0.f;
	// transform_comp.position = position;

	Enemy* typed_enemy_component = buildEnemyOfType(type, entity);
	Enemy* enemy = registry.enemies.insert(entity, typed_enemy_component);

	enemy->renderer = renderer;

	enemy->type = (ENEMY_TYPE)type;

	// store a reference to the potentially re-used mesh object
	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	// !!!
	// position
	auto& transform_comp = registry.transforms.get(entity);
	transform_comp.position = position;

	return entity;
}


Entity createProjectile(RenderSystem* renderer, vec2 spawn_position, vec2 direction, float speed, PROJECTILE_SPELL_ID spell_id, Entity entity_type, ParticleEmitter particle_emitter) {
	// reserve an entity
	auto entity = Entity();

	// Create Projectile
	Projectile& projectile = registry.projectiles.emplace(entity);
	projectile.spell_id = spell_id;
	projectile.owner = entity_type;

	ProjectileSpell* spell = projectile_spells[(int)spell_id];

	projectile.lifetime = spell->getLifetime();

	// store a reference to the potentially re-used mesh object
	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	Transformation& position_comp = registry.transforms.emplace(entity);
	Motion& motion = registry.motions.emplace(entity);
	position_comp.angle = 0.f;
	motion.velocity = glm::normalize(direction) * speed;
	position_comp.position = spawn_position; 

	position_comp.scale = vec2(1, 1);

	registry.renderRequests.insert(
		entity,
		{
			spell->getAssetID(),
			EFFECT_ASSET_ID::TEXTURED,
			GEOMETRY_BUFFER_ID::SPRITE
		}
	);

	// projectile hitbox
	Hitbox projectile_hitbox;

	if (registry.players.has(entity_type)) {
		projectile_hitbox.layer = (int)COLLISION_LAYER::P_PROJECTILE;
		projectile_hitbox.mask = (int)COLLISION_MASK::WALL | (int)COLLISION_MASK::ENEMY;
	}
	else if (registry.enemies.has(entity_type)) {
		projectile_hitbox.layer = (int)COLLISION_LAYER::E_PROJECTILE;
		projectile_hitbox.mask = (int)COLLISION_MASK::WALL | (int)COLLISION_MASK::PLAYER;
	}
	
	/*GLuint spell_asset_id = (GLuint)spell->getAssetID(); // Munn: This causes a read access violation ONLY WHEN it this function is called by the ai_system -- WHY??
	ivec2 dimension = renderer->getTextureDimensions(spell_asset_id);*/
	projectile_hitbox.hitbox_scale = vec2(4, 4);
	//projectile_hitbox.hitbox_scale = dimension;
	registry.hitboxes.emplace(entity, projectile_hitbox);

	particle_emitter.setParentEntity(entity);
	particle_emitter.start_emitting();

	ParticleEmitterContainer& particle_emitter_container = registry.particle_emitter_containers.emplace(entity);
	particle_emitter_container.particle_emitter_map.insert(std::pair<PARTICLE_EMITTER_ID, ParticleEmitter>(PARTICLE_EMITTER_ID::PROJECTILE_TRAIL, particle_emitter));


CollisionMesh& cm = registry.collisionMeshes.emplace(entity);
cm.local_points = {
    // e.g. approximate an arrow shape or a triangle
   		 vec2(   0, -3), 
   		 vec2(  +1.5,  -1.5), 
   		 vec2(  +1.5,  +1.5), 
    	 vec2(   0, +3), 
    	 vec2(  -1.5,  +1.5), 
    	vec2(  -1.5,  -1.5) 
	
};
// cm.local_points = {
//     // e.g. approximate an arrow shape or a triangle
//     vec2( 0, -8), 
//     vec2(12,  8),
//     vec2(-12, 8)
// };
	return entity;
}


Entity createTimer(float duration, std::function<void()> callable, bool is_looping) {
	auto entity = Entity();

	Timer& timer = registry.timers.emplace(entity);

	timer.duration = duration;		// in seconds
	timer.current_time = duration;
	timer.is_active = true;
	timer.timeout = callable;
	timer.is_looping = is_looping;

	return entity;
} 

Entity createTween(float duration, std::function<void()> callable, float* f_value, float from, float to, std::function<float(float, float, float)> interp_func) {
	auto entity = Entity();

	Tween& tween = registry.tweens.emplace(entity);

	tween.duration = duration;
	tween.current_time = duration;
	tween.is_active = true;
	tween.timeout = callable;

	tween.type = TWEEN_TYPE::FLOAT;

	tween.f_value = f_value;
	tween.f_from = from;
	tween.f_to = to;

	tween.interp_func = interp_func; 

	return entity;
}

Entity createTween(float duration, std::function<void()> callable, vec2* v2_value, vec2 from, vec2 to, std::function<float(float, float, float)> interp_func) {
	auto entity = Entity();

	Tween& tween = registry.tweens.emplace(entity);

	tween.duration = duration;
	tween.current_time = duration;
	tween.is_active = true;
	tween.timeout = callable;

	tween.type = TWEEN_TYPE::VEC2;

	tween.v2_value = v2_value;
	tween.v2_from = from;
	tween.v2_to = to;

	std::cout << tween.v2_from.x << ", " << tween.v2_from.y << std::endl;
	std::cout << tween.v2_to.x << ", " << tween.v2_to.y << std::endl;

	tween.interp_func = interp_func;

	return entity;
}



Entity createCamera(RenderSystem* renderer, vec2 position)
{
	// reserve an entity
	auto entity = Entity();

	// Player
	Camera& camera = registry.cameras.emplace(entity); 

	// Initialize Position
	auto& transform_comp = registry.transforms.emplace(entity);
	transform_comp.angle = 0.f;
	transform_comp.position = position;

	// Initialize Motion
	auto& motion = registry.motions.emplace(entity);
	motion.velocity = { 0, 0 };

	// Munn: We will most likely need to scale pixel textures up to fit properly in the window 
	transform_comp.scale = vec2(1, 1);

	return entity;
}





const std::unordered_map<INTERACTABLE_ID, TEXTURE_ASSET_ID> interactable_map = {
	{INTERACTABLE_ID::PROJECTILE_SPELL_DROP, TEXTURE_ASSET_ID::SPELL_DROP},
	{INTERACTABLE_ID::MOVEMENT_SPELL_DROP, TEXTURE_ASSET_ID::SPELL_DROP},
	{INTERACTABLE_ID::HEALTH_RESTORE, TEXTURE_ASSET_ID::HEALTH_RESTORE},
	{INTERACTABLE_ID::RELIC_DROP, TEXTURE_ASSET_ID::RELIC_DROP},
	{INTERACTABLE_ID::NEXT_LEVEL_ENTRY, TEXTURE_ASSET_ID::NEXT_LEVEL_ENTRY},
};


Entity createInteractableDrop(RenderSystem* renderer, vec2 position,
	Interactable interactable) {
	Entity entity = Entity();

	TEXTURE_ASSET_ID texture_id = interactable_map.at((INTERACTABLE_ID)interactable.interactable_id);

	registry.renderRequests.insert(
		entity,
		{
			texture_id,
			EFFECT_ASSET_ID::OUTLINE,
			GEOMETRY_BUFFER_ID::SPRITE
		}
	);

	Hitbox& hitbox = registry.hitboxes.emplace(entity);
	hitbox.layer = (int)COLLISION_LAYER::INTERACTABLE;
	hitbox.mask = (int)COLLISION_MASK::PLAYER;
	hitbox.hitbox_scale = renderer->getTextureDimensions((int)texture_id);

	// Initialize Position
	auto& transform_comp = registry.transforms.emplace(entity);
	transform_comp.position = position;

	registry.interactables.insert(entity, interactable);

	return entity;
}



const int chest_projectile_spell_count = 7;
std::array<PROJECTILE_SPELL_ID, chest_projectile_spell_count> chest_projectile_spells = {
	PROJECTILE_SPELL_ID::WATERBALL,
	PROJECTILE_SPELL_ID::THORN_BOMB,
	PROJECTILE_SPELL_ID::BOOMERANG,
	PROJECTILE_SPELL_ID::MAGNET,
	PROJECTILE_SPELL_ID::SHOTGUN,
	PROJECTILE_SPELL_ID::LIGHTNING,
	PROJECTILE_SPELL_ID::CUTTER,

};


Entity createProjectileSpellDrop(RenderSystem* renderer, vec2 position, PROJECTILE_SPELL_ID spell_id) {

	Interactable interactable = buildInteractableComponent(
		INTERACTABLE_ID::PROJECTILE_SPELL_DROP, (int)spell_id, 0, 0);

	return createInteractableDrop(renderer, position, interactable);
}



Entity createMovementSpellDrop(RenderSystem* renderer, vec2 position, MOVEMENT_SPELL_ID spell_id) {

	Interactable interactable = buildInteractableComponent(
		INTERACTABLE_ID::MOVEMENT_SPELL_DROP, (int)spell_id, 0, 0);

	return createInteractableDrop(renderer, position, interactable);
}



Entity createHealthRestore(RenderSystem* renderer, vec2 position, int heal_amount) {

	Interactable interactable = buildInteractableComponent(
		INTERACTABLE_ID::HEALTH_RESTORE, 0, heal_amount, 0);

	return createInteractableDrop(renderer, position, interactable);
}

Entity createChestWithRandomLoot(RenderSystem* renderer, vec2 position) {

	return createChest(renderer, position, INTERACTABLE_ID::PROJECTILE_SPELL_DROP);
}



Entity createChest(RenderSystem* renderer, vec2 position, INTERACTABLE_ID type)
{
	Entity entity = Entity();

	TEXTURE_ASSET_ID texture_id = TEXTURE_ASSET_ID::CHEST;

	registry.renderRequests.insert(
		entity,
		{
			texture_id,
			EFFECT_ASSET_ID::TEXTURED,
			GEOMETRY_BUFFER_ID::SPRITE
		}
	); 

	Interactable interactable;
	if (type == INTERACTABLE_ID::PROJECTILE_SPELL_DROP) {
		float random_n = uniform_dist(rng);
		int index = ((int)random_n == 1) ? 0 : (int)(chest_projectile_spell_count * random_n);
		int projectile_spell_id = (int)chest_projectile_spells[index];
		interactable = buildInteractableComponent(type, projectile_spell_id, 0, 0);
	}
	else if (type == INTERACTABLE_ID::MOVEMENT_SPELL_DROP) {
		float random_n = uniform_dist(rng);
		int movement_spell_id = ((int)random_n == 1) ? 0 : (int)(movement_spell_count * random_n);
		interactable = buildInteractableComponent(type, movement_spell_id, 0, 0);
	}
	else if (type == INTERACTABLE_ID::HEALTH_RESTORE) {
		interactable = buildInteractableComponent(type, 0, 25, 0);
	}
	else if (type == INTERACTABLE_ID::RELIC_DROP) {
		float random_n = uniform_dist(rng);
		int relic_id = ((int)random_n == 1) ? 0 : (int)(relic_count * random_n);
		interactable = buildInteractableComponent(type, 0, 0, relic_id);
	}

	Lootable& lootable = registry.lootables.emplace(entity);
	lootable.drop = interactable;

	registry.chests.emplace(entity);

	Health& chest_health = registry.healths.emplace(entity);
	chest_health.currentHealth = CHEST_HEALTH;
	chest_health.maxHealth = CHEST_HEALTH;

	auto& transform_comp = registry.transforms.emplace(entity);
	transform_comp.position = position;
	transform_comp.scale = vec2(1.5, 1.5);

	Hitbox& hitbox = registry.hitboxes.emplace(entity);
	hitbox.layer = (int)COLLISION_LAYER::ENEMY;
	hitbox.mask = (int)COLLISION_MASK::WALL | (int)COLLISION_MASK::PLAYER;
	hitbox.hitbox_scale = renderer->getTextureDimensions((int)texture_id);

	//Interactable& interactable = registry.interactables.emplace(entity);
	//interactable.interactable_id = INTERACTABLE_ID::NEXT_LEVEL_ENTRY;

	return entity;
}



Entity createChest(RenderSystem* renderer, vec2 position, Interactable interactable)
{
	Entity entity = Entity();

	TEXTURE_ASSET_ID texture_id = TEXTURE_ASSET_ID::CHEST;

	registry.renderRequests.insert(
		entity,
		{
			texture_id,
			EFFECT_ASSET_ID::TEXTURED,
			GEOMETRY_BUFFER_ID::SPRITE
		}
	); 

	Lootable& lootable = registry.lootables.emplace(entity);
	lootable.drop = interactable;

	registry.chests.emplace(entity);

	Health& chest_health = registry.healths.emplace(entity);
	chest_health.currentHealth = CHEST_HEALTH;
	chest_health.maxHealth = CHEST_HEALTH;

	auto& transform_comp = registry.transforms.emplace(entity);
	transform_comp.position = position;
	transform_comp.scale = vec2(1.5, 1.5);

	Hitbox& hitbox = registry.hitboxes.emplace(entity);
	hitbox.layer = (int)COLLISION_LAYER::ENEMY;
	hitbox.mask = (int)COLLISION_MASK::WALL | (int)COLLISION_MASK::PLAYER;
	hitbox.hitbox_scale = renderer->getTextureDimensions((int)texture_id); 

	return entity;
}


Entity createNextLevelEntry(RenderSystem* renderer, vec2 position, GAME_SCREEN_ID game_screen_id)
{
	Entity entity = Entity();

	TEXTURE_ASSET_ID texture_id = TEXTURE_ASSET_ID::NEXT_LEVEL_ENTRY;

	registry.renderRequests.insert(
		entity,
		{
			texture_id,
			EFFECT_ASSET_ID::OUTLINE,
			GEOMETRY_BUFFER_ID::SPRITE
		}
	);

	Hitbox& hitbox = registry.hitboxes.emplace(entity);
	hitbox.layer = (int)COLLISION_LAYER::INTERACTABLE;
	hitbox.mask = (int)COLLISION_MASK::PLAYER;
	hitbox.hitbox_scale = renderer->getTextureDimensions((int)texture_id);

	Transformation& transform_comp = registry.transforms.emplace(entity);
	transform_comp.position = position;

	Interactable& interactable = registry.interactables.emplace(entity);
	interactable.interactable_id = INTERACTABLE_ID::NEXT_LEVEL_ENTRY;

	interactable.game_screen_id = (int)game_screen_id;

	return entity;
}

Entity createRandomRelic(RenderSystem* renderer, vec2 position) {
	RELIC_ID relic_id;
	do {

		relic_id = (RELIC_ID)(uniform_dist(rng) * relic_count);
	} while (relic_id == RELIC_ID::NUMBERS);

	return createRelic(renderer, position, relic_id);
}

Entity createRelic(RenderSystem* renderer, vec2 position, RELIC_ID relic_id) {
	Entity entity = Entity();

	TEXTURE_ASSET_ID texture_id = TEXTURE_ASSET_ID::RELIC_DROP;

	registry.renderRequests.insert(
		entity,
		{
			texture_id,
			EFFECT_ASSET_ID::OUTLINE,
			GEOMETRY_BUFFER_ID::SPRITE
		}
	);


	Hitbox& hitbox = registry.hitboxes.emplace(entity);
	hitbox.layer = (int)COLLISION_LAYER::INTERACTABLE;
	hitbox.mask = (int)COLLISION_MASK::PLAYER;
	hitbox.hitbox_scale = renderer->getTextureDimensions((int)texture_id);

	// Initialize Position
	auto& transform_comp = registry.transforms.emplace(entity);
	transform_comp.position = position;

	Interactable& interactable = registry.interactables.emplace(entity);
	interactable.interactable_id = INTERACTABLE_ID::RELIC_DROP;

	interactable.relic_id = (int)relic_id;


	return entity;
}


Entity createFloorDecor(RenderSystem* renderer, vec2 position, TEXTURE_ASSET_ID asset_id)
{
	Entity entity = Entity(); 

	registry.renderRequests.insert(
		entity,
		{
			asset_id,
			EFFECT_ASSET_ID::OUTLINE,
			GEOMETRY_BUFFER_ID::SPRITE
		}
	);

	Transformation& transform_comp = registry.transforms.emplace(entity);
	transform_comp.position = position; 

	registry.floorDecors.emplace(entity);

	return entity;
}

const float SPAWN_INDICATOR_DURATION = 1.0;
Entity createEnemySpawnIndicator(RenderSystem* renderer, vec2 position, ENEMY_TYPE enemy_type) {
	Entity entity = Entity();

	registry.renderRequests.insert(
		entity,
		{
			TEXTURE_ASSET_ID::ENEMY_SPAWN_INDICATOR,
			EFFECT_ASSET_ID::TEXTURED,
			GEOMETRY_BUFFER_ID::SPRITE
		}
	);

	Transformation& transform_comp = registry.transforms.emplace(entity);
	transform_comp.position = position;

	registry.floorDecors.emplace(entity);

	std::function<void()> spawnEnemy = [renderer, position, entity, enemy_type]() { // pass in motion as a reference 
		// Create entity 
		createEnemy(renderer, position, enemy_type);

		// Remove indicator
		registry.remove_all_components_of(entity);
	};

	createTimer(SPAWN_INDICATOR_DURATION, spawnEnemy);

	return entity;
}

Entity createBossMinionSpawnIndicator(RenderSystem* renderer, vec2 position) {
	Entity entity = Entity();

	registry.renderRequests.insert(
		entity,
		{
			TEXTURE_ASSET_ID::ENEMY_SPAWN_INDICATOR,
			EFFECT_ASSET_ID::OUTLINE,
			GEOMETRY_BUFFER_ID::SPRITE
		}
	);

	Transformation& transform_comp = registry.transforms.emplace(entity);
	transform_comp.position = position;

	registry.floorDecors.emplace(entity);

	std::function<void()> spawnEnemy = [renderer, position, entity]() { // pass in motion as a reference 
		// Create entity
		Entity enemy = createRandomEnemy(renderer, position);

		Health& health = registry.healths.get(enemy);
		health.currentHealth = 10;

		if (registry.lootables.has(enemy)) {
			registry.lootables.remove(enemy);
		}

		// Remove indicator
		registry.remove_all_components_of(entity);
	};

	createTimer(SPAWN_INDICATOR_DURATION, spawnEnemy);

	return entity;
}


Entity createHealingFountain(RenderSystem* renderer, vec2 position, int heal_amount) {
	Entity entity = Entity(); 

	registry.renderRequests.insert(
		entity,
		{
			TEXTURE_ASSET_ID::FOUNTAIN,
			EFFECT_ASSET_ID::ANIMATED_OUTLINE,
			GEOMETRY_BUFFER_ID::SPRITE
		}
	);

	// Initialize Position
	auto& transform_comp = registry.transforms.emplace(entity);
	transform_comp.position = position; 

	Interactable interactable;
	interactable.interactable_id = INTERACTABLE_ID::FOUNTAIN;
	interactable.heal_amount = heal_amount;
	registry.interactables.insert(entity, interactable);

	AnimationManager& animation_manager = registry.animation_managers.emplace(entity);

	// Create Idle and Run animations
	Animation idle_animation = Animation(TEXTURE_ASSET_ID::FOUNTAIN, 8, true); // Asset id, num_frames, is_looping
	idle_animation.h_frames = 4;
	idle_animation.v_frames = 2;
	idle_animation.frame_rate = ANIMATION_FRAME_RATE / 2; 

	// Insert animations into animation_manager
	animation_manager.animations.insert(std::pair(TEXTURE_ASSET_ID::FOUNTAIN, idle_animation));

	// Play idle animation
	animation_manager.transition_to(animation_manager.animations[TEXTURE_ASSET_ID::FOUNTAIN]);



	Hitbox& hitbox = registry.hitboxes.emplace(entity);
	hitbox.layer = (int)COLLISION_LAYER::INTERACTABLE;
	hitbox.mask = (int)COLLISION_MASK::PLAYER;
	hitbox.hitbox_scale = vec2(
		renderer->getTextureDimensions((int)TEXTURE_ASSET_ID::FOUNTAIN).x / (float)idle_animation.h_frames,
		renderer->getTextureDimensions((int)TEXTURE_ASSET_ID::FOUNTAIN).y / (float)idle_animation.v_frames / 2.0f
	); 

	return entity;
}

Entity createSacrificeFountain(RenderSystem* renderer, vec2 position) {
	Entity entity = Entity();

	registry.renderRequests.insert(
		entity,
		{
			TEXTURE_ASSET_ID::SACRIFICE_FOUNTAIN,
			EFFECT_ASSET_ID::ANIMATED_OUTLINE,
			GEOMETRY_BUFFER_ID::SPRITE
		}
	);

	// Initialize Position
	auto& transform_comp = registry.transforms.emplace(entity);
	transform_comp.position = position;

	Interactable interactable;
	interactable.interactable_id = INTERACTABLE_ID::SACRIFICE_FOUNTAIN; 
	registry.interactables.insert(entity, interactable);

	AnimationManager& animation_manager = registry.animation_managers.emplace(entity);

	// Create Idle and Run animations
	Animation idle_animation = Animation(TEXTURE_ASSET_ID::SACRIFICE_FOUNTAIN, 8, true); // Asset id, num_frames, is_looping
	idle_animation.h_frames = 4;
	idle_animation.v_frames = 2;
	idle_animation.frame_rate = ANIMATION_FRAME_RATE / 2;

	// Insert animations into animation_manager
	animation_manager.animations.insert(std::pair(TEXTURE_ASSET_ID::SACRIFICE_FOUNTAIN, idle_animation));

	// Play idle animation
	animation_manager.transition_to(animation_manager.animations[TEXTURE_ASSET_ID::SACRIFICE_FOUNTAIN]);



	Hitbox& hitbox = registry.hitboxes.emplace(entity);
	hitbox.layer = (int)COLLISION_LAYER::INTERACTABLE;
	hitbox.mask = (int)COLLISION_MASK::PLAYER;
	hitbox.hitbox_scale = vec2(
		renderer->getTextureDimensions((int)TEXTURE_ASSET_ID::FOUNTAIN).x / (float)idle_animation.h_frames,
		renderer->getTextureDimensions((int)TEXTURE_ASSET_ID::FOUNTAIN).y / (float)idle_animation.v_frames / 2.0f
	);

	return entity;
}


Entity createDestructableBox(RenderSystem* renderer, vec2 position) {
	// initialize new entity
	auto entity = Entity();

	// store a reference to the potentially re-used mesh object
	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh); 
	
	registry.renderRequests.insert(
		entity,
		{
			TEXTURE_ASSET_ID::BOXES,
			EFFECT_ASSET_ID::TILE,
			GEOMETRY_BUFFER_ID::SPRITE
		}
	);

	Tile& tile = registry.tiles.emplace(entity);
	tile.h_tiles = 4;
	tile.v_tiles = 4;
	tile.tilecoord = vec2(
		(int)(uniform_dist(rng) * tile.h_tiles),
		(int)(uniform_dist(rng) * tile.v_tiles)
	);

	registry.environmentObjects.emplace(entity);

	// position
	auto& transform_comp = registry.transforms.emplace(entity);
	transform_comp.angle = 0.f;
	transform_comp.position = position;

	Hitbox& hitbox = registry.hitboxes.emplace(entity);
	hitbox.layer = (int)COLLISION_LAYER::WALL;
	hitbox.mask = (int)COLLISION_MASK::PLAYER | (int)COLLISION_MASK::ENEMY | (int)COLLISION_MASK::E_PROJECTILE | (int)COLLISION_MASK::P_PROJECTILE;
	hitbox.hitbox_scale = vec2(16); // HARD_CODED FOR NOW

	Health& health = registry.healths.emplace(entity);
	health.maxHealth = 20;
	health.currentHealth = health.maxHealth;

	return entity;
}


Entity createGoalManager() {
	Entity entity = Entity();
	registry.goalManagers.emplace(entity);
	
	return entity;
}


Entity createDisplayableText(std::string text_info, vec3 color, vec2 scaling, float rotation, 
	vec2 translation, bool in_screen, bool middle_align) {
	Entity entity = Entity();

	Text& text_entity = registry.texts.emplace(entity);

	float middle_alignment = -((text_info.length() * FONT_SIZE * scaling.x - 5)/ 2.7f);

	text_entity.text = text_info;
	text_entity.color = color;
	text_entity.scale = scaling;
	text_entity.rotation = rotation;
	text_entity.translation = middle_align ? translation + vec2(middle_alignment, 0) : translation;
	//text_entity.translation = translation;
	text_entity.in_screen = in_screen;

	// might be changed later, hard-coded
	/*
	text_entity.mouse_detection_box_start = vec2(
		translation.x * scaling.x, 
		(translation.y - 55) * scaling.y
	);
	text_entity.mouse_detection_box_end = vec2(
		WINDOW_WIDTH_PX - translation.x * scaling.x,
		translation.y * scaling.y
	);
	*/
	text_entity.mouse_detection_box_start = vec2(
		text_entity.translation.x * scaling.x,
		(text_entity.translation.y - 55) * scaling.y
	);
	text_entity.mouse_detection_box_end = 
		vec2(middle_align ? 
			WINDOW_WIDTH_PX - text_entity.translation.x * scaling.x : 
			(text_entity.translation.x + text_info.length() * FONT_SIZE) * scaling.x,
		text_entity.translation.y * scaling.y);

	text_entity.mouse_pressing_color = vec3(0.5 * color.x, 0.5 * color.y, 0.5 * color.z);
	text_entity.mouse_pressing_translation = vec2(text_entity.translation.x + 5, text_entity.translation.y + 5);

	registry.renderRequests.insert(
		entity,
		{
			TEXTURE_ASSET_ID::TEXTURE_COUNT,
			EFFECT_ASSET_ID::FONT,
			GEOMETRY_BUFFER_ID::FONT
		}
	);

	return entity;
}



const float TRANSLATE_DURATION = 0.50;
const float FADE_OUT_DURATION = 0.25;
const float Y_OFFSET = 60;
const float RAND_X_OFFSET_RANGE = 20;
const float RAND_Y_OFFSET_RANGE = 20;
Entity createTextPopup(std::string text, vec3 color, float alpha, vec2 scale, float rotation, vec2 translation, bool in_screen, bool is_moving) {
	Entity entity = Entity();

	TextPopup& text_popup = registry.textPopups.emplace(entity);
	text_popup.text = text;
	text_popup.color = color;
	text_popup.alpha = new float(alpha);
	text_popup.scale = scale;
	text_popup.rotation = rotation;

	float x_offset = -(text.length() * FONT_SIZE * scale.x / 3.0f); // Divide by 3 is just an estimation of where it should end up, it is not perfect
	
	text_popup.translation = new vec2(translation + vec2(x_offset, 0));
	text_popup.in_screen = in_screen; 

	std::function<void()> remove_timeout = [entity]() {
		if (!registry.textPopups.has(entity)) {
			return;
		}
		TextPopup& text_popup = registry.textPopups.get(entity);
		delete text_popup.alpha;
		delete text_popup.translation; 
		registry.remove_all_components_of(entity);
		};
	std::function<void()> transform_timeout = [remove_timeout, entity]() {
		if (!registry.textPopups.has(entity)) {
			return;
		}
		TextPopup& text_popup = registry.textPopups.get(entity);

		createTween(FADE_OUT_DURATION, remove_timeout, text_popup.alpha, 1.0f, 0.0f, cubic_interp);
		};
	 
	float rand_x = uniform_dist(rng) * RAND_X_OFFSET_RANGE * 2 - (RAND_X_OFFSET_RANGE);
	float rand_y = uniform_dist(rng) * RAND_Y_OFFSET_RANGE * 2 - (RAND_Y_OFFSET_RANGE);

	vec2 original_pos = *text_popup.translation;
	vec2 new_pos = *text_popup.translation + vec2(rand_x, -Y_OFFSET + rand_y);
	if (!is_moving) {
		new_pos = original_pos;
	}

	createTween(TRANSLATE_DURATION, transform_timeout, text_popup.translation, original_pos, new_pos, cubic_interp);

	return entity;
}
 
const vec2 ANNOUNCEMENT_SCALE = vec2(1.5, 1.5);
const float ANNOUNCEMENT_ROTATION = 0;
const vec2 ANNOUNCEMENT_TRANSLATION = vec2(WINDOW_WIDTH_PX / 2.0f, WINDOW_HEIGHT_PX / 4.0f);

const float WAIT_DURATION = 2.5f;
const float ANNOUNCEMENT_FADE_DURATION = 1.0f;
Entity createAnnouncement(std::string text, vec3 color, float alpha) {
	Entity entity = Entity();

	TextPopup& text_popup = registry.textPopups.emplace(entity);
	text_popup.text = text;
	text_popup.color = color;
	text_popup.alpha = new float(alpha);
	text_popup.scale = ANNOUNCEMENT_SCALE;
	text_popup.rotation = ANNOUNCEMENT_ROTATION;

	float x_offset = -(text.length() * FONT_SIZE * ANNOUNCEMENT_SCALE.x / 3.0f); // Divide by 3 is just an estimation of where it should end up, it is not perfect

	text_popup.translation = new vec2(ANNOUNCEMENT_TRANSLATION + vec2(x_offset, 0));
	text_popup.in_screen = true;

	std::function<void()> remove_timeout = [entity]() {
		if (!registry.textPopups.has(entity)) {
			return;
		}
		TextPopup& text_popup = registry.textPopups.get(entity);
		delete text_popup.alpha;
		delete text_popup.translation;
		registry.remove_all_components_of(entity);
		};
	std::function<void()> wait_timeout = [remove_timeout, entity]() {
		if (!registry.textPopups.has(entity)) {
			return;
		}
		TextPopup& text_popup = registry.textPopups.get(entity);

		createTween(ANNOUNCEMENT_FADE_DURATION, remove_timeout, text_popup.alpha, 1.0f, 0.0f, cubic_interp);
		}; 

	createTimer(WAIT_DURATION, wait_timeout);

	return entity;
}
 
Entity createMinimap() {
	Entity entity = Entity();

	registry.minimaps.emplace(entity);

	Transformation& transform = registry.transforms.emplace(entity);
	transform.position = vec2(WINDOW_WIDTH_PX - 150, 150);
	transform.scale = vec2(0.15);

	registry.renderRequests.insert(
		entity,
		{
			TEXTURE_ASSET_ID::MINIMAP,
			EFFECT_ASSET_ID::MINIMAP,
			GEOMETRY_BUFFER_ID::SPRITE
		}
	);

	return entity;
}

Entity createNPC(RenderSystem* renderer, vec2 position, NPC_NAME npc_name) {
	// reserve an entity
	auto entity = Entity();


	// store a reference to the potentially re-used mesh object
	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	// Initialize Position
	auto& transform_comp = registry.transforms.emplace(entity);
	transform_comp.angle = 0.f;
	transform_comp.position = position;


	NPC& npc = registry.npcs.emplace(entity);
	Entity text_entity = createDialogue("Hello!", vec3(1), position + vec2(0, -PIXEL_SCALE_FACTOR * 8));
	npc.text_entity = text_entity;
	npc.npc_name = npc_name;

	// Choose random conversation
	std::map<NPC_CONVERSATION, std::vector<std::string>> conversations = NPC_DIALOGUES.at(npc_name);

	std::vector<NPC_CONVERSATION> keys;
	for (auto pair : conversations) {
		keys.push_back(pair.first);
	}

	int rand = uniform_dist(rng) * keys.size();
	if (rand == keys.size()) {
		rand = 0;
	}

	NPC_CONVERSATION rand_conversation = keys[rand];

	npc.npc_conversation = rand_conversation;


	// Creating player hitbox
	Hitbox hitbox;
	hitbox.layer = (int)COLLISION_LAYER::INTERACTABLE;
	hitbox.mask = (int)COLLISION_MASK::PLAYER;
	hitbox.hitbox_scale = vec2(24, 24); // Munn: hardcoded
	registry.hitboxes.emplace(entity, hitbox);

	// Munn: We will most likely need to scale pixel textures up to fit properly in the window 
	transform_comp.scale = vec2(1, 1);
	

	TEXTURE_ASSET_ID asset_id = TEXTURE_ASSET_ID::OLD_MAN_IDLE;

	switch (npc_name) {
	case NPC_NAME::OLD_MAN:
		asset_id = TEXTURE_ASSET_ID::OLD_MAN_IDLE;
		break;
	case NPC_NAME::SHOP_KEEPER:
		asset_id = TEXTURE_ASSET_ID::SHOP_KEEPER_IDLE;
		break;
	}



	registry.renderRequests.insert(
		entity,
		{
			asset_id,
			EFFECT_ASSET_ID::ANIMATED_OUTLINE,
			GEOMETRY_BUFFER_ID::ANIMATED_SPRITE
		}
	);

	// Create animation manager
	AnimationManager& animation_manager = registry.animation_managers.emplace(entity);

	// Create Idle and Run animations
	Animation idle_animation = Animation(asset_id, 4, true); // Asset id, num_frames, is_looping
	idle_animation.h_frames = 4;
	idle_animation.v_frames = 1;
	idle_animation.frame_rate = ANIMATION_FRAME_RATE / 2;

	// Insert animations into animation_manager
	animation_manager.animations.insert(std::pair(asset_id, idle_animation));

	// Play idle animation
	animation_manager.transition_to(animation_manager.animations[asset_id]);
	

	Interactable& interactable = registry.interactables.emplace(entity);
	interactable.interactable_id = INTERACTABLE_ID::NPC;

	return entity;
}


Entity createDialogue(std::string text, vec3 color, vec2 translation, vec2 scale, bool in_screen, float alpha) {
	Entity entity = Entity();

	TextPopup& text_popup = registry.textPopups.emplace(entity);
	text_popup.text = text;
	text_popup.color = color;
	text_popup.alpha = new float(alpha);
	text_popup.scale = scale;
	text_popup.pivot = TEXT_PIVOT::CENTER;

	text_popup.translation = new vec2(translation);
	text_popup.in_screen = in_screen;

	return entity;
}

Entity createBackgroundImage(RenderSystem* renderer, vec2 position, TEXTURE_ASSET_ID asset_id, vec2 scale) {
	Entity entity = Entity();

	registry.renderRequests.insert(
		entity,
		{
			asset_id,
			EFFECT_ASSET_ID::TEXTURED,
			GEOMETRY_BUFFER_ID::SPRITE
		}
	);

	Transformation& transform_comp = registry.transforms.emplace(entity);
	transform_comp.position = position;
	transform_comp.scale = scale;

	registry.backgroundImages.emplace(entity);

	return entity;
}


const int TEXT_BOX_HEIGHT = WINDOW_HEIGHT_PX / 3;
Entity createCutsceneDialogue(RenderSystem* renderer, std::vector<std::string> texts) {
	Entity entity = Entity();

	registry.dialogueBoxes.emplace(entity);

	Transformation& transform_comp = registry.transforms.emplace(entity);
	transform_comp.position = vec2(WINDOW_WIDTH_PX / 2, WINDOW_HEIGHT_PX - TEXT_BOX_HEIGHT / 2);
	transform_comp.scale = vec2(WINDOW_WIDTH_PX, TEXT_BOX_HEIGHT) / (float)PIXEL_SCALE_FACTOR;

	registry.renderRequests.insert(
		entity,
		{
			TEXTURE_ASSET_ID::BLACK_PIXEL,
			EFFECT_ASSET_ID::TEXTURED,
			GEOMETRY_BUFFER_ID::SPRITE
		}
	);

	vec2 text_box_middle = vec2(WINDOW_WIDTH_PX / 2, WINDOW_HEIGHT_PX - TEXT_BOX_HEIGHT / 2);

	const int TEXT_HEIGHT = FONT_SIZE + 8;

	int current_y_offset = -((texts.size() - 1) * TEXT_HEIGHT / 2);

	for (int i = 0; i < texts.size(); i++) {
		Entity text_entity = createDialogue(texts[i], vec3(1), text_box_middle + vec2(0, current_y_offset), vec2(0.75), true);
		TextPopup& text_popup = registry.textPopups.get(text_entity);
		*text_popup.alpha = 1.0;


		current_y_offset += TEXT_HEIGHT;
	}

	Entity text_entity = createDialogue("Press any key to continue", vec3(1), vec2(WINDOW_WIDTH_PX - 8, WINDOW_HEIGHT_PX - 4 - TEXT_HEIGHT / 2), vec2(0.5), true);
	TextPopup& text_popup = registry.textPopups.get(text_entity);
	text_popup.pivot = TEXT_PIVOT::RIGHT;
	*text_popup.alpha = 1.0;

	Entity text_entity2 = createDialogue("Press ESC to skip", vec3(1), vec2(WINDOW_WIDTH_PX - 8, WINDOW_HEIGHT_PX - 8), vec2(0.5), true);
	TextPopup& text_popup2 = registry.textPopups.get(text_entity2);
	text_popup2.pivot = TEXT_PIVOT::RIGHT;
	*text_popup2.alpha = 1.0;

	return entity;

}


Entity createBar(RenderSystem* renderer, vec2 pos) {

	Entity entity = Entity();

	registry.slideBars.emplace(entity);

	auto& transform_comp = registry.transforms.emplace(entity);
	transform_comp.position = pos;
	registry.renderRequests.insert(
		entity,
		{
			TEXTURE_ASSET_ID::SLIDE_BAR,
			EFFECT_ASSET_ID::TEXTURED,
			GEOMETRY_BUFFER_ID::SPRITE
		}
	);

	return entity;
}



Entity createSlideBlock(RenderSystem* renderer, vec2 pos) {

	Entity entity = Entity();

	registry.slideBlocks.emplace(entity);

	auto& transform_comp = registry.transforms.emplace(entity);
	transform_comp.position = pos;

	registry.renderRequests.insert(
		entity,
		{
			TEXTURE_ASSET_ID::SLIDE_BLOCK,
			EFFECT_ASSET_ID::TEXTURED,
			GEOMETRY_BUFFER_ID::SPRITE
		}
	);

	return entity;
}




void createSlider(RenderSystem* renderer, vec2 pos, float percentage) {
	createBar(renderer, pos);
	vec2 block_pos = vec2(pos.x - 200.0f + percentage * 400.0f, pos.y - 2.0f);
	createSlideBlock(renderer, block_pos);
}