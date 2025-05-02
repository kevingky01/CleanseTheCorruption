#include "spells.hpp"
#include "common.hpp"
#include "tinyECS/registry.hpp"
#include "tinyECS/components.hpp"
#include "tinyECS/entity.hpp"
#include "render_system.hpp"
#include "world_system.hpp"
#include "world_init.hpp"
#include <iostream>
#include <glm/gtx/rotate_vector.hpp>


/////////////////////////////////////////////////
//           Projectile Spells                 //
/////////////////////////////////////////////////

// Default spell functions
void ProjectileSpell::cast(RenderSystem* renderer, vec2 spawn_position, vec2 direction, PROJECTILE_SPELL_ID spell_id, Entity casted_by) {

	Entity entity = createProjectile(renderer, spawn_position, direction, getSpeed(), spell_id, casted_by, getParticleEmitter());
	Projectile& projectile = registry.projectiles.get(entity);
	projectile.damage = getDamage(); 


	// Recast spell num_casts times, maybe we can still use this, just add some spread to the projectiles 
	/*setNumCasts(getNumCasts() - 1);

	if (getNumCasts() <= 0) {
		return;
	}

	this->cast(renderer, spawn_position, direction, spell_id, casted_by);*/
}

void ProjectileSpell::onDeath(RenderSystem* renderer, Entity projectileEntity) {
	Projectile& projectile = registry.projectiles.get(projectileEntity);

	projectile.is_dead = true;

	Motion& motion = registry.motions.get(projectileEntity);
	motion.velocity = vec2(0, 0);

	registry.renderRequests.remove(projectileEntity);
	registry.hitboxes.remove(projectileEntity);

	std::function<void()> timeout = [projectileEntity]() { // pass in motion as a reference 
		registry.remove_all_components_of(projectileEntity);
		};

	ParticleEmitterContainer& particle_container = registry.particle_emitter_containers.get(projectileEntity);
	ParticleEmitter& particle_emitter = particle_container.particle_emitter_map[PARTICLE_EMITTER_ID::PROJECTILE_TRAIL];
	particle_emitter.stop_emitting();

	createTimer(particle_emitter.loop_duration, timeout);
}

void ProjectileSpell::stepProjectile(Entity projectileEntity, float elapsed_ms) {
	// By default, projectiles just move in a straight line
}




// Shotgun Spell shoots numProjectiles projectiles when it is casted, in a random spread around direction
void ShotgunSpell::cast(RenderSystem* renderer, vec2 spawn_position, vec2 direction, PROJECTILE_SPELL_ID spell_id, Entity casted_by) {

	// cast X number of projectiles in a random spread
	for (int i = 0; i < numProjectiles; i++) {

		float randomAngleOffset = uniform_dist(rng) * spreadDegrees - (spreadDegrees / 2.0); // -spreadDegrees/2 to spreadDegrees/2
		float randRad = (randomAngleOffset * M_PI) / 180.0f;
		vec2 newDirection = glm::rotate(direction, randRad);
		//vec2 newDirection = direction;
		Entity entity = createProjectile(renderer, spawn_position, newDirection, getSpeed(), spell_id, casted_by, getParticleEmitter());
		Projectile& projectile = registry.projectiles.get(entity);
		projectile.damage = getDamage();
	} 
}





// Decelerating spell slows the projectile every frame, until it eventually comes to a halt
const float ZERO_VELOCITY_THRESHOLD = 0.1f;
void DeceleratingSpell::stepProjectile(Entity projectileEntity, float elapsed_ms) {
	if (!registry.motions.has(projectileEntity)) {
		return;
	}

	Motion& projectileMotion = registry.motions.get(projectileEntity);

	// If velocity is near zero, set it to zero and return
	if (glm::length(projectileMotion.velocity) < ZERO_VELOCITY_THRESHOLD) {
		projectileMotion.velocity = vec2(0, 0);
		return;
	}

	float stepSeconds = elapsed_ms / 1000.0f;

	// Otherwise, decrease the velocity
	projectileMotion.velocity -= glm::normalize(projectileMotion.velocity) * decelStrength * stepSeconds;
}







// Accelerating spell speeds up the projectile every frame, with no max speed
void AcceleratingSpell::stepProjectile(Entity projectileEntity, float elapsed_ms) {
	if (!registry.motions.has(projectileEntity)) {
		return;
	}

	Motion& projectileMotion = registry.motions.get(projectileEntity);

	float stepSeconds = elapsed_ms / 1000.0f;

	// Otherwise, decrease the velocity
	projectileMotion.velocity += glm::normalize(projectileMotion.velocity) * accelStrength * stepSeconds;
}







// Shrapnel spell release numShrapnel 
const int SHRAPNEL_RANDOM_SPREAD = 10;
void ShrapnelSpell::onDeath(RenderSystem* renderer, Entity projectileEntity) {
	if (!registry.motions.has(projectileEntity) || !registry.transforms.has(projectileEntity)) {
		return;
	}

	Motion& motion = registry.motions.get(projectileEntity);
	Transformation& transform = registry.transforms.get(projectileEntity);
	Projectile& projectile = registry.projectiles.get(projectileEntity);

	vec2 shrapnelDirection = motion.velocity;

	// Choose a random direction if projectile's velocity is 0
	if (glm::length(shrapnelDirection) == 0) {
		float rand_x = uniform_dist(rng) * 2 - 1;
		float rand_y = uniform_dist(rng) * 2 - 1;
		shrapnelDirection = vec2(rand_x, rand_y);
	}

	for (int i = 0; i < this->getNumShrapnel(); i++) {
		float randomAngleOffset = uniform_dist(rng) * SHRAPNEL_RANDOM_SPREAD - (SHRAPNEL_RANDOM_SPREAD / 2.0); // -spreadDegrees/2 to spreadDegrees/2
		float randRad = (randomAngleOffset * M_PI) / 180.0f;
		vec2 slightly_random_direction = shrapnelDirection;
		slightly_random_direction = glm::rotate(slightly_random_direction, randRad);

		PROJECTILE_SPELL_ID shrapnels = PROJECTILE_SPELL_ID::THORN;
		ProjectileSpell* spell = projectile_spells[(int)shrapnels];
		spell->cast(renderer, transform.position, slightly_random_direction, shrapnels, projectile.owner);

		shrapnelDirection = glm::rotate(shrapnelDirection, (float)(2 * M_PI / numShrapnel));
	}

	ProjectileSpell::onDeath(renderer, projectileEntity);
}








void SeekingSpell::cast(RenderSystem* renderer, vec2 spawn_position, vec2 direction, PROJECTILE_SPELL_ID spell_id, Entity casted_by) {

	Entity entity = createProjectile(renderer, spawn_position, direction, getSpeed(), spell_id, casted_by, getParticleEmitter());

	Seeking& seeking = registry.seekings.emplace(entity);

	//Entity nearest_enemy = getNearestEnemyToMouse();
	Entity nearest_enemy = getNearestEnemyEntity(casted_by);
	seeking.target = nearest_enemy;

	Transformation& projectile_transform = registry.transforms.get(entity);
	projectile_transform.angle = atan2(direction.y, direction.x) * 180 / M_PI + 90;

	Projectile& projectile = registry.projectiles.get(entity);
	projectile.damage = getDamage(); 
}

void SeekingSpell::stepProjectile(Entity projectileEntity, float elapsed_ms) {
	Projectile projectile = registry.projectiles.get(projectileEntity);

	if (!registry.seekings.has(projectileEntity)) {
		return;
	}

	Seeking& seeking = registry.seekings.get(projectileEntity);

	if (seeking.target == -1) {
		Entity player_entity = registry.players.entities[0];

		// If projectile is casted by player, get nearest enemy to projectile
		if (projectile.owner == player_entity) {
			//seeking.target = getNearestEnemyToMouse();
			seeking.target = getNearestEnemyEntity(projectileEntity);
		}
		else { // Otherwise, seek the player
			seeking.target = player_entity;
		}
	}

	// If enemy to follow no longer exists, reset
	if (!registry.transforms.has(seeking.target)) {
		seeking.target = -1;
	}

	// Only make changes to velocity if can find enemy entity
	if (seeking.target != -1) {
		Motion& projectile_motion = registry.motions.get(projectileEntity);
		Transformation& projectile_transform = registry.transforms.get(projectileEntity);
		Transformation& entity_transform = registry.transforms.get(seeking.target);

		float step_seconds = elapsed_ms / 1000.0f;

		// Turn the projectile towards entity
		vec2 direction = glm::normalize(entity_transform.position - projectile_transform.position);
		projectile_motion.velocity += direction * seek_strength * step_seconds;

		// Ensure speed is still capped
		if (glm::length(projectile_motion.velocity) > getSpeed()) {
			projectile_motion.velocity = glm::normalize(projectile_motion.velocity) * getSpeed();
		}

		projectile_transform.angle = atan2(direction.y, direction.x) * 180 / M_PI + 90;
	}
}

Entity SeekingSpell::getNearestEnemyEntity(Entity projectileEntity) {
	Transformation projectile_transform = registry.transforms.get(projectileEntity);
	float nearest_dist = 9999999; 
	Entity nearest_enemy = -1;
	for (Entity enemy_entity : registry.enemies.entities) {
		if (!registry.transforms.has(enemy_entity)) {
			continue;
		}

		Transformation enemy_transform = registry.transforms.get(enemy_entity);
		float dist = glm::distance(enemy_transform.position, projectile_transform.position);
		if (dist < nearest_dist) {
			nearest_dist = dist;
			nearest_enemy = enemy_entity;
		}
	}

	return nearest_enemy;
}

Entity SeekingSpell::getNearestEnemyToMouse() {
	vec2 mouse_pos = vec2(mouse_pos_x, mouse_pos_y);

	Entity camera_entity = registry.cameras.entities[0];
	Transformation camera_transform = registry.transforms.get(camera_entity);

	vec2 mouse_world_pos = mouse_pos + camera_transform.position;

	float nearest_dist = 9999999;
	Entity nearest_enemy = -1;
	for (Entity enemy_entity : registry.enemies.entities) {
		if (!registry.transforms.has(enemy_entity)) {
			continue;
		}

		Transformation enemy_transform = registry.transforms.get(enemy_entity);
		float dist = glm::distance(enemy_transform.position, mouse_world_pos);

		if (dist < nearest_dist) {
			nearest_dist = dist;
			nearest_enemy = enemy_entity;
		}
	}

	return nearest_enemy;
}




// Shoot a PlayerSeekingprojectile at the player
void BoomerangSpell::onDeath(RenderSystem* renderer, Entity projectileEntity) {
	if (!registry.motions.has(projectileEntity) || !registry.transforms.has(projectileEntity)) {
		return;
	}

	Motion& motion = registry.motions.get(projectileEntity);
	Transformation& transform = registry.transforms.get(projectileEntity);
	Projectile& projectile = registry.projectiles.get(projectileEntity);

	Entity player_entity = registry.players.entities[0];
	Transformation& player_transform = registry.transforms.get(player_entity);

	vec2 direction = player_transform.position - transform.position;

	PROJECTILE_SPELL_ID shrapnels = PROJECTILE_SPELL_ID::BOOMERANG_RETURN;
	ProjectileSpell* spell = projectile_spells[(int)shrapnels];
	spell->cast(renderer, transform.position, direction, shrapnels, projectile.owner);

	ProjectileSpell::onDeath(renderer, projectileEntity); 
}

void BoomerangSpell::stepProjectile(Entity projectileEntity, float elapsed_ms) {
	DeceleratingSpell::stepProjectile(projectileEntity, elapsed_ms);

	if (!registry.transforms.has(projectileEntity)) {
		return;
	}
	Transformation& projectile_transform = registry.transforms.get(projectileEntity);
	projectile_transform.angle += rotationSpeed * elapsed_ms / 1000;
}




void BoomerangReturnSpell::cast(RenderSystem* renderer, vec2 spawn_position, vec2 direction, PROJECTILE_SPELL_ID spell_id, Entity casted_by) {

	Entity entity = createProjectile(renderer, spawn_position, direction, getSpeed(), spell_id, casted_by, getParticleEmitter());

	Seeking& seeking = registry.seekings.emplace(entity);

	Entity player_entity = registry.players.entities[0];
	seeking.target = player_entity;

	Motion& projectile_motion = registry.motions.get(entity);
	projectile_motion.velocity = vec2(0, 0);

	Projectile& projectile = registry.projectiles.get(entity);
	projectile.damage = getDamage(); 
}

const int SEEKING_PROJECTILE_NEAR_THRESHOLD = 15 * PIXEL_SCALE_FACTOR;
void BoomerangReturnSpell::stepProjectile(Entity projectileEntity, float elapsed_ms) {
	Projectile projectile = registry.projectiles.get(projectileEntity);

	if (!registry.seekings.has(projectileEntity)) {
		return;
	}

	Seeking& seeking = registry.seekings.get(projectileEntity);

	Motion& projectile_motion = registry.motions.get(projectileEntity);
	Transformation& projectile_transform = registry.transforms.get(projectileEntity);
	Transformation& entity_transform = registry.transforms.get(seeking.target);

	float distance_to_target = glm::distance(projectile_transform.position, entity_transform.position);

	if (distance_to_target < SEEKING_PROJECTILE_NEAR_THRESHOLD) {
		Projectile& projectile = registry.projectiles.get(projectileEntity);
		projectile.lifetime = 0; // Munn: Kinda scuffed way to "kill" the projectile
		return;
	}
	
	float step_seconds = elapsed_ms / 1000.0f;

	// Turn the projectile towards entity
	vec2 direction = glm::normalize(entity_transform.position - projectile_transform.position);
	projectile_motion.velocity += direction * getSeekStrength() * step_seconds;

	// Ensure speed is still capped
	if (glm::length(projectile_motion.velocity) > getSpeed()) {
		projectile_motion.velocity = glm::normalize(projectile_motion.velocity) * getSpeed();
	}

	projectile_transform.angle += rotationSpeed * step_seconds;
}


//void RicochetSpell::onDeath(RenderSystem* renderer, Entity projectileEntity) {
//	if (!registry.motions.has(projectileEntity) || !registry.transforms.has(projectileEntity)) {
//		return;
//	}
//
//	Motion& motion = registry.motions.get(projectileEntity);
//	Transformation& transform = registry.transforms.get(projectileEntity);
//	Projectile& projectile = registry.projectiles.get(projectileEntity);
//
//	float numBounce = this->getNumBounces();
//
//	// -30 to 30 degree random bounce
//	float randomAngleOffset = uniform_dist(rng) * 60.0 - 30.0;
//	float randRad = (randomAngleOffset * M_PI) / 180.0f;
//	vec2 newDirection = glm::rotate(glm::normalize(motion.velocity), randRad);
//
//	if (numBounce > 0) {
//		PROJECTILE_SPELL_ID ricochet = PROJECTILE_SPELL_ID::LIGHTNING;
//		RicochetSpell* ricochetSpell = dynamic_cast<RicochetSpell*>(projectile_spells[(int)ricochet]->clone());
//
//		// Cast the new ricochet with reduced bounce count
//		ricochetSpell->setNumBounces(numBounce - 1);
//		ricochetSpell->cast(renderer, transform.position, newDirection, ricochet, projectile.owner);
//		
//		delete ricochetSpell;
//	} else {
//		PROJECTILE_SPELL_ID lightning = PROJECTILE_SPELL_ID::LIGHTNING;
//		ProjectileSpell* spell = projectile_spells[(int)lightning];
//		spell->cast(renderer, transform.position, newDirection, lightning, projectile.owner);
//	}
//
//	ProjectileSpell::onDeath(renderer, projectileEntity);
//}


// Guo: stub for acid effect, will work on this later
void AreaOnDeathSpell::onDeath(RenderSystem* renderer, Entity projectileEntity) {
	if (!registry.motions.has(projectileEntity) || !registry.transforms.has(projectileEntity)) {
		return;
	}

	Motion& motion = registry.motions.get(projectileEntity);
	Transformation& transform = registry.transforms.get(projectileEntity);
	Projectile& projectile = registry.projectiles.get(projectileEntity);

	float duration = this->getAreaLifetime();

	// Create an AOE that lingers for duration
	PROJECTILE_SPELL_ID aoeEffect = PROJECTILE_SPELL_ID::ACID_EFFECT;
	ProjectileSpell* spell = projectile_spells[(int)aoeEffect];
	spell->cast(renderer, transform.position, vec2(0), aoeEffect, projectile.owner);

	ProjectileSpell::onDeath(renderer, projectileEntity);
}





void ArcSpell::cast(RenderSystem* renderer, vec2 spawn_position, vec2 direction, PROJECTILE_SPELL_ID spell_id, Entity casted_by) {

	PROJECTILE_SPELL_ID arc_left = PROJECTILE_SPELL_ID::CUTTER_LEFT;
	ProjectileSpell* spell_left = projectile_spells[(int)arc_left];
	spell_left->cast(renderer, spawn_position, direction, arc_left, casted_by);

	PROJECTILE_SPELL_ID arc_right = PROJECTILE_SPELL_ID::CUTTER_RIGHT;
	ProjectileSpell* spell_right = projectile_spells[(int)arc_right];
	spell_right->cast(renderer, spawn_position, direction, arc_right, casted_by);
}

// ArcSpell double-cast in front
void ArcSpell::onDeath(RenderSystem* renderer, Entity projectileEntity) {
	/*if (!registry.motions.has(projectileEntity) || !registry.transforms.has(projectileEntity)) {
		return;
	}

	Motion& motion = registry.motions.get(projectileEntity);
	Transformation& transform = registry.transforms.get(projectileEntity);
	Projectile& projectile = registry.projectiles.get(projectileEntity);

	vec2 direction = glm::length(motion.velocity) > 0 ? glm::normalize(motion.velocity) : vec2(1, 0);

	PROJECTILE_SPELL_ID arc_left = PROJECTILE_SPELL_ID::CUTTER_LEFT;
	ProjectileSpell* spell_left = projectile_spells[(int)arc_left];
	spell_left->cast(renderer, transform.position, direction, arc_left, projectile.owner);

	PROJECTILE_SPELL_ID arc_right = PROJECTILE_SPELL_ID::CUTTER_RIGHT;
	ProjectileSpell* spell_right = projectile_spells[(int)arc_right];
	spell_right->cast(renderer, transform.position, direction, arc_right, projectile.owner);*/

	ProjectileSpell::onDeath(renderer, projectileEntity);
}


void ArcSpell::stepProjectile(Entity projectileEntity, float elapsed_ms) {
	if (!registry.motions.has(projectileEntity) || !registry.transforms.has(projectileEntity)) {
		return;
	}
	Transformation& projectile_transform = registry.transforms.get(projectileEntity);
	projectile_transform.angle += rotationSpeed * elapsed_ms / 1000;
}


void ArcSpellLeft::cast(RenderSystem* renderer, vec2 spawn_position, vec2 direction, PROJECTILE_SPELL_ID spell_id, Entity casted_by) {
	float angleOffset = 0.69f;
	vec2 leftDirection = glm::rotate(direction, -angleOffset);

	Entity left_arc_entity = createProjectile(renderer, spawn_position, leftDirection, getSpeed(), spell_id, casted_by, getParticleEmitter());
	Projectile& left_arc = registry.projectiles.get(left_arc_entity);
	left_arc.damage = getDamage();

	Transformation& transform1 = registry.transforms.get(left_arc_entity);
	transform1.angle = atan2(leftDirection.y, leftDirection.x) * 180.0f / M_PI + 90.0f;

	// Curve arc will be handled in stepProjectile
	Motion& motion1 = registry.motions.get(left_arc_entity);
	motion1.velocity = leftDirection * getSpeed();
}

void ArcSpellRight::cast(RenderSystem* renderer, vec2 spawn_position, vec2 direction, PROJECTILE_SPELL_ID spell_id, Entity casted_by) {
	float angleOffset = 0.69f;
	vec2 rightDirection = glm::rotate(direction, angleOffset);

	Entity right_arc_entity = createProjectile(renderer, spawn_position, rightDirection, getSpeed(), spell_id, casted_by, getParticleEmitter());
	Projectile& right_arc = registry.projectiles.get(right_arc_entity);
	right_arc.damage = getDamage();

	Transformation& transform2 = registry.transforms.get(right_arc_entity);
	transform2.angle = atan2(rightDirection.y, rightDirection.x) * 180.0f / M_PI + 90.0f;

	Motion& motion2 = registry.motions.get(right_arc_entity);
	motion2.velocity = rightDirection * getSpeed();
}

void ArcSpellLeft::stepProjectile(Entity projectileEntity, float elapsed_ms) {
	if (!registry.motions.has(projectileEntity) || !registry.transforms.has(projectileEntity)) {
		return;
	}

	Motion& motion = registry.motions.get(projectileEntity);
	Transformation& transform = registry.transforms.get(projectileEntity);

	vec2 currentDirection = glm::normalize(motion.velocity);


	// Perpendicular vector for curving
	vec2 perpendicular = vec2(-currentDirection.y, currentDirection.x);
	perpendicular = glm::normalize(perpendicular);

	// Small velocity going back toward the center
	vec2 towardCenter = -currentDirection * 0.1f;

	// The longer the projectile exists, the more it curves
	float projectileLifetime = 0.0f;
	if (registry.projectiles.has(projectileEntity)) {
		Projectile& projectile = registry.projectiles.get(projectileEntity);
		projectileLifetime = getLifetime() - projectile.lifetime;
	}

	float curveStrength = 6.5f;
	float step_seconds = elapsed_ms / 1000.0f;

	// Apply curved direction to velocity
	vec2 curveVelocity = (perpendicular * curveStrength + towardCenter) * step_seconds * getSpeed();
	motion.velocity += curveVelocity;

	// Ensure speed is still capped
	if (glm::length(motion.velocity) > getSpeed()) {
		motion.velocity = glm::normalize(motion.velocity) * getSpeed();
	}

	transform.angle += rotationSpeed * step_seconds;
}

void ArcSpellRight::stepProjectile(Entity projectileEntity, float elapsed_ms) {
	if (!registry.motions.has(projectileEntity) || !registry.transforms.has(projectileEntity)) {
		return;
	}

	Motion& motion = registry.motions.get(projectileEntity);
	Transformation& transform = registry.transforms.get(projectileEntity);

	vec2 currentDirection = glm::normalize(motion.velocity);



	// Perpendicular vector for curving
	vec2 perpendicular = vec2(currentDirection.y, -currentDirection.x);
	perpendicular = glm::normalize(perpendicular);

	// Small velocity going back toward the center
	vec2 towardCenter = -currentDirection * 0.1f;

	// The longer the projectile exists, the more it curves
	float projectileLifetime = 0.0f;
	if (registry.projectiles.has(projectileEntity)) {
		Projectile& projectile = registry.projectiles.get(projectileEntity);
		projectileLifetime = getLifetime() - projectile.lifetime;
	}

	float curveStrength = 6.5f;
	float step_seconds = elapsed_ms / 1000.0f;

	// Apply curved direction to velocity
	vec2 curveVelocity = (perpendicular * curveStrength + towardCenter) * step_seconds * getSpeed();
	motion.velocity += curveVelocity;

	// Ensure speed is still capped
	if (glm::length(motion.velocity) > getSpeed()) {
		motion.velocity = glm::normalize(motion.velocity) * getSpeed();
	}

	transform.angle += rotationSpeed * step_seconds;
}







/////////////////////////////////////////////////
//                 Boss Spells                 //
/////////////////////////////////////////////////













/////////////////////////////////////////////////
//             Movement Spells                 //
/////////////////////////////////////////////////


void MovementSpell::cast(RenderSystem* renderer, Entity entity, vec2 direction) {
	// Do something with the entity
}



// Dash increases velocity, 
void DashSpell::cast(RenderSystem* renderer, Entity entity, vec2 direction) {
	if (!registry.motions.has(entity) || !registry.transforms.has(entity)) {
		return;
	} 
	
	Motion& motion = registry.motions.get(entity); 
	Transformation& transform = registry.transforms.get(entity);

	int initial_pos = transform.position.x;
	//std::cout << "Started dash at: " << transform.position.x << std::endl;

	float in_combat_multiplier = 1.f;
	//float in_combat_multiplier = motion.is_in_combat ? 1.f : 8.f;
	float speed = (this->getDistance() / duration) + (PLAYER_ACCELERATION * duration * 0.5f * in_combat_multiplier) - PLAYER_MOVE_SPEED; // Munn: we love kinematics 

	float num_cast_multiplier = max(1.0f, ((float)this->getNumCasts() / 1.5f));
	motion.velocity = glm::normalize(direction) * speed * num_cast_multiplier * in_combat_multiplier;

	motion.is_dashing = true;

	ParticleEmitterContainer& particle_container = registry.particle_emitter_containers.get(entity);
	ParticleEmitter& particle_emitter = particle_container.particle_emitter_map[PARTICLE_EMITTER_ID::DASH_TRAIL];

	particle_emitter.setLoopDuration(duration);
	particle_emitter.start_emitting();

	// Munn: Using lambda functions for timer timeout callback 
	std::function<void()> timeout = [entity]() { // pass in motion as a reference 
		// DO NOT CALL FUNCTION IF ENTITY HAS DIED BEFORE TIMER TIMEOUT OCCURS
		if (!registry.motions.has(entity) || !registry.particle_emitter_containers.has(entity)) {
			return;
		}

		Motion& motion = registry.motions.get(entity);
		ParticleEmitterContainer& particle_container = registry.particle_emitter_containers.get(entity);
		ParticleEmitter& particle_emitter = particle_container.particle_emitter_map[PARTICLE_EMITTER_ID::DASH_TRAIL];

		motion.is_dashing = false;
		particle_emitter.stop_emitting();
		};

	Entity timerEntity = createTimer(duration, timeout);
}

void BlinkSpell::cast(RenderSystem* renderer, Entity entity, vec2 direction) {
	if (!registry.motions.has(entity) || !registry.transforms.has(entity)) {
		return;
	}

	Motion& motion = registry.motions.get(entity);

	motion.is_dashing = true;

	float num_cast_multiplier = max(1.0f, ((float)this->getNumCasts() / 1.5f));
	vec2 blink_distance = glm::normalize(direction) * this->getDistance() * num_cast_multiplier;

	// Munn: Using lambda functions for timer timeout callback 
	std::function<void()> timeout = [entity, blink_distance]() { // pass in motion as a reference 
		
		// DO NOT CALL FUNCTION IF ENTITY HAS DIED BEFORE TIMER TIMEOUT OCCURS
		if (!registry.motions.has(entity) || !registry.transforms.has(entity)) {
			return;
		}

		Motion& motion = registry.motions.get(entity);
		Transformation& transform = registry.transforms.get(entity);

		transform.position += blink_distance;
		motion.is_dashing = false;
		};

	Entity timerEntity = createTimer(castTime, timeout);
}