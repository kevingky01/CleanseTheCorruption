#pragma once

#include "array"

#include "spells.hpp"
#include "tinyECS/components.hpp"
#include <iostream>


class Relic {
private:
	TEXTURE_ASSET_ID icon_asset;
public:
	Relic(TEXTURE_ASSET_ID icon_asset) {
		this->icon_asset = icon_asset;
	}

	virtual ProjectileSpell* modifyProjectileSpell(ProjectileSpell* spell);

	virtual MovementSpell* modifyMovementSpell(MovementSpell* spell);

	TEXTURE_ASSET_ID getIconAsset() {
		return this->icon_asset;
	}
	void setIconAsset() {
		this->icon_asset = icon_asset;
	}
};

class StrengthRelic : public Relic {
private:
	TEXTURE_ASSET_ID icon_asset;
public:
	StrengthRelic(TEXTURE_ASSET_ID icon_asset) : Relic(icon_asset) {}

	ProjectileSpell* modifyProjectileSpell(ProjectileSpell* spell);

	MovementSpell* modifyMovementSpell(MovementSpell* spell);
};

class TimeRelic : public Relic {
private:
	TEXTURE_ASSET_ID icon_asset;
public:
	TimeRelic(TEXTURE_ASSET_ID icon_asset) : Relic(icon_asset) {}

	ProjectileSpell* modifyProjectileSpell(ProjectileSpell* spell);

	MovementSpell* modifyMovementSpell(MovementSpell* spell);
};

class SpeedRelic : public Relic {
public:
	SpeedRelic(TEXTURE_ASSET_ID icon_asset) : Relic(icon_asset) {}

	ProjectileSpell* modifyProjectileSpell(ProjectileSpell* spell);

	MovementSpell* modifyMovementSpell(MovementSpell* spell);
};

class NumberRelic : public Relic {
public:
	NumberRelic(TEXTURE_ASSET_ID icon_asset) : Relic(icon_asset) {}

	ProjectileSpell* modifyProjectileSpell(ProjectileSpell* spell);

	MovementSpell* modifyMovementSpell(MovementSpell* spell);
};





const std::array<Relic*, relic_count> relics = {
	new StrengthRelic(TEXTURE_ASSET_ID::STRENGTH_RELIC),
	new TimeRelic(TEXTURE_ASSET_ID::TIME_RELIC),
	new SpeedRelic(TEXTURE_ASSET_ID::SPEED_RELIC),
	new NumberRelic(TEXTURE_ASSET_ID::NUMBER_RELIC)
};