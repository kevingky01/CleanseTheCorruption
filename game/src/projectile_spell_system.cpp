#include "projectile_spell_system.hpp"
#include "spells.hpp"
#include <iostream>
#include "world_init.hpp"


void ProjectileSpellSystem::step(float elapsed_ms) {
	for (Entity& projectileEntity : registry.projectiles.entities) {

		if (!registry.projectiles.has(projectileEntity)) {
			continue;
		}
		
		Projectile& projectile = registry.projectiles.get(projectileEntity);

		if (projectile.is_dead) {
			continue;
		}

		ProjectileSpell* spell = projectile_spells[(int)projectile.spell_id];
		
		float stepSeconds = elapsed_ms / 1000.0f;

		// Update and check projectile lifetime
		projectile.lifetime -= stepSeconds;
		if (projectile.lifetime <= 0) {
			spell->onDeath(renderer, projectileEntity); // Munn: renderer is passed so we can use it to create new render request entities
			continue;
		}

		// Update projectile behaviour
		spell->stepProjectile(projectileEntity, elapsed_ms);
	}
}