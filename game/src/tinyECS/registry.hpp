#pragma once
#include <vector>

#include "tiny_ecs.hpp"
#include "components.hpp"
#include <enemy_types/enemy_components.hpp>
#include "enemy_room_manager.hpp"

class ECSRegistry
{
public:
	// callbacks to remove a particular or all entities in the system
	std::vector<ContainerInterface*> registry_list;

	// Manually created list of all components this game has
	ComponentContainer<DeathTimer> deathTimers;
	ComponentContainer<Motion> motions;
	ComponentContainer<Transformation> transforms; // separated position from motion
	ComponentContainer<Collision> collisions;
	ComponentContainer<Player> players;
	ComponentContainer<Mesh*> meshPtrs;
	ComponentContainer<RenderRequest> renderRequests;
	ComponentContainer<ScreenState> screenStates;
	ComponentContainer<Eatable> eatables;
	ComponentContainer<Deadly> deadlys;
	ComponentContainer<DebugComponent> debugComponents;
	ComponentContainer<vec3> colors;
	// MAP / ENVIRONMENT
	ComponentContainer<Floor> floors;
	ComponentContainer<Wall> walls;
	ComponentContainer<Chest> chests;
	ComponentContainer<Room> rooms;
	// IMPORTANT: Add any new CC's below to the registry_list
	ComponentContainer<Tower> towers;
	ComponentContainer<GridLine> gridLines; 
	ComponentContainer<Enemy*> enemies; // Enemies identification 
	ComponentContainer<Projectile> projectiles;
	ComponentContainer<Timer> timers;
	ComponentContainer<AnimationManager> animation_managers;
	ComponentContainer<ParticleEmitterContainer> particle_emitter_containers;


	// New ones
	ComponentContainer<SpellSlot> spellSlots;
	ComponentContainer<Hitbox> hitboxes;
	ComponentContainer<Camera> cameras;
	ComponentContainer<SpellSlotContainer> spellSlotContainers;
	ComponentContainer<Health> healths;
	ComponentContainer<Boss> bosses;
	ComponentContainer<Tile> tiles;
	ComponentContainer<WallCollision> wallCollisions;
	ComponentContainer<Interactable> interactables;
	ComponentContainer<Seeking> seekings;
	ComponentContainer<Lootable> lootables;
	
	ComponentContainer<CollisionMesh> collisionMeshes;

	ComponentContainer<TutorialUse> tutorialUses;

	ComponentContainer<FloorDecor> floorDecors;
	ComponentContainer<Tween> tweens;
	ComponentContainer<EnemyRoomManager> enemyRoomManagers;
	ComponentContainer<EnvironmentObject> environmentObjects;

	ComponentContainer<GoalManager> goalManagers;
	ComponentContainer<Text> texts;
	ComponentContainer<TextPopup> textPopups;
	ComponentContainer<NPC> npcs;
	ComponentContainer<Minimap> minimaps;

	ComponentContainer<BackgroundImage> backgroundImages;

	ComponentContainer<DialogueBox> dialogueBoxes;
	ComponentContainer<Slide_Bar> slideBars;
	ComponentContainer<Slide_Block> slideBlocks;

	// constructor that adds all containers for looping over them
	ECSRegistry()
	{
		// TODO: A1 add a LightUp component
		registry_list.push_back(&deathTimers);
		registry_list.push_back(&motions);
		registry_list.push_back(&transforms);
		registry_list.push_back(&collisions);
		registry_list.push_back(&players);
		registry_list.push_back(&meshPtrs);
		registry_list.push_back(&renderRequests);
		registry_list.push_back(&screenStates);
		registry_list.push_back(&eatables);
		registry_list.push_back(&deadlys);
		registry_list.push_back(&debugComponents);
		registry_list.push_back(&colors);

		registry_list.push_back(&floors);
		registry_list.push_back(&walls);
		registry_list.push_back(&chests);
		registry_list.push_back(&rooms);

		registry_list.push_back(&towers);
		registry_list.push_back(&gridLines);
		registry_list.push_back(&enemies);
		registry_list.push_back(&projectiles);
		
		registry_list.push_back(&timers);
		registry_list.push_back(&animation_managers);
		registry_list.push_back(&particle_emitter_containers);


		// New ones
		registry_list.push_back(&spellSlots);
		registry_list.push_back(&hitboxes);
		registry_list.push_back(&cameras);
		registry_list.push_back(&spellSlotContainers);
		registry_list.push_back(&healths);
		registry_list.push_back(&bosses);
		registry_list.push_back(&tiles);

		registry_list.push_back(&wallCollisions);
		registry_list.push_back(&interactables);
		registry_list.push_back(&seekings);
		registry_list.push_back(&lootables);

		registry_list.push_back(&tutorialUses);
		registry_list.push_back(&floorDecors);
		registry_list.push_back(&collisionMeshes);
		registry_list.push_back(&tweens);
		registry_list.push_back(&enemyRoomManagers); 
		registry_list.push_back(&environmentObjects);
		registry_list.push_back(&goalManagers);
		registry_list.push_back(&texts);
		registry_list.push_back(&textPopups);
		registry_list.push_back(&npcs);
		registry_list.push_back(&minimaps);
		registry_list.push_back(&backgroundImages);
		registry_list.push_back(&dialogueBoxes);


		registry_list.push_back(&slideBars);
		registry_list.push_back(&slideBlocks);
	}

	void clear_all_components() {
		for (ContainerInterface* reg : registry_list)
			reg->clear();
	}

	void list_all_components() {
		printf("Debug info on all registry entries:\n");
		for (ContainerInterface* reg : registry_list)
			if (reg->size() > 0)
				printf("%4d components of type %s\n", (int)reg->size(), typeid(*reg).name());
	}

	void list_all_components_of(Entity e) {
		printf("Debug info on components of entity %u:\n", (unsigned int)e);
		for (ContainerInterface* reg : registry_list)
			if (reg->has(e))
				printf("type %s\n", typeid(*reg).name());
	}

	void remove_all_components_of(Entity e) {
		for (ContainerInterface* reg : registry_list)
			reg->remove(e);
	}
};

extern ECSRegistry registry;