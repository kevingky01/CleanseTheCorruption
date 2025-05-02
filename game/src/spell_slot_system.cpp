
#include "spell_slot_system.hpp"
#include <iostream>

// Decrement spell slot cooldown
void SpellSlotSystem::step(float elapsed_ms) {
	for (SpellSlotContainer& spellSlotContainer : registry.spellSlotContainers.components) {
		for (SpellSlot& spellSlot : spellSlotContainer.spellSlots) {
			if (spellSlot.remainingCooldown > 0.0) {
				spellSlot.remainingCooldown -= elapsed_ms / 1000.0f;
			} 

			if (spellSlot.internalCooldown > 0.0) {
				spellSlot.internalCooldown -= elapsed_ms / 1000.0f;
			}
		}
	}
}