#include "enemy_components.hpp"

float Enemy::getMaxHealth() {
	return max_health;
}

vec2 Enemy::getHitboxSize() {
	return hitbox_size;
}

CollisionMesh Enemy::getCollisionMesh() {
	return col_mesh;
}