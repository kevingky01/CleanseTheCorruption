#include "particle_system.hpp"

#include "common.hpp"
#include "tinyECS/tiny_ecs.hpp"
#include "tinyECS/components.hpp"
#include "tinyECS/registry.hpp"


void ParticleSystem::step(float elapsed_ms) {
	for (ParticleEmitterContainer& particle_emitter_container : registry.particle_emitter_containers.components) {
		for (auto& pair : particle_emitter_container.particle_emitter_map) {
			ParticleEmitter& particle_emitter = pair.second;
			updateParticles(particle_emitter, elapsed_ms);
		}
	}
}

void ParticleSystem::updateParticles(ParticleEmitter& particle_emitter, float elapsed_ms) {

	int numNewParticles = 1; // MunnL Create 1 particle per frame (we usually won't make more (?)) 

	float stepSeconds = elapsed_ms / 1000.0f;

	particle_emitter.current_respawn_time += stepSeconds;

	for (int i = 0; i < numNewParticles; i++) {
		int unusedParticle = getFirstUnusedParticle(particle_emitter); // This is actually inefficient, see https://learnopengl.com/In-Practice/2D-Game/Particles for better way to iterate
		if (unusedParticle < 0) { // All particles are being used
			break;
		}
		if (particle_emitter.current_respawn_time > particle_emitter.loop_duration / particle_emitter.max_particles) {

			// Don't respawn particles if not emitting, but still continue to update other particles
			if (particle_emitter.is_emitting) {
				respawnParticle(particle_emitter, particle_emitter.particles[unusedParticle]);
				particle_emitter.current_respawn_time = 0;
			}
		}
	}


	for (int i = 0; i < particle_emitter.max_particles; i++) {
		Particle& particle = particle_emitter.particles[i];

		if (particle.lifetime <= 0) { // ignore particles that have "died"
			//std::cout << "Particle " << i << " is dead!" << std::endl;
			continue;
		}

		particle.lifetime -= stepSeconds;

		particle.position += particle.velocity * stepSeconds; // update particle position
		particle.color.w -= (particle_emitter.color_i.w / particle.lifetime) * stepSeconds; // fade particle out
	}
}


void ParticleSystem::respawnParticle(ParticleEmitter& particle_emitter, Particle& particle) {
	vec2 random_position = vec2((uniform_dist(rng) - 0.5) * (particle_emitter.random_position_max.x - particle_emitter.random_position_min.x),
		(uniform_dist(rng) - 0.5) * (particle_emitter.random_position_max.y - particle_emitter.random_position_min.y));
	vec2 random_velocity = vec2((uniform_dist(rng) - 0.5) * (particle_emitter.random_velocity_max.x - particle_emitter.random_velocity_min.x),
		(uniform_dist(rng) - 0.5) * (particle_emitter.random_velocity_max.y - particle_emitter.random_velocity_max.y));

	vec2 parent_position = vec2(0, 0);
	vec2 parent_scale = vec2(1, 1);
	if (particle_emitter.parent_entity != -1) {
		Transformation parent_transform = registry.transforms.get(particle_emitter.parent_entity);

		parent_position = parent_transform.position;
		parent_scale = parent_transform.scale;
	}

	particle.position = parent_position + particle_emitter.position_i + random_position;
	particle.velocity = particle_emitter.velocity_i + random_velocity;
	particle.scale = vec2(parent_scale.x * particle_emitter.scale_i.x, parent_scale.y * particle_emitter.scale_i.y);
	particle.color = particle_emitter.color_i;
	particle.lifetime = particle_emitter.loop_duration;
}

int ParticleSystem::getFirstUnusedParticle(ParticleEmitter& particle_emitter) {
	for (int i = 0; i < particle_emitter.max_particles; i++) {
		if (particle_emitter.particles[i].lifetime <= 0) {
			return i;
		}
	}

	return -1;
}