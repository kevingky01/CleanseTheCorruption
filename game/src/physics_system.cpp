// internal
#include "physics_system.hpp"
#include "world_init.hpp"
#include "render_system.hpp"
#include <iostream>
#include <cmath>


std::vector<vec2> getWorldPoints(Entity e)
{
	// Jason: apply the transformation for every vertex in an convage polygon
	//		  Before we only have a single vertex for every object, now we have
	//		  multiple vertices for each object. Thus we need to manually code the transformation.
    auto& mesh = registry.collisionMeshes.get(e);
    auto& t    = registry.transforms.get(e);

    std::vector<vec2> result;
    result.reserve(mesh.local_points.size());


	float rad = t.angle * (M_PI / 180.f);

    for (vec2 p : mesh.local_points)
    {
        // Scale
        vec2 scaled = vec2(
            p.x * t.scale.x * PIXEL_SCALE_FACTOR,
            p.y * t.scale.y * PIXEL_SCALE_FACTOR
        );
        // Rotate
        float rx = scaled.x * cos(rad) - scaled.y * sin(rad);
        float ry = scaled.x * sin(rad) + scaled.y * cos(rad);

        // Translate
        vec2 worldPos = t.position + vec2(rx, ry);

        result.push_back(worldPos);
    }
    return result;
}


bool polygonsCollide(const std::vector<vec2>& polyA, 
                     const std::vector<vec2>& polyB)
{
    // Collect the axes (normals) from edges
    std::vector<vec2> axes;
    getAxes(polyA, axes);
    getAxes(polyB, axes);

    // Check for overlap along each axis, foe
    for (auto axis : axes)
    {
        float minA, maxA;
        projectPolygon(polyA, axis, minA, maxA);

        float minB, maxB;
        projectPolygon(polyB, axis, minB, maxB);

        // If there's a gap found at any axis, no collision
        if (maxA < minB || maxB < minA) {
            return false;
        }
    }
    return true; // Overlapped on all axes, there is collision
}

void getAxes(const std::vector<vec2>& poly, std::vector<vec2>& axesOut)
{
    for (size_t i = 0; i < poly.size(); i++)
    {
        vec2 p1 = poly[i];
        vec2 p2 = poly[(i+1) % poly.size()]; // wrap around the last local point with the first. The shape is defined by the local points, ordering matters 
        vec2 edge = p2 - p1;

        // calculate normal vector
        vec2 normal = vec2(-edge.y, edge.x);


        axesOut.push_back(normal);
    }
	
}
void projectPolygon(const std::vector<vec2>& poly, 
                    const vec2& axis, 
                    float& outMin, float& outMax)
{
    float first = dot(poly[0], axis);
    outMin = first;
    outMax = first;

    for (size_t i = 1; i < poly.size(); i++)
    {
        float val = dot(poly[i], axis);
        if (val < outMin) outMin = val;
        if (val > outMax) outMax = val;
    }
}

// Returns the local bounding coordinates scaled by the current size of the entity
vec2 get_bounding_box(const Transformation& transform, vec2 texture_dimension)
{
	vec2 scale = vec2(
		// multiplying by texture_dimension so bbox scales correctly with sprite
		abs(transform.scale.x) * PIXEL_SCALE_FACTOR * texture_dimension.x,
		abs(transform.scale.y) * PIXEL_SCALE_FACTOR * texture_dimension.y
	);
	
	return scale;
}

bool collideAABB(Entity entity_i, Entity entity_j){
	Hitbox hitbox_i = registry.hitboxes.get(entity_i);
	ivec2 i_dimension = hitbox_i.hitbox_scale;
	Hitbox hitbox_j = registry.hitboxes.get(entity_j);
	ivec2 j_dimension = hitbox_j.hitbox_scale;

	Transformation& transform1 = registry.transforms.get(entity_i);
	Transformation& transform2 = registry.transforms.get(entity_j);
	

	vec2 box1 = get_bounding_box(transform1, i_dimension);
	vec2 box2 = get_bounding_box(transform2, j_dimension);

	// Calculate the half-dimensions of each box
	vec2 half1 = box1 / 2.f;
	vec2 half2 = box2 / 2.f;

	// Calculate the distance between centers
	vec2 dist = abs(transform1.position - transform2.position);

	// Check for overlap on both axes
	return (dist.x < (half1.x + half2.x)) && (dist.y < (half1.y + half2.y));
}



// If the entity has a collision mesh, collide using collision meshes
bool collides(Entity entity_i, Entity entity_j)
{
	// AABB for now, since it is cheaper (hopefully)
	return collideAABB(entity_i, entity_j);

	// // NEW
	bool hasMesh1 = registry.collisionMeshes.has(entity_i);
    bool hasMesh2 = registry.collisionMeshes.has(entity_j);
	if (hasMesh1 && hasMesh2)
    {
        std::vector<vec2> polyA = getWorldPoints(entity_i);
        std::vector<vec2> polyB = getWorldPoints(entity_j);

        return polygonsCollide(polyA, polyB);
    } else {
		
	}
	// return collideAABB(entity_i, entity_j);

}





void PhysicsSystem::step(float elapsed_ms)
{
	// Munn: WE REALLY NEED TO OPTIMIZE COLLISIONS!
	const float COLLISION_CHECK_DISTANCE = 30 * TILE_SIZE;

	Entity player_entity = registry.players.entities[0];
	Transformation& player_transform = registry.transforms.get(player_entity);

	// Move each entity that has motion (invaders, projectiles, and even towers [they have 0 for velocity])
	// based on how much time has passed, this is to (partially) avoid
	// having entities move at different speed based on the machine.
	auto& motion_registry = registry.motions;
	for(uint i = 0; i< motion_registry.size(); i++)
	{
		// update motion.position based on step_seconds and motion.velocity
		Motion& motion = motion_registry.components[i];
		Entity entity = motion_registry.entities[i];
		float step_seconds = elapsed_ms / 1000.f;
		
		// Guo: get position associated with the entity that is in motion
		auto& position = registry.transforms.get(entity).position;

		if (glm::distance(player_transform.position, position) > COLLISION_CHECK_DISTANCE) {
			continue;
		}

		position += motion.velocity * step_seconds;
	}

	int num_collisions_checked = 0;
	
	// check for collisions between all moving entities
	// then check whether their collision matters or not
    //ComponentContainer<Hitbox> &hitbox_container = registry.hitboxes;
	for(Entity entity_i : registry.hitboxes.entities)
	{  
		if (!registry.hitboxes.has(entity_i))
			continue;

		Transformation& transformation_i = registry.transforms.get(entity_i);

		if (glm::distance(transformation_i.position, player_transform.position) > COLLISION_CHECK_DISTANCE) {
			continue;
		}
		
		// note starting j at i+1 to compare all (i,j) pairs only once (and to not compare with itself)
		for(Entity entity_j : registry.hitboxes.entities)
		{ 
			if (entity_i == entity_j)
				continue;

			Transformation& transformation_j = registry.transforms.get(entity_j);

			if (glm::distance(transformation_j.position, player_transform.position) > COLLISION_CHECK_DISTANCE) {
				continue;
			}

			num_collisions_checked++;

			Hitbox& hitbox_i = registry.hitboxes.get(entity_i);
			Hitbox& hitbox_j = registry.hitboxes.get(entity_j);
			bool i_j_detect = (hitbox_i.layer & hitbox_j.mask) != 0;
			bool j_i_detect = (hitbox_j.layer & hitbox_i.mask) != 0;

			if (i_j_detect || j_i_detect)
			{
				if (collides(entity_i, entity_j))
				{ 
					// Remove duplicates
					if (registry.collisions.has(entity_j)) {
						Collision collision = registry.collisions.get(entity_j);

						if (collision.other == entity_i) {
							continue;
						}
					}
					// Create a collisions event
					// We are abusing the ECS system a bit in that we potentially insert multiple collisions for the same entity
					// CK: why the duplication, except to allow searching by entity_id
					registry.collisions.emplace_with_duplicates(entity_i, entity_j);
					// registry.collisions.emplace_with_duplicates(entity_j, entity_i);
				}
			}
		}
	}

	// std::cout << num_collisions_checked << std::endl;
}



