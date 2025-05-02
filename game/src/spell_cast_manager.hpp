#pragma once

#include "tinyECS/components.hpp"
#include "tinyECS/entity.hpp"
#include "tinyECS/registry.hpp"

#include "spells.hpp"
#include "relics.hpp"
#include "render_system.hpp"
#include <iostream>

#include "world_init.hpp"


// Static spell cast manager 

class SpellCastManager {
private:

	static Spell applyRelicsAndCast(RenderSystem* renderer, SpellSlot& spell_slot, vec2 spawn_position, vec2 direction, Entity casted_by) {
		switch (spell_slot.spell_type) {


		case (SPELL_TYPE::PROJECTILE): {

			// Get base spell
			ProjectileSpell* projectile_spell = projectile_spells[spell_slot.spell_id];


			// Copy base spell (must be a pointer, otherwise it will cast to a ProjectileSpell, not an inherited class of ProjectileSpell)
			// Munn: a solution to this is to add a Spell object to each projectile struct, but that causes a circular dependency, and I can't think of a solution to fix that?
			ProjectileSpell* new_spell = projectile_spell->clone();


			// Apply relics to copied spell
			for (int i = 0; i < spell_slot.relics.size(); i++) {

				RELIC_ID relic_id = spell_slot.relics[i];
				Relic* relic = relics[(int)relic_id];
				new_spell = relic->modifyProjectileSpell(new_spell);

				//std::cout << "Applied relic: " << (int)relic_id << " to projectile spell" << std::endl;
			}

			// Cast copied spell

			new_spell->cast(renderer, spawn_position, direction, (PROJECTILE_SPELL_ID)spell_slot.spell_id, casted_by);

			// Copy spell to get cooldown information
			Spell spell = *new_spell;

			// Delete spell pointer
			delete new_spell;
			
			// Return spell for cooldown information
			return spell;
		}
		case (SPELL_TYPE::MOVEMENT): {
			// Get base spell
			MovementSpell* movement_spell = movement_spells[spell_slot.spell_id];

			// Copy base spell
			MovementSpell* new_spell = movement_spell->clone();

			// Apply relics to copied spell
			for (int i = 0; i < spell_slot.relics.size(); i++) {
				RELIC_ID relic_id = spell_slot.relics[i];
				Relic* relic = relics[(int)relic_id];
				new_spell = relic->modifyMovementSpell(new_spell);
			}

			new_spell->cast(renderer, casted_by, direction);

			spell_slot.remainingCooldown = new_spell->getCooldown();

			// Copy spell to get cooldown information
			Spell spell = *new_spell;

			// Delete spell pointer
			delete new_spell;

			// Return spell for cooldown information
			return spell;
		}
		default: {
			std::cout << "Error: Invalid spell type casted: " << (int)spell_slot.spell_type << std::endl;
			return Spell();
		}
		}
	}
	
public:
	SpellCastManager() {}

	// Cast a spell, set spell_slot cooldown
	static void castSpell(RenderSystem* renderer, SpellSlot& spell_slot, vec2 spawn_position, vec2 direction, Entity casted_by) {

		if (spell_slot.internalCooldown > 0) {
			return;
		}

		if (spell_slot.remainingCooldown > 0) {
			if (spell_slot.num_casts > 0) {
				spell_slot.num_casts -= 1;
				Spell spell = applyRelicsAndCast(renderer, spell_slot, spawn_position, direction, casted_by);

				spell_slot.internalCooldown = spell.getInternalCastCooldown();
			}

			return;
		}

		Spell spell = applyRelicsAndCast(renderer, spell_slot, spawn_position, direction, casted_by);

		spell_slot.num_casts = spell.getNumCasts() - 1;

		spell_slot.internalCooldown = spell.getInternalCastCooldown();
		spell_slot.remainingCooldown = spell.getCooldown(); 

	}

	static void castBossSpell(RenderSystem* renderer, SpellSlot& spell_slot, vec2 spawn_position, vec2 direction, Entity casted_by) {
		Spell spell = applyRelicsAndCast(renderer, spell_slot, spawn_position, direction, casted_by);
	}
	
	// Cast a spell consecutively
	static void castBurstSpell(RenderSystem* renderer, SpellSlot& spell_slot, vec2 spawn_position, vec2 direction, Entity casted_by, int burst_count, float delay_ms) {
		for (int i = 0; i < burst_count; i++) {
			std::function<void()> delayed_cast = [renderer, &spell_slot, spawn_position, direction, casted_by]() {
				// Only cast if enemy is still alive - otherwise cast should be interrupted
				if (registry.enemies.has(casted_by)) {
					// Update position for subsequent casts
					vec2 current_position = registry.transforms.get(casted_by).position;
					applyRelicsAndCast(renderer, spell_slot, current_position, direction, casted_by);
				}
			};
			
			createTimer(delay_ms * i, delayed_cast);
		}
	}

};