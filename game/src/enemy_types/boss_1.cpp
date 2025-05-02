#pragma once
#include "enemy_components.hpp"
#include <tinyECS/components.hpp>
#include <tinyECS/registry.hpp>
#include <spell_cast_manager.hpp>

Boss_1::Boss_1(Entity& entity) {
	max_health = 600;
	speed = 150;
	detection_range = 2000;
	shooting_range = 1500;
	base_recharge_time = 1500;
	hitbox_size = vec2(40, 70);

	col_mesh.local_points = {
		 vec2(0, -10),
		 vec2(+5,  -5),
		 vec2(+5,  +5),
		 vec2(0, +10),
		 vec2(-5,  +5),
		 vec2(-5,  -5)
	};

	time_since_last_state = 0;
	time_since_last_attack = 0;
	radian_rotation = 0;

	std::cout << "MADE BOSS " << std::endl;

	buildComponents(entity);
}

void Boss_1::buildComponents(Entity& self) {

	registry.renderRequests.insert(
		self,
		{
			TEXTURE_ASSET_ID::BOSS_1_IDLE,
			EFFECT_ASSET_ID::ANIMATED,
			GEOMETRY_BUFFER_ID::ANIMATED_SPRITE
		}
	);



	// health
	float enemy_health = max_health;
	Health& health = registry.healths.emplace(self);
	health.maxHealth = enemy_health;
	health.currentHealth = enemy_health;
	

	// Enemy hitbox
	Hitbox enemy_hitbox;
	enemy_hitbox.layer = (int)collision_layer;
	enemy_hitbox.mask = collision_mask;
	enemy_hitbox.hitbox_scale = getHitboxSize();
	registry.hitboxes.emplace(self, enemy_hitbox);

	CollisionMesh colMesh = col_mesh;
	registry.collisionMeshes.insert(self, colMesh);


	// position and motion
	// transform
	Transformation t = registry.transforms.emplace(self);
	t.angle = 0.f;
	t.position = vec2(0, 0);
	t.scale = vec2(2, 2);

	// motion
	auto& motion = registry.motions.emplace(self);
	motion.velocity = { 0, 0 };  // need function to let enemy search for player




	// spells and spell slots
	addSpellsAndSpellSlotsAndParticleEmitters(self);


	// Create animation manager
	AnimationManager& animation_manager = registry.animation_managers.emplace(self);

	// Create Idle and Run animations
	Animation idle_animation = Animation(TEXTURE_ASSET_ID::BOSS_1_IDLE, 4, true); // Asset id, num_frames, is_looping
	idle_animation.h_frames = 4;
	idle_animation.v_frames = 1;
	idle_animation.frame_rate = ANIMATION_FRAME_RATE / 2;
	Animation run_animation = Animation(TEXTURE_ASSET_ID::BOSS_1_RUN, 4, true);
	run_animation.h_frames = 4;
	run_animation.v_frames = 1;
	run_animation.frame_rate = ANIMATION_FRAME_RATE / 2;

	// Insert animations into animation_manager
	animation_manager.animations.insert(std::pair(TEXTURE_ASSET_ID::BOSS_1_IDLE, idle_animation));
	animation_manager.animations.insert(std::pair(TEXTURE_ASSET_ID::BOSS_1_RUN, run_animation));

	// Play idle animation
	animation_manager.transition_to(animation_manager.animations[TEXTURE_ASSET_ID::BOSS_1_IDLE]);

	// lootable: createNextLevelEntry(renderer, player_spawn_pos + vec2(0, -12) * (float)TILE_SIZE, GAME_SCREEN_ID::LEVEL_1);

	Interactable next_level_door;
	next_level_door.interactable_id = INTERACTABLE_ID::NEXT_LEVEL_ENTRY;
	next_level_door.game_screen_id = (int) GAME_SCREEN_ID::IN_BETWEEN;

	Lootable& lootable = registry.lootables.emplace(self);
	lootable.drop = next_level_door;
}



enum BOSS_1_SPELLS { MEDIUM_RED_ORB, RED_ORB, BOSS_ORB, DASH };

void Boss_1::addSpellsAndSpellSlotsAndParticleEmitters(Entity& self) {
	// Create spell slots
	SpellSlotContainer& spellSlotContainer = registry.spellSlotContainers.emplace(self);

	// MEDIUM_RED_ORB
	SpellSlot damageSpellSlot = SpellSlot();
	damageSpellSlot.spell_type = SPELL_TYPE::PROJECTILE;
	damageSpellSlot.spell_id = (int)PROJECTILE_SPELL_ID::MEDIUM_SPEED_RED_ORB;
	damageSpellSlot.internalCooldown = 50;
	spellSlotContainer.spellSlots.push_back(damageSpellSlot);

	// RED_ORB
	SpellSlot damageSpellSlot2 = SpellSlot();
	damageSpellSlot2.spell_type = SPELL_TYPE::PROJECTILE;
	damageSpellSlot2.spell_id = (int)PROJECTILE_SPELL_ID::RED_ORB;
	damageSpellSlot2.internalCooldown = 50;
	spellSlotContainer.spellSlots.push_back(damageSpellSlot2);

	// BOSS_ORB
	SpellSlot damageSpellSlot3 = SpellSlot();
	damageSpellSlot3.spell_type = SPELL_TYPE::PROJECTILE;
	damageSpellSlot3.spell_id = (int)PROJECTILE_SPELL_ID::BOSS_ORB;
	damageSpellSlot3.internalCooldown = 50;
	spellSlotContainer.spellSlots.push_back(damageSpellSlot3);
	
	// DASH
	SpellSlot movementSpellSlot = SpellSlot();
	movementSpellSlot.spell_type = SPELL_TYPE::MOVEMENT;
	movementSpellSlot.spell_id = (int)MOVEMENT_SPELL_ID::DASH;
	movementSpellSlot.internalCooldown = 0;
	spellSlotContainer.spellSlots.push_back(movementSpellSlot);


	ParticleEmitter dash_particle_emitter = ParticleEmitter();
	dash_particle_emitter.setNumParticles(8);
	dash_particle_emitter.setParentEntity(self);
	dash_particle_emitter.setInitialColor(vec4(0.75, 0.75, 1, 0.25));
	dash_particle_emitter.setTextureAssetId(TEXTURE_ASSET_ID::PLAYER_DASH_PARTICLES);

	ParticleEmitterContainer& particle_emitter_container = registry.particle_emitter_containers.emplace(self);
	particle_emitter_container.particle_emitter_map.insert(std::pair<PARTICLE_EMITTER_ID, ParticleEmitter>(PARTICLE_EMITTER_ID::DASH_TRAIL, dash_particle_emitter));
}

void Boss_1::resetAndGoToState(BOSS_STATE target_state, Entity& self) {
	time_since_last_state = 0;
	time_since_last_substate = 0;
	time_since_last_marker = 0;
	time_since_last_attack = 0;
	Motion& m = registry.motions.get(self);
	m.is_dashing = false;
	m.velocity = vec2(0, 0);
	prev_state = state;
	state = target_state;
}

void Boss_1::goToRandomAttackState(BOSS_STATE currentState, Entity& self) {
	// get random state here
	BOSS_STATE next_state = currentState;
	while (next_state == currentState) {
		int rand_state = uniform_dist(rng) * attack_states.size();
		next_state = attack_states[rand_state];
	}
	resetAndGoToState(next_state, self);
}

void printState(BOSS_STATE state) {
	std::cout << " CURRENTLY IN STATE: " << (int)state << std::endl;
}

void handle_dashing_movement(Motion& playerMotion, float stepSeconds) {
	if (glm::length(playerMotion.velocity) < PLAYER_ACCELERATION * stepSeconds) {
		playerMotion.velocity = vec2(0, 0);
	}
	else {
		playerMotion.velocity -= glm::normalize(playerMotion.velocity) * PLAYER_ACCELERATION * stepSeconds;
	}
}

void Boss_1::step(float elapsed_ms, Entity& self) {
	time_since_last_state += elapsed_ms;
	time_since_last_substate += elapsed_ms;
	time_since_last_marker += elapsed_ms;
	time_since_last_attack += elapsed_ms;

	Motion& motion = registry.motions.get(self);
	float stepSeconds = elapsed_ms / 1000.0f;

	if (motion.is_dashing) {
		handle_dashing_movement(motion, stepSeconds);
	}

	switch (state) {
	case BOSS_STATE::INACTIVE:
		if (time_since_last_state >= 1000) {
			printState(state);
			time_since_last_state = 0;
			time_since_last_attack = 0;
			goToRandomAttackState(prev_state, self);
		}
		break;
	case BOSS_STATE::DASH_TOWARD_PLAYER:
		if (time_since_last_attack >= 400) {
			printState(state);
			time_since_last_attack = 0;
			Motion& m = registry.motions.get(self);
			m.is_dashing = false;
			m.velocity = vec2(0, 0);
			dashTowardPlayer(self);
		}

		if (time_since_last_state >= 1600) {
			printState(state);
			resetAndGoToState(BOSS_STATE::CIRCLE_EXPLOSION, self);
		}
		break;
	case BOSS_STATE::SUMMON_ENEMIES:
		if (time_since_last_attack >= 100) {
			time_since_last_attack = 0;
			// get random position around boss
			float max_dist_from_boss = 300;
			float rand_x_offset = (uniform_dist(rng) * max_dist_from_boss) - max_dist_from_boss / 2;
			float rand_y_offset = (uniform_dist(rng) * max_dist_from_boss) - max_dist_from_boss / 2;
			vec2 boss_pos = registry.transforms.get(self).position;
			createBossMinionSpawnIndicator(renderer, vec2(boss_pos.x + rand_x_offset, boss_pos.y + rand_y_offset));
		}

		if (time_since_last_state >= 500) {	
			resetAndGoToState(BOSS_STATE::INACTIVE, self);
		}
		break;
	case BOSS_STATE::CIRCLE_SPIRAL_ATTACK:
		if (time_since_last_attack >= 350) {
			printState(state);
			time_since_last_attack = 0;
			radian_rotation += M_PI / 24.f;
			circleAttack(self);
		}

		if (time_since_last_state >= 6000) {
			printState(state);
			resetAndGoToState(BOSS_STATE::INACTIVE, self);
		}
		break;

	case BOSS_STATE::FIREBALL_STREAM:
		if (time_since_last_attack >= 200) {
			printState(state);
			time_since_last_attack = 0;
			fireballStream(self);
		}

		if (time_since_last_state >= 3050 ) {
			printState(state);
			resetAndGoToState(BOSS_STATE::INACTIVE, self);
		}
		break;
	case BOSS_STATE::CIRCLE_EXPLOSION:
		if (time_since_last_attack >= 150) {
			printState(state);
			time_since_last_attack = 0;
			radian_rotation += M_PI / 24.f;
			circleExplosion(self);
		}

		if (time_since_last_state >= 500) {
			resetAndGoToState(BOSS_STATE::FIREBALL_STREAM, self);
		}
		break;
	}
}


void Boss_1::dashTowardPlayer(Entity& self) {
	Transformation& transform = registry.transforms.get(self);
	Transformation& player_transform = registry.transforms.get(registry.players.entities[0]);
	SpellSlotContainer& spell_slot_container = registry.spellSlotContainers.get(self);

	vec2& self_pos = transform.position;
	vec2& player_pos = player_transform.position;

	SpellSlot& spell_slot = spell_slot_container.spellSlots[(int)BOSS_1_SPELLS::DASH]; // dash
	vec2 angle = player_pos - self_pos;
	SpellCastManager::castSpell(renderer, spell_slot, transform.position, angle, self);
	spell_slot.remainingCooldown = 0;
}


void Boss_1::circleAttack(Entity& self) {
	Transformation& transform = registry.transforms.get(self);
	SpellSlotContainer& spell_slot_container = registry.spellSlotContainers.get(self);
	SpellSlot& spell_slot = spell_slot_container.spellSlots[(int)BOSS_1_SPELLS::MEDIUM_RED_ORB];
	int radius = 100;

	int bullet_count = 10;
	for (int i = 0; i < bullet_count; i++) {
		double angleCW = 2 * M_PI * i / bullet_count + radian_rotation;
		vec2 currentAngle = { radius * sin(angleCW), radius * cos(angleCW) };
		SpellCastManager::castBossSpell(renderer, spell_slot, transform.position, currentAngle, self);
	}
}


void Boss_1::fireballStream(Entity& self) {
	Transformation& transform = registry.transforms.get(self);
	SpellSlotContainer& spell_slot_container = registry.spellSlotContainers.get(self);
	SpellSlot& spell_slot = spell_slot_container.spellSlots[(int)BOSS_1_SPELLS::BOSS_ORB];
	SpellSlot& spell_slot_2 = spell_slot_container.spellSlots[(int)BOSS_1_SPELLS::MEDIUM_RED_ORB];

	Transformation& player_transform = registry.transforms.get(registry.players.entities[0]);
	vec2 angle = player_transform.position - transform.position;

	int bullet_count = 3;
	int angle_randomness = 120;
	for (int i = 0; i < bullet_count; i++) {
		float angle_x_offset = (uniform_dist(rng) * angle_randomness) - angle_randomness / 2;
		float angle_y_offset = (uniform_dist(rng) * angle_randomness) - angle_randomness / 2;

		vec2 current_angle = vec2(angle.x + angle_x_offset, angle.y + angle_y_offset);
		SpellCastManager::castBossSpell(renderer, spell_slot, transform.position, current_angle, self);
	}
}


void Boss_1::circleExplosion(Entity& self) {
	Transformation& transform = registry.transforms.get(self);
	SpellSlotContainer& spell_slot_container = registry.spellSlotContainers.get(self);
	SpellSlot& spell_slot = spell_slot_container.spellSlots[(int)BOSS_1_SPELLS::RED_ORB];

	int bullet_count = 12;
	for (int i = 0; i < bullet_count; i++) {
		double angleCW = 2 * M_PI * i / bullet_count + radian_rotation;
		vec2 currentAngle = { sin(angleCW), cos(angleCW) };
		SpellCastManager::castBossSpell(renderer, spell_slot, transform.position, currentAngle, self);
	}
}