#include "interactable.hpp"

#include "common.hpp"
#include "tinyECS/components.hpp"
#include "tinyECS/tiny_ecs.hpp"
#include "tinyECS/registry.hpp"

#include "world_init.hpp"
#include "render_system.hpp"

void ProjectileSpellDropItem::interact(Entity interactable_entity) {

	Entity player_entity = registry.players.entities[0];

	SpellSlotContainer& spell_slot_container = registry.spellSlotContainers.get(player_entity);
	SpellSlot& projectile_spell_slot = spell_slot_container.spellSlots[0];

	Interactable& interactable = registry.interactables.get(interactable_entity);

	Transformation& interactable_transform = registry.transforms.get(interactable_entity);
	Transformation& player_transform = registry.transforms.get(player_entity);
	interactable_transform.position = player_transform.position + vec2(0, TILE_SIZE);

	// Swap spells
	int player_old_spell = projectile_spell_slot.spell_id;
	projectile_spell_slot.spell_id = interactable.spell_id;
	interactable.spell_id = player_old_spell;

	createTextPopup("Swapped Spells!", vec3(1.0), 1.0, vec2(0.5), 0, player_transform.position, false);
}

void MovementSpellDropItem::interact(Entity interactable_entity) {

	Entity player_entity = registry.players.entities[0];

	SpellSlotContainer& spell_slot_container = registry.spellSlotContainers.get(player_entity);
	SpellSlot& movement_spell_slot = spell_slot_container.spellSlots[1]; // MovementSpel

	Interactable& interactable = registry.interactables.get(interactable_entity);

	Transformation& interactable_transform = registry.transforms.get(interactable_entity);
	Transformation& player_transform = registry.transforms.get(player_entity);
	interactable_transform.position = player_transform.position + vec2(0, TILE_SIZE);

	// Swap spells
	int player_old_spell = movement_spell_slot.spell_id;
	movement_spell_slot.spell_id = interactable.spell_id;
	interactable.spell_id = player_old_spell;
}

void HealthRestoreItem::interact(Entity interactable_entity) {

	Entity player_entity = registry.players.entities[0];

	Interactable& interactable = registry.interactables.get(interactable_entity);

	Health& player_health = registry.healths.get(player_entity);
	player_health.currentHealth += interactable.heal_amount;

	if (player_health.currentHealth > player_health.maxHealth) {
		player_health.currentHealth = player_health.maxHealth;
	}
	 
	Transformation& player_transform = registry.transforms.get(player_entity); 
	createTextPopup(std::to_string((int)interactable.heal_amount), HEALING_NUMBER_COLOR, 1.0, vec2(0.5), 0, player_transform.position, false);

	registry.remove_all_components_of(interactable_entity);
}

void NextLevelEntry::interact(Entity interactable_entity) {

	Entity player_entity = registry.players.entities[0];

	Interactable& interactable = registry.interactables.get(interactable_entity);
	interactable.can_interact = false;
	interactable.disabled = true;
}


void RelicDropItem::interact(Entity interactable_entity, bool apply_to_movement) {

	Entity player_entity = registry.players.entities[0];

	Interactable& interactable = registry.interactables.get(interactable_entity);

	SpellSlotContainer& player_spell_slots = registry.spellSlotContainers.get(player_entity);

	if (apply_to_movement) {
		SpellSlot& movement_spell_slot = player_spell_slots.spellSlots[1];
		movement_spell_slot.relics.push_back((RELIC_ID)interactable.relic_id);
	}
	else {
		SpellSlot& projectile_spell_slot = player_spell_slots.spellSlots[0];
		projectile_spell_slot.relics.push_back((RELIC_ID)interactable.relic_id);
	} 

	Transformation& player_transform = registry.transforms.get(player_entity);
	std::string text = "";
	switch (interactable.relic_id) {
	case (int)RELIC_ID::NUMBERS:
		text = "+1 Projectiles!";
		break;
	case (int)RELIC_ID::SPEED:
		text = "Projectile Speed Up!";
		break;
	case (int)RELIC_ID::STRENGTH:
		text = "Damage Up!";
		break;
	case (int)RELIC_ID::TIME:
		text = "Lowered Cooldown!";
		break;
	}
	 
	createTextPopup(text, vec3(1.0), 1.0, vec2(0.5), 0, player_transform.position, false);

	// Munn: Uncomment this, it's just for testing
	registry.remove_all_components_of(interactable_entity);
}

void FountainInteractable::interact(Entity interactable_entity) {

	Entity player_entity = registry.players.entities[0];

	Interactable& interactable = registry.interactables.get(interactable_entity);
	interactable.disabled = true;
	interactable.can_interact = false;

	Health& player_health = registry.healths.get(player_entity);
	player_health.currentHealth += interactable.heal_amount;

	if (player_health.currentHealth > player_health.maxHealth) {
		player_health.currentHealth = player_health.maxHealth;
	} 

	RenderRequest& rr = registry.renderRequests.get(interactable_entity);
	rr.used_texture = TEXTURE_ASSET_ID::FOUNTAIN_EMPTY;
	rr.used_effect = EFFECT_ASSET_ID::TEXTURED;
	rr.used_geometry = GEOMETRY_BUFFER_ID::SPRITE;
	 
	Transformation& player_transform = registry.transforms.get(player_entity); 
	createTextPopup("Full Heal!", HEALING_NUMBER_COLOR, 1.0, vec2(0.5), 0, player_transform.position, false);

	registry.animation_managers.remove(interactable_entity);
}

// Sacrifice half your health to spawn 2 random relics
void SacrificeFountainInteractable::interact(RenderSystem* renderer, Entity interactable_entity) {

	Entity player_entity = registry.players.entities[0];

	Interactable& interactable = registry.interactables.get(interactable_entity);
	interactable.disabled = true;
	interactable.can_interact = false;

	Health& player_health = registry.healths.get(player_entity);
	int damage = ceil(player_health.currentHealth / 2.0f);
	player_health.currentHealth = damage;

	Transformation& transform = registry.transforms.get(interactable_entity);

	createRandomRelic(renderer, transform.position + vec2(2.5, 1) * (float)TILE_SIZE);
	createRandomRelic(renderer, transform.position + vec2(-2.5, 1) * (float)TILE_SIZE);

	RenderRequest& rr = registry.renderRequests.get(interactable_entity);
	rr.used_texture = TEXTURE_ASSET_ID::SACRIFICE_FOUNTAIN_EMPTY;
	rr.used_effect = EFFECT_ASSET_ID::TEXTURED;
	rr.used_geometry = GEOMETRY_BUFFER_ID::SPRITE;

	registry.animation_managers.remove(interactable_entity); 

	Entity screen_state_entity = registry.screenStates.entities[0];
	ScreenState& screen_state = registry.screenStates.get(screen_state_entity);

	screen_state.vignette_factor = 1.0;
	screen_state.vignette_persist_duration = 0.5;
 
	Transformation& player_transform = registry.transforms.get(player_entity);
	std::string text = std::to_string((int)damage);
	createTextPopup(text, DAMAGE_NUMBER_COLOR, 1.0, vec2(0.5), 0, player_transform.position, false);
}



// To be accessed globally
ProjectileSpellDropItem projectile_spell_interactable = ProjectileSpellDropItem();
MovementSpellDropItem movement_spell_interactable = MovementSpellDropItem();
HealthRestoreItem health_restore_interactable = HealthRestoreItem();

NextLevelEntry entry_interactable = NextLevelEntry();
RelicDropItem relic_interactable = RelicDropItem();
FountainInteractable fountain_interactable = FountainInteractable();
SacrificeFountainInteractable sacrifice_fountain_interactable = SacrificeFountainInteractable();