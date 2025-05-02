#include "relics.hpp"
#include "spells.hpp"

#include "common.hpp"

#include "tinyECS/registry.hpp"
#include "tinyECS/components.hpp"
#include "tinyECS/entity.hpp"


// Base relic has no effects
ProjectileSpell* Relic::modifyProjectileSpell(ProjectileSpell* spell) {
	return spell;
}

MovementSpell* Relic::modifyMovementSpell(MovementSpell* spell) {
	return spell;
}



const int DAMAGE_INCREASE = 1;
ProjectileSpell* StrengthRelic::modifyProjectileSpell(ProjectileSpell* spell) {
	spell->setDamage(spell->getDamage() + DAMAGE_INCREASE);
	return spell;
}

const int DISTANCE_INCREASE = 100;
MovementSpell* StrengthRelic::modifyMovementSpell(MovementSpell* spell) {
	spell->setDistance(spell->getDistance() + DISTANCE_INCREASE);
	return spell;
}




const float COOLDOWN_REDUCTION = 0.02;
const float MIN_PROJECTILE_COOLDOWN = 0.1;
ProjectileSpell* TimeRelic::modifyProjectileSpell(ProjectileSpell* spell) {
	float new_cooldown = max(MIN_PROJECTILE_COOLDOWN, spell->getCooldown() - COOLDOWN_REDUCTION);
	spell->setCooldown(new_cooldown);
	return spell;
}

const float MIN_MOVEMENT_COOLDOWN = 0.25;
MovementSpell* TimeRelic::modifyMovementSpell(MovementSpell* spell) {
	float new_cooldown = max(MIN_MOVEMENT_COOLDOWN, spell->getCooldown() - COOLDOWN_REDUCTION);
	spell->setCooldown(new_cooldown);
	return spell;
}



const float SPEED_INCREASE = 25;
ProjectileSpell* SpeedRelic::modifyProjectileSpell(ProjectileSpell* spell) {
	spell->setSpeed(spell->getSpeed() + SPEED_INCREASE);
	return spell;
}


const float MIN_DASH_DURATION = 0.1;
const float DASH_DURATION_DECREASE = 0.05;

const float MIN_CAST_TIME = 0;
const float CAST_TIME_DECREASE = 0.05;
MovementSpell* SpeedRelic::modifyMovementSpell(MovementSpell* spell) {

	// Munn: This is a really scuffed check to see what kind of movement spell it is...
	if (spell->getAssetID() == TEXTURE_ASSET_ID::DASH_ICON) {
		// Cast this spell to a dash spell and adjust parameters
		DashSpell* new_spell = (DashSpell*)spell;

		float new_duration = max(MIN_DASH_DURATION, new_spell->getDuration() - DASH_DURATION_DECREASE);
		new_spell->setDuration(new_duration);

		spell = new_spell;
	}
	else if (spell->getAssetID() == TEXTURE_ASSET_ID::BLINK_ICON) {
		BlinkSpell* new_spell = (BlinkSpell*)spell;

		float new_cast_time = max(MIN_DASH_DURATION, new_spell->getCastTime() - CAST_TIME_DECREASE);
		new_spell->setCastTime(new_cast_time);

		spell = new_spell;
	}
	return spell;
}




ProjectileSpell* NumberRelic::modifyProjectileSpell(ProjectileSpell* spell) {
	spell->setNumCasts(spell->getNumCasts() + 1);
	return spell;
}

MovementSpell* NumberRelic::modifyMovementSpell(MovementSpell* spell) {
	spell->setNumCasts(spell->getNumCasts() + 1);
	return spell;
}