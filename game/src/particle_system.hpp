#pragma once

#include "common.hpp"
#include "tinyECS/tiny_ecs.hpp"
#include "tinyECS/components.hpp"
#include "tinyECS/registry.hpp"

class ParticleSystem {
public:
	void step(float elapsed_ms);

	ParticleSystem() {}

private: 

	void updateParticles(ParticleEmitter& particle_emitter, float elapsed_ms);

	void respawnParticle(ParticleEmitter& particle_emitter, Particle& particle);

	int getFirstUnusedParticle(ParticleEmitter& particle_emitter);
};