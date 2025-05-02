#pragma once

#include "array"

#include "tinyECS/components.hpp"
#include "render_system.hpp"
#include <iostream>

class Spell {
private:
	float cooldown;
	TEXTURE_ASSET_ID asset_id;

	// Munn: New "cast amount"
	int num_casts = 1;
	float internal_cast_cooldown = 0;
public:
	Spell() {}

	Spell(float cooldown, TEXTURE_ASSET_ID asset_id) {
		this->cooldown = cooldown;
		this->asset_id = asset_id;
	} 
	virtual void cast() {

	}


	virtual Spell* clone() {
		Spell* clone = new Spell(this->cooldown, this->asset_id);

		return clone;
	}


	float getCooldown() {
		return cooldown;
	}
	void setCooldown(float cooldown) {
		this->cooldown = cooldown;
	}
	TEXTURE_ASSET_ID getAssetID() {
		return asset_id;
	}
	void setAssetID(TEXTURE_ASSET_ID asset_id) {
		this->asset_id = asset_id;
	}

	int getNumCasts() {
		return this->num_casts;
	}
	void setNumCasts(int num_casts) {
		this->num_casts = num_casts;
	}

	float getInternalCastCooldown() {
		return this->internal_cast_cooldown;
	}
	void setInternalCastCooldown(float internal_cast_cooldown) {
		this->internal_cast_cooldown = internal_cast_cooldown;
	}
};

class ProjectileSpell : public Spell {
private: 
	int damage;
	float speed;
	float lifetime;
	ParticleEmitter particle_emitter;
public:
	ProjectileSpell(float cooldown, TEXTURE_ASSET_ID asset_id, int damage, float speed, float lifetime, ParticleEmitter particle_emitter) : Spell(cooldown, asset_id) {
 
		this->damage = damage;
		this->speed = speed;
		this->lifetime = lifetime;
		this->particle_emitter = particle_emitter;

		setInternalCastCooldown(0.05);
	} 

	virtual void cast(RenderSystem* renderer, vec2 spawn_position, vec2 direction, PROJECTILE_SPELL_ID spell_id, Entity entity_type);

	virtual void onDeath(RenderSystem* renderer, Entity projectileEntity);

	virtual void stepProjectile(Entity projectileEntity, float elapsed_ms);

	virtual ProjectileSpell* clone() {
		ProjectileSpell* clone = new ProjectileSpell(getCooldown(), getAssetID(), this->damage, this->speed, this->lifetime, this->particle_emitter);

		return clone;
	}

	
	int getDamage() {
		return damage;
	}
	void setDamage(int damage) {
		this->damage = damage;
	}
	float getSpeed() {
		return speed;
	}
	void setSpeed(float speed) {
		this->speed = speed;
	}
	float getLifetime() {
		return lifetime;
	}
	void setLifetime(float lifetime) {
		this->lifetime = lifetime;
	}
	ParticleEmitter getParticleEmitter() {
		return this->particle_emitter;
	}
	void setParticleEmitter(ParticleEmitter particle_emitter) {
		this->particle_emitter = particle_emitter;
	}
};


// Example of spell changing step behaviour
class DeceleratingSpell : public ProjectileSpell {
private:
	float decelStrength;
public:
	DeceleratingSpell(float cooldown, TEXTURE_ASSET_ID asset_id, int damage, float speed, float lifetime, float decelStrength, ParticleEmitter particle_emitter)
		: ProjectileSpell(cooldown, asset_id, damage, speed, lifetime, particle_emitter) {
		this->decelStrength = decelStrength;
	}

	virtual void stepProjectile(Entity projectileEntity, float elapsed_ms);

	virtual DeceleratingSpell* clone() {
		DeceleratingSpell* clone = new DeceleratingSpell(getCooldown(), getAssetID(), getDamage(), getSpeed(), getLifetime(), this->decelStrength, getParticleEmitter());

		return clone;
	}

	float getDecelStrength() {
		return decelStrength;
	}
	void setDecelStrength(float decelStrength) {
		this->decelStrength = decelStrength;
	}
};

class AcceleratingSpell : public ProjectileSpell {
private:
	float accelStrength;
public:
	AcceleratingSpell(float cooldown, TEXTURE_ASSET_ID asset_id, int damage, float speed, float lifetime, float accelStrength, ParticleEmitter particle_emitter)
		: ProjectileSpell(cooldown, asset_id, damage, speed, lifetime, particle_emitter) {
		this->accelStrength = accelStrength;
	}

	virtual void stepProjectile(Entity projectileEntity, float elapsed_ms);

	virtual AcceleratingSpell* clone() {
		AcceleratingSpell* clone = new AcceleratingSpell(getCooldown(), getAssetID(), getDamage(), getSpeed(), getLifetime(), this->accelStrength, getParticleEmitter());

		return clone;
	}

	float getAccelStrength() {
		return accelStrength;
	}
	void setAccelStrength(float accelStrength) {
		this->accelStrength = accelStrength;
	}
};

// Example of spell changing cast behaviour
class ShotgunSpell : public ProjectileSpell {
private:
	int numProjectiles;
	float spreadDegrees;
public:
	ShotgunSpell(float cooldown, TEXTURE_ASSET_ID asset_id, int damage, float speed, float lifetime, int numProjectiles, float spreadDegrees, ParticleEmitter particle_emitter)
		: ProjectileSpell(cooldown, asset_id, damage, speed, lifetime, particle_emitter) {
		this->numProjectiles = numProjectiles;
		this->spreadDegrees = spreadDegrees;
	}

	void cast(RenderSystem* renderer, vec2 spawn_position, vec2 direction, PROJECTILE_SPELL_ID spell_id, Entity entity_type);

	virtual ShotgunSpell* clone() {
		ShotgunSpell* clone = new ShotgunSpell(getCooldown(), getAssetID(), getDamage(), getSpeed(), getLifetime(), this->numProjectiles, this->spreadDegrees, getParticleEmitter());

		return clone;
	}

	int getNumProjectiles() {
		return numProjectiles;
	}
	void setNumProjectiles(int numProjectiles) {
		this->numProjectiles = numProjectiles;
	}
	float getSpreadDegrees() {
		return spreadDegrees;
	}
	void setSpreadDegrees(float degrees) {
		this->spreadDegrees = degrees;
	}
};


// Example of spell on death behaviour
class ShrapnelSpell : public DeceleratingSpell {
private:
	int numShrapnel;
public:
	ShrapnelSpell(float cooldown, TEXTURE_ASSET_ID asset_id, int damage, float speed, float lifetime, float decelStrength, int numShrapnel, ParticleEmitter particle_emitter)
		: DeceleratingSpell (cooldown, asset_id, damage, speed, lifetime, decelStrength, particle_emitter) {
		this->numShrapnel = numShrapnel;
	}

	void onDeath(RenderSystem* renderer, Entity projectileEntity);

	virtual ShrapnelSpell* clone() {
		ShrapnelSpell* clone = new ShrapnelSpell(getCooldown(), getAssetID(), getDamage(), getSpeed(), getLifetime(), getDecelStrength(), this->numShrapnel, getParticleEmitter());

		return clone;
	}

	int getNumShrapnel() {
		return numShrapnel;
	}
	void setNumShrapnel(int numShrapnel) {
		this->numShrapnel = numShrapnel;
	}
};

// Example of spell on death behaviour
class BoomerangSpell : public DeceleratingSpell {
private:
	float rotationSpeed;
public:
	BoomerangSpell(float cooldown, TEXTURE_ASSET_ID asset_id, int damage, float speed, float lifetime, float decelStrength, float rotationSpeed, ParticleEmitter particle_emitter)
		: DeceleratingSpell(cooldown, asset_id, damage, speed, lifetime, decelStrength, particle_emitter) {
		this->rotationSpeed = rotationSpeed;
	}

	void onDeath(RenderSystem* renderer, Entity projectileEntity);

	void stepProjectile(Entity projectileEntity, float elapsed_ms);

	virtual BoomerangSpell* clone() {
		BoomerangSpell* clone = new BoomerangSpell(getCooldown(), getAssetID(), getDamage(), getSpeed(), getLifetime(), getDecelStrength(), this->rotationSpeed, getParticleEmitter());

		return clone;
	}

	void setRotationSpeed(float rotationSpeed) {
		this->rotationSpeed = rotationSpeed;
	}

	float getRotationSpeed() {
		return this->rotationSpeed;
	}
};

// Example of spell changing step behaviour
class SeekingSpell : public ProjectileSpell {
private:
	float seek_strength;

	Entity getNearestEnemyEntity(Entity projectileEntity);
	Entity getNearestEnemyToMouse();
public:
	SeekingSpell(float cooldown, TEXTURE_ASSET_ID asset_id, int damage, float speed, float lifetime, float seek_strength, ParticleEmitter particle_emitter)
		: ProjectileSpell(cooldown, asset_id, damage, speed, lifetime, particle_emitter) {
		this->seek_strength = seek_strength;
	}

	virtual void cast(RenderSystem* renderer, vec2 spawn_position, vec2 direction, PROJECTILE_SPELL_ID spell_id, Entity entity_type);

	virtual void stepProjectile(Entity projectileEntity, float elapsed_ms);

	virtual SeekingSpell* clone() {
		SeekingSpell* clone = new SeekingSpell(getCooldown(), getAssetID(), getDamage(), getSpeed(), getLifetime(), this->seek_strength, getParticleEmitter());

		return clone;
	}

	float getSeekStrength() {
		return seek_strength;
	}
	void setSeekStrength(float seek_strength) {
		this->seek_strength = seek_strength;
	}
};


// 
class BoomerangReturnSpell : public SeekingSpell {
private:
	float rotationSpeed;
public:
	BoomerangReturnSpell(float cooldown, TEXTURE_ASSET_ID asset_id, int damage, float speed, float lifetime, float seek_strength, float rotationSpeed, ParticleEmitter particle_emitter)
		: SeekingSpell(cooldown, asset_id, damage, speed, lifetime, seek_strength, particle_emitter) {
		this->rotationSpeed = rotationSpeed;
	}

	virtual void cast(RenderSystem* renderer, vec2 spawn_position, vec2 direction, PROJECTILE_SPELL_ID spell_id, Entity entity_type);

	virtual void stepProjectile(Entity projectileEntity, float elapsed_ms);

	virtual BoomerangReturnSpell* clone() {
		BoomerangReturnSpell* clone = new BoomerangReturnSpell(getCooldown(), getAssetID(), getDamage(), getSpeed(), getLifetime(), getSeekStrength(), this->rotationSpeed, getParticleEmitter());

		return clone;
	}

	void setRotationSpeed(float rotationSpeed) {
		this->rotationSpeed = rotationSpeed;
	}

	float getRotationSpeed() {
		return this->rotationSpeed;
	}
};




class RicochetSpell : public ProjectileSpell {
private:
	int numBounces;
public:
	RicochetSpell(float cooldown, TEXTURE_ASSET_ID asset_id, int damage, float speed, float lifetime, int numBounces, ParticleEmitter particle_emitter)
		: ProjectileSpell(cooldown, asset_id, damage, speed, lifetime, particle_emitter) {
		this->numBounces = numBounces;
	}

	void onDeath(RenderSystem* renderer, Entity projectileEntity);

	virtual RicochetSpell* clone() {
		RicochetSpell* clone = new RicochetSpell(getCooldown(), getAssetID(), getDamage(), getSpeed(), getLifetime(), this->numBounces, getParticleEmitter());

		return clone;
	}

	int getNumBounces() {
		return numBounces;
	}
	void setNumBounces(int numBounces) {
		this->numBounces = numBounces;
	}
};




class AreaOnDeathSpell : public ProjectileSpell {
private:
	float areaLifetime;
public:
	AreaOnDeathSpell(float cooldown, TEXTURE_ASSET_ID asset_id, int damage, float speed, float lifetime, float areaLifetime, ParticleEmitter particle_emitter)
		: ProjectileSpell(cooldown, asset_id, damage, speed, lifetime, particle_emitter) {
		this->areaLifetime = areaLifetime;
	}

	void onDeath(RenderSystem* renderer, Entity projectileEntity);

	virtual AreaOnDeathSpell* clone() {
		AreaOnDeathSpell* clone = new AreaOnDeathSpell(getCooldown(), getAssetID(), getDamage(), getSpeed(), getLifetime(), this->areaLifetime, getParticleEmitter());

		return clone;
	}

	float getAreaLifetime() {
		return areaLifetime;
	}
	void setAreaLifetime(float areaLifetime) {
		this->areaLifetime = areaLifetime;
	}
};





class ArcSpell : public ProjectileSpell {
private:
	float rotationSpeed;
public:
	ArcSpell(float cooldown, TEXTURE_ASSET_ID asset_id, int damage, float speed, float lifetime, float rotationSpeed, ParticleEmitter particle_emitter)
		: ProjectileSpell(cooldown, asset_id, damage, speed, lifetime, particle_emitter) {
		this->rotationSpeed = rotationSpeed;
	}

	void cast(RenderSystem* renderer, vec2 spawn_position, vec2 direction, PROJECTILE_SPELL_ID spell_id, Entity casted_by);

	void onDeath(RenderSystem* renderer, Entity projectileEntity);

	void stepProjectile(Entity projectileEntity, float elapsed_ms);

	virtual ArcSpell* clone() {
		ArcSpell* clone = new ArcSpell(getCooldown(), getAssetID(), getDamage(), getSpeed(), getLifetime(), this->rotationSpeed, getParticleEmitter());

		return clone;
	}

	void setRotationSpeed(float rotationSpeed) {
		this->rotationSpeed = rotationSpeed;
	}

	float getRotationSpeed() {
		return this->rotationSpeed;
	}
};

class ArcSpellLeft : public ProjectileSpell {
private:
	float rotationSpeed;
public:
	ArcSpellLeft(float cooldown, TEXTURE_ASSET_ID asset_id, int damage, float speed, float lifetime, float rotationSpeed, ParticleEmitter particle_emitter)
		: ProjectileSpell(cooldown, asset_id, damage, speed, lifetime, particle_emitter) {
		this->rotationSpeed = rotationSpeed;
	}

	void cast(RenderSystem* renderer, vec2 spawn_position, vec2 direction, PROJECTILE_SPELL_ID spell_id, Entity entity_type);

	void stepProjectile(Entity projectileEntity, float elapsed_ms);

	virtual ArcSpellLeft* clone() {
		ArcSpellLeft* clone = new ArcSpellLeft(getCooldown(), getAssetID(), getDamage(), getSpeed(), getLifetime(), this->rotationSpeed, getParticleEmitter());

		return clone;
	}

	void setRotationSpeed(float rotationSpeed) {
		this->rotationSpeed = rotationSpeed;
	}

	float getRotationSpeed() {
		return this->rotationSpeed;
	}
};

class ArcSpellRight : public ProjectileSpell {
private:
	float rotationSpeed;
public:
	ArcSpellRight(float cooldown, TEXTURE_ASSET_ID asset_id, int damage, float speed, float lifetime, float rotationSpeed, ParticleEmitter particle_emitter)
		: ProjectileSpell(cooldown, asset_id, damage, speed, lifetime, particle_emitter) {
		this->rotationSpeed = rotationSpeed;
	}

	void cast(RenderSystem* renderer, vec2 spawn_position, vec2 direction, PROJECTILE_SPELL_ID spell_id, Entity entity_type);

	void stepProjectile(Entity projectileEntity, float elapsed_ms);

	virtual ArcSpellRight* clone() {
		ArcSpellRight* clone = new ArcSpellRight(getCooldown(), getAssetID(), getDamage(), getSpeed(), getLifetime(), this->rotationSpeed, getParticleEmitter());

		return clone;
	}

	void setRotationSpeed(float rotationSpeed) {
		this->rotationSpeed = rotationSpeed;
	}

	float getRotationSpeed() {
		return this->rotationSpeed;
	}
};




// Projectile Particles

const int FIREBALL_PARTICLE_SPEED = 50;
const ParticleEmitter fireball_particles(
	12,													// number of particles
	vec4(0.647, 0.188, 0.188, 0.75),					// color
	vec2(-4, -4) * (float)PIXEL_SCALE_FACTOR,			// min pos offset (random)
	vec2(4, 4) * (float)PIXEL_SCALE_FACTOR,				// max pos offset
	vec2(-1, -1) * (float)FIREBALL_PARTICLE_SPEED,		// min velocity (random)
	vec2(1, 1) * (float)FIREBALL_PARTICLE_SPEED			// max velocity
);

const ParticleEmitter waterball_particles(
	12,							
	vec4(0.643, 0.867, 0.859, 0.75),
	vec2(-1, -1) * (float)PIXEL_SCALE_FACTOR,
	vec2(1, 1) * (float)PIXEL_SCALE_FACTOR,
	vec2(),
	vec2()
);

const ParticleEmitter shotgun_particles(
	10,						
	vec4(0, 0, 0, 0.5),
	vec2(-3, -3) * (float)PIXEL_SCALE_FACTOR,
	vec2(3, 3) * (float)PIXEL_SCALE_FACTOR,
	vec2(),
	vec2()
);

const int THORN_BOMB_PARTICLE_SPEED = 100;
const ParticleEmitter thorn_bomb_particles(
	10,							
	vec4(0.275, 0.51, 0.196, 0.5),
	vec2(-4, -4) * (float)PIXEL_SCALE_FACTOR,
	vec2(4, 4) * (float)PIXEL_SCALE_FACTOR,
	vec2(-1, -1) * (float)THORN_BOMB_PARTICLE_SPEED,
	vec2(1, 1) * (float)THORN_BOMB_PARTICLE_SPEED
);

const ParticleEmitter thorn_particles(
	10,							
	vec4(0.275, 0.51, 0.196, 0.5),
	vec2(-2, -2) * (float)PIXEL_SCALE_FACTOR,
	vec2(2, 2) * (float)PIXEL_SCALE_FACTOR,
	vec2(),
	vec2()
);

const ParticleEmitter boomerang_particles(
	15,							
	vec4(0.533, 0.294, 0.169, 0.75),
	vec2(-2, -2) * (float)PIXEL_SCALE_FACTOR,
	vec2(2, 2) * (float)PIXEL_SCALE_FACTOR,
	vec2(),
	vec2()
);

const ParticleEmitter magnet_particles(
	15,
	vec4(1, 1, 1, 0.5),
	vec2(-1, -1) * (float)PIXEL_SCALE_FACTOR,
	vec2(1, 1) * (float)PIXEL_SCALE_FACTOR,
	vec2(),
	vec2()
);

const ParticleEmitter red_orb_particles(
	5,											// number of particles
	vec4(0.62, 0, 0, 0.75),						// color
	vec2(-4, -4)* (float)PIXEL_SCALE_FACTOR,	// min pos offset (random)
	vec2(4, 4)* (float)PIXEL_SCALE_FACTOR,		// max pos offset
	vec2(-1, -1)* (float)10,					// min velocity (random)
	vec2(1, 1)* (float)10						// max velocity
);

const ParticleEmitter lightning_particles(
	20,
	vec4(1.0, 0.972, 0, 1.0),
	vec2(-1, -1)* (float)PIXEL_SCALE_FACTOR,
	vec2(1, 1)* (float)PIXEL_SCALE_FACTOR,
	vec2(),
	vec2()
);

const ParticleEmitter cutter_particles(
	10,
	vec4(0.712, 0.775, 1, 1.0),
	vec2(-1, -1)* (float)PIXEL_SCALE_FACTOR,
	vec2(1, 1)* (float)PIXEL_SCALE_FACTOR,
	vec2(),
	vec2()
);



// Projectile Declarations

const std::array<ProjectileSpell*, projectile_spell_count> projectile_spells = {
	new ProjectileSpell( // FIREBALL
		0.4f,							// cooldown
		TEXTURE_ASSET_ID::FIREBALL,		// asset id
		5,								// damage
		850,							// speed
		0.6f,							// lifetime (s)
		fireball_particles
	),
	new DeceleratingSpell( // WATERBALL
		0.35f,
		TEXTURE_ASSET_ID::WATERBALL,
		5,
		2000,
		0.75f,
		3000,							// deceleration (units per second)
		waterball_particles
	),
	new ShotgunSpell( // SHOTGUN
		1.1f,
		TEXTURE_ASSET_ID::SHOTGUN_PELLET,
		2,
		750,
		1.0f,
		7,								// Number of projectiles
		30.0f,							// Spread degrees
		shotgun_particles
	),
	new ShrapnelSpell( // THORN_BOMB
		1.0f,
		TEXTURE_ASSET_ID::THORN_BOMB,
		0,
		550,
		0.85f,
		500.0f,
		12,								// Number of shrapnel
		thorn_bomb_particles
	),
	new DeceleratingSpell( // THORN
		0.25f,
		TEXTURE_ASSET_ID::THORN,
		3,
		750,
		0.5f,
		1500,							// deceleration (units per second)
		thorn_particles
	),
	new BoomerangSpell( // BOOMERANG
		0.5f,
		TEXTURE_ASSET_ID::BOOMERANG,
		0,
		1100,
		0.5f,
		2000,							// deceleration (units per second)
		1440,							// Rotation of sprite (degrees per second)
		boomerang_particles
	),
	new BoomerangReturnSpell( // BOOMERANG_RETURN
		0.75f,
		TEXTURE_ASSET_ID::BOOMERANG,
		10,
		1000,
		20.0f,
		3000,							// Seek strength
		1440,
		boomerang_particles
	),
	new SeekingSpell( // MAGNET
		0.4f,
		TEXTURE_ASSET_ID::MAGNET,
		2,
		500,
		5.0f,
		2500,							// Seek strength
		magnet_particles
	),
	new ProjectileSpell( // LIGHTNING
		0.35f,
		TEXTURE_ASSET_ID::LIGHTNING,
		5,
		1200,
		1.5f,
		lightning_particles
	),
	new ShrapnelSpell( // ACID
		1.0f,
		TEXTURE_ASSET_ID::ACID,
		0,
		500,
		0.75f,
		750.0f,
		12,								// Number of shrapnel
		thorn_bomb_particles
	),
	new ProjectileSpell( // ACID_EFFECT
		0.3f,							// cooldown
		TEXTURE_ASSET_ID::ACID_EFFECT,		// asset id
		1,								// damage
		0,								// speed
		5.0f,							// lifetime (s)
		fireball_particles
	),
	new ArcSpell( // CUTTER
		0.55f,							// CD
		TEXTURE_ASSET_ID::CUTTER,
		0,								// Dmg
		1,								// Speed
		0.5f,							// Lifetime
		2000,							// Rotation spd
		cutter_particles
	),
	new ArcSpellLeft( // CUTTER_LEFT
		0,							// CD
		TEXTURE_ASSET_ID::CUTTER,
		8,								// Dmg
		1700,							// Speed
		0.75f,							// Lifetime
		2000,							// Rotation spd
		cutter_particles
	),
	new ArcSpellRight( // CUTTER_RIGHT
		0,							// CD
		TEXTURE_ASSET_ID::CUTTER,
		8,								// Dmg
		1700,							// Speed
		0.75f,							// Lifetime
		2000,							// Rotation spd
		cutter_particles
	),



	// Enemy Use
	new ProjectileSpell(// RED_ORB
		0.f,							// cooldown
		TEXTURE_ASSET_ID::RED_ORB,		// asset id
		8,								// damage
		330,							// speed
		10.0f,							// lifetime (s)
		red_orb_particles
	),
	new ProjectileSpell( // MEDIUM_SPEED_RED_ORB
		0.f,							// cooldown
		TEXTURE_ASSET_ID::RED_ORB,		// asset id
		15,								// damage
		250,							// speed
		10.0f,							// lifetime (s)
		red_orb_particles
	),
	new ProjectileSpell( // FAKE_MELEE
		1.f,
		TEXTURE_ASSET_ID::RED_ORB,
		8,
		2000,
		1.f,
		red_orb_particles
	),
	new ProjectileSpell( // BOSS_ORB
		0.f,
		TEXTURE_ASSET_ID::RED_ORB,
		8,
		570,
		10.f,
		red_orb_particles
	),
};










class MovementSpell : public Spell {
private:
	float distance;
public:
	MovementSpell(float cooldown, float distance, TEXTURE_ASSET_ID asset_id) : Spell(cooldown, asset_id) {
		this->distance = distance;

		setInternalCastCooldown(0.1);
	}

	// Takes renderer to render some effects, probably
	virtual void cast(RenderSystem* renderer, Entity entity, vec2 direction);

	virtual MovementSpell* clone() {
		MovementSpell* clone = new MovementSpell(getCooldown(), this->distance, getAssetID());

		return clone;
	}

	float getDistance() {
		return distance;
	}
	void setDistance(float distance) {
		this->distance = distance;
	}
};

class DashSpell : public MovementSpell {
private:
	// Munn: Use distance / duration to get speed? Or should we just use speed and distance, and not worry about setting a specific duration?
	float duration;
public:
	DashSpell(float cooldown, float distance, float duration, TEXTURE_ASSET_ID asset_id) : MovementSpell(cooldown, distance, asset_id) {
		this->duration = duration;
	}

	void cast(RenderSystem* renderer, Entity entity, vec2 direction);

	virtual DashSpell* clone() {
		DashSpell* clone = new DashSpell(getCooldown(), getDistance(), this->duration, getAssetID());

		return clone;
	}

	float getDuration() {
		return duration;
	}
	void setDuration(float duration) {
		this->duration = duration;
	}
};

class BlinkSpell : public MovementSpell {
private:
	// Munn: Use distance / duration to get speed? Or should we just use speed and distance, and not worry about setting a specific duration?
	float castTime;
public:
	BlinkSpell(float cooldown, float distance, float castTime, TEXTURE_ASSET_ID asset_id) : MovementSpell(cooldown, distance, asset_id) {
		this->castTime = castTime;
	}

	void cast(RenderSystem* renderer, Entity entity, vec2 direction);

	virtual BlinkSpell* clone() {
		BlinkSpell* clone = new BlinkSpell(getCooldown(), getDistance(), this->castTime, getAssetID());

		return clone;
	}

	float getCastTime() {
		return castTime;
	}
	void setCastTime(float castTime) {
		this->castTime = castTime;
	}
};

// Munn: Array of pointers, since an array of the objects would cast all the spells to DamageSpells, and we lose any inherited spells
const std::array<MovementSpell*, movement_spell_count> movement_spells = {
	new DashSpell(
		1.5f,				// Cooldown	
		250.0f,				// Distance Munn: I couldn't get the distance to travel perfectly this amount, so just treat this as some "strength" constant
		0.25f,				// Duration
		TEXTURE_ASSET_ID::DASH_ICON
	),
	new BlinkSpell( // should be blink
		1.5f,				// Cooldown	
		250.0f,				// Distance
		0.25f,				// Cast time eg. time between player pressing cast, and player moving
		TEXTURE_ASSET_ID::BLINK_ICON
	)
};










