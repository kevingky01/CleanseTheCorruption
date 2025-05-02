#pragma once

#include <tinyECS/entity.hpp>
#include <common.hpp>
#include <iostream>
#include "render_system.hpp"

enum ENEMY_TYPE { BASIC_RANGED_ENEMY, SHOTGUN_RANGED_ENEMY, TOWER_ENEMY, DUMMY_ENEMY, BASIC_MELEE_ENEMY,BOSS_1, BOSS_2, };
enum BOSS2_SEGMENT_ACTIVE {
    SEGMENT_A,
    SEGMENT_B
};


// Enemy - Components for enemy identification, maybe will add new properties here later - Mark
class Enemy {
private:
public:
	COLLISION_LAYER collision_layer = COLLISION_LAYER::ENEMY;
	int collision_mask = (int)COLLISION_MASK::WALL | (int)COLLISION_MASK::PLAYER;

	RenderSystem* renderer;
	float max_health;
	float max_speed;
	float damage;
	float speed;
	vec2 hitbox_size;

	CollisionMesh col_mesh;

	int detection_range;
	int shooting_range;

	ENEMY_TYPE type; // Type of enemies

	// maximum distance that enemy is able to shoot to player, 
	// inside range enemy will stop moving then start shooting
	int attack_countdown = 0.0;  // when to shoot
	int base_recharge_time; // const for reset time_ms

	int idle_state_time_ms;
	// enemies flee for a set time before exiting state
	int flee_state_time_ms;

	virtual void step(float elapsed_ms, Entity& self) = 0;

	float getMaxHealth();
	vec2 getHitboxSize();
	CollisionMesh getCollisionMesh();
	vec2 random_direction;
};



// Basic Three: Idle, Targeting, Shooting
// Status for enemy
enum ENEMY_STATUS { IDLE, TARGETING, SHOOTING, FLEEING };
enum STATUS_IN_IDLE { M_STOP_R, ROTATE, R_STOP_M, MOVE };

class BasicRangedEnemy : public Enemy {
public:
	BasicRangedEnemy(Entity& entity);
	void step(float elapsed_ms, Entity& self);
private:
	ENEMY_STATUS status; // Will be initialized as Idle
	void updateEnemyAnimation(Entity& enemy_entity);
	void addComponents(Entity& entity);
	void burstAttack(Entity& self);

};

class TowerEnemy : public Enemy {
	public:
		TowerEnemy(Entity& entity);
		void step(float elapsed_ms, Entity& self);
	private:
		ENEMY_STATUS status; // Will be initialized as Idle
		void updateEnemyAnimation(Entity& enemy_entity);
		void addComponents(Entity& entity);
		void burstAttack(Entity& self);
	
	};

class ShotgunRangedEnemy : public Enemy {
public:
	ShotgunRangedEnemy(Entity& entity);
	void step(float elapsed_ms, Entity& self);
private:
	ENEMY_STATUS status; // Will be initialized as Idle
	void updateEnemyAnimation(Entity& enemy_entity);
	void addComponents(Entity& entity);
	void shotgunAttack(Entity& self);
// private:
};


class MeleeEnemy : public Enemy {
public:
	MeleeEnemy(Entity& entity);
	void step(float elapsed_ms, Entity& self);
private:
	ENEMY_STATUS status;
	void updateEnemyAnimation(Entity& enemy_entity);
	void addComponents(Entity& entity);
};

class DummyEnemy : public Enemy {
	public:
	DummyEnemy(Entity& entity);
	void step(float elapsed_ms, Entity& self);
private:
	ENEMY_STATUS status;
	void updateEnemyAnimation(Entity& enemy_entity);
	void addComponents(Entity& entity);
};



enum BOSS_STATE { INACTIVE, DASH_TOWARD_PLAYER, SUMMON_ENEMIES, CIRCLE_SPIRAL_ATTACK, FIREBALL_STREAM, CIRCLE_EXPLOSION,CIRCLE_ATTACK };

class Boss_1 : public Enemy {
public:
	Boss_1(Entity& entity);
	void step(float elapsed_ms, Entity& self);
private:
	BOSS_STATE prev_state = BOSS_STATE::CIRCLE_SPIRAL_ATTACK;
	BOSS_STATE state = BOSS_STATE::INACTIVE;
	std::vector<BOSS_STATE> attack_states = { DASH_TOWARD_PLAYER, SUMMON_ENEMIES, CIRCLE_SPIRAL_ATTACK };
	float radian_rotation;
	float time_since_last_state;
	float time_since_last_substate;
	float time_since_last_marker;
	float time_since_last_attack;

	void buildComponents(Entity& self);
	void addSpellsAndSpellSlotsAndParticleEmitters(Entity& self);

	void resetAndGoToState(BOSS_STATE state, Entity& self);
	void goToRandomAttackState(BOSS_STATE currentState, Entity& self);

	void circleAttack(Entity& self);
	void fireballStream(Entity& self);
	void circleExplosion(Entity& self);
	void dashTowardPlayer(Entity& self);
};




class Boss_2 : public Enemy {
	public:
		Boss_2(Entity& main_boss_entity);
		void step(float elapsed_ms, Entity& self);

	private:

		BOSS2_SEGMENT_ACTIVE active_segment;
		Entity segmentA_entity;
		Entity segmentB_entity;
		float time_since_last_state;
		float time_since_last_attack;
		float time_since_last_switch;

		
		
		BOSS_STATE state = BOSS_STATE::INACTIVE;
		float radian_rotation;

		void buildComponents(Entity& main_boss_entity);
		void circleAttack(Entity& main_boss_entity);

	
		// void spawnSegmentA(Entity& enemy_1);
		void spawnSegmentB(Entity& enemy_2);
		void  dashTowardPlayer(Entity& self);
		void addSpellsAndSpellSlotsAndParticleEmitters(Entity& self);
		void handle_dashing_movement(Motion& playerMotion, float stepSeconds);
	};

	
	
	
