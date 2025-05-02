# pragma once

#include "array"

#include "common.hpp"
#include "tinyECS/components.hpp"
#include "tinyECS/tiny_ecs.hpp"
#include "tinyECS/registry.hpp"

#include "render_system.hpp"

class InteractableItem {
public:
	InteractableItem() {}

	virtual void interact() {};
};

class ProjectileSpellDropItem : public InteractableItem {
private:

public:
	ProjectileSpellDropItem() {}

	void interact(Entity interactable_entity);
};

class MovementSpellDropItem : public InteractableItem {
private:

public:
	MovementSpellDropItem() {}

	void interact(Entity interactable_entity);
};

class HealthRestoreItem {
public:
	HealthRestoreItem() {}

	void interact(Entity interactable_entity);
};

class NextLevelEntry {
public:
	NextLevelEntry() {}

	void interact(Entity interactable_entity);

};

class RelicDropItem {
public:
	RelicDropItem() {}

	void interact(Entity interactable_entity, bool apply_to_movement);
};


class FountainInteractable {
public:
	FountainInteractable() {}

	void interact(Entity interactable_entity);
};

class SacrificeFountainInteractable {
public:
	SacrificeFountainInteractable() {}

	void interact(RenderSystem* renderer, Entity interactable_entity);
};


// Interactable objects (they just define behaviour, no data is stored)
extern ProjectileSpellDropItem projectile_spell_interactable;
extern MovementSpellDropItem movement_spell_interactable;
extern HealthRestoreItem health_restore_interactable;

extern NextLevelEntry entry_interactable; 
extern RelicDropItem relic_interactable;

extern FountainInteractable fountain_interactable;
extern SacrificeFountainInteractable sacrifice_fountain_interactable;