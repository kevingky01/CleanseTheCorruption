

#include "enemy_components.hpp"
#include <tinyECS/components.hpp>
#include <tinyECS/registry.hpp>
#include <spell_cast_manager.hpp>

enum BOSS_2_SPELLS { MEDIUM_FIREBALL, FIREBALL, DASH };

Boss_2::Boss_2(Entity& main_boss_entity)
{
    max_health       = 50.f;
    speed         = 150.f;
    detection_range  = 2000;
    shooting_range   = 1500;
    base_recharge_time = 1500;
    hitbox_size      = vec2(10, 20);

    col_mesh.local_points = {
        vec2(0, -10),
        vec2(+5, -5),
        vec2(+5, +5),
        vec2(0, +10),
        vec2(-5, +5),
        vec2(-5, -5)
    };

    time_since_last_state  = 0.f;
    time_since_last_attack = 0.f;
    radian_rotation   = 0.f;
    time_since_last_switch = 0.f;

    active_segment = BOSS2_SEGMENT_ACTIVE::SEGMENT_A;


    buildComponents(main_boss_entity);
    segmentA_entity = main_boss_entity;
    std::cout << "check 3" << std::endl;

    // Build segment B
    spawnSegmentB(main_boss_entity);
    segmentB_entity = segmentB_entity; 
    // std::cout << "check 4" << std::endl;
}


void Boss_2::buildComponents(Entity& boss_segment)
{
    std::cout << "check 4" << std::endl;

    
	// Enemy* enemy = registry.enemies.insert(boss_segment, this);
    std::cout << "check 5" << std::endl;

    // Enemy* enemy =  registry.enemies.get(boss_segment);
	// enemy->renderer = renderer;
	// enemy->type  =  (ENEMY_TYPE)type;

    

	// Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	// registry.meshPtrs.emplace(boss_segment, &mesh);
    
    auto& transform_comp = registry.transforms.emplace(boss_segment);
	// transform_comp.angle = 0.f;
	// transform_comp.position = position;

	registry.renderRequests.insert(
		boss_segment,
		{
			TEXTURE_ASSET_ID::RANGED_ENEMY_IDLE,
			EFFECT_ASSET_ID::ANIMATED,
			GEOMETRY_BUFFER_ID::ANIMATED_SPRITE
		}
	);
    std::cout << "check 6" << std::endl;


	auto& motion = registry.motions.emplace(boss_segment);
	motion.velocity = {0, 0};
    std::cout << "check 7" << std::endl;

    Health& health = registry.healths.emplace(boss_segment);
    health.maxHealth = max_health;
    health.currentHealth = max_health;

    Hitbox enemy_hitbox;
	enemy_hitbox.layer  = 0b10; 
	enemy_hitbox.mask   = 0b10;
	enemy_hitbox.hitbox_scale= getHitboxSize();
	registry.hitboxes.emplace(boss_segment, enemy_hitbox);
    std::cout << "check 8" << std::endl;




	// Collision mesh
	CollisionMesh colMesh = col_mesh;
	registry.collisionMeshes.insert(boss_segment, colMesh);

	// Animations
	AnimationManager& animation_manager = registry.animation_managers.emplace(boss_segment);
	Animation idle_animation(TEXTURE_ASSET_ID::RANGED_ENEMY_IDLE, 4, true);
	idle_animation.h_frames   = 4;
	idle_animation.v_frames   = 1;
	idle_animation.frame_rate = ANIMATION_FRAME_RATE / 2;

	Animation run_animation(TEXTURE_ASSET_ID::RANGED_ENEMY_RUN, 4, true);
	run_animation.h_frames   = 4;
	run_animation.v_frames   = 1;
	run_animation.frame_rate = ANIMATION_FRAME_RATE / 2;

	animation_manager.animations.insert({TEXTURE_ASSET_ID::RANGED_ENEMY_IDLE, idle_animation});
	animation_manager.animations.insert({TEXTURE_ASSET_ID::RANGED_ENEMY_RUN, run_animation});
	animation_manager.transition_to(idle_animation);

    addSpellsAndSpellSlotsAndParticleEmitters(boss_segment);
}

void Boss_2::addSpellsAndSpellSlotsAndParticleEmitters(Entity& self)
{
	// SpellSlotContainer& spellSlotContainer = registry.spellSlotContainers.get(self);
    SpellSlotContainer& spellSlotContainer = registry.spellSlotContainers.emplace(self);
    



	{
		SpellSlot damageSpellSlot;
		damageSpellSlot.spell_type = SPELL_TYPE::PROJECTILE;
		damageSpellSlot.spell_id   = (int)PROJECTILE_SPELL_ID::MEDIUM_SPEED_RED_ORB;
		damageSpellSlot.internalCooldown = 50;
		spellSlotContainer.spellSlots.push_back(damageSpellSlot);
	}

	{
		SpellSlot damageSpellSlot2;
		damageSpellSlot2.spell_type = SPELL_TYPE::PROJECTILE;
		damageSpellSlot2.spell_id   = (int)PROJECTILE_SPELL_ID::FIREBALL;
		damageSpellSlot2.internalCooldown = 50;
		spellSlotContainer.spellSlots.push_back(damageSpellSlot2);
	}

	// DASH
	{
		SpellSlot movementSpellSlot;
		movementSpellSlot.spell_type = SPELL_TYPE::MOVEMENT;
		movementSpellSlot.spell_id   = (int)MOVEMENT_SPELL_ID::DASH;
		movementSpellSlot.internalCooldown = 0;
		spellSlotContainer.spellSlots.push_back(movementSpellSlot);
	}

	ParticleEmitter dash_particle_emitter;
	dash_particle_emitter.setNumParticles(8);
	dash_particle_emitter.setParentEntity(self);
	dash_particle_emitter.setInitialColor(vec4(0.75, 0.75, 1, 0.25));
	dash_particle_emitter.setTextureAssetId(TEXTURE_ASSET_ID::PLAYER_DASH_PARTICLES);

	ParticleEmitterContainer& pec = registry.particle_emitter_containers.emplace(self);
	pec.particle_emitter_map.insert({PARTICLE_EMITTER_ID::DASH_TRAIL, dash_particle_emitter});
}


void Boss_2::spawnSegmentB(Entity& parent)
{
   vec2 player_spawn_pos = vec2(22, 38) * (float)TILE_SIZE;
   vec2 pos = player_spawn_pos + vec2(-15, -24) * (float)TILE_SIZE;

   auto entity = Entity();
   auto& transform_comp = registry.transforms.emplace(entity);
   transform_comp.angle    = 0.f;
   transform_comp.position = pos;

   Enemy* enemy = registry.enemies.insert(entity, this);
   enemy->renderer = renderer;
   enemy->type  = (ENEMY_TYPE)type;

   Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
   registry.meshPtrs.emplace(entity, &mesh);
   registry.renderRequests.insert(
       entity,
       {
           TEXTURE_ASSET_ID::RANGED_ENEMY_IDLE,
           EFFECT_ASSET_ID::ANIMATED,
           GEOMETRY_BUFFER_ID::ANIMATED_SPRITE
       }
   );

   auto& motion = registry.motions.emplace(entity);
   motion.velocity = vec2(0, 0);

   Health& health = registry.healths.emplace(entity);
   health.maxHealth     = max_health;
   health.currentHealth = max_health;

   Hitbox enemy_hitbox;
   enemy_hitbox.layer       = 0b10;
   enemy_hitbox.mask        = 0b10;
   enemy_hitbox.hitbox_scale= getHitboxSize();
   registry.hitboxes.emplace(entity, enemy_hitbox);

   SpellSlotContainer& spellSlotContainer = registry.spellSlotContainers.emplace(entity);
   SpellSlot damageSpellSlot;
   damageSpellSlot.spell_type = SPELL_TYPE::PROJECTILE;
   damageSpellSlot.spell_id   = (int)PROJECTILE_SPELL_ID::MEDIUM_SPEED_RED_ORB;
   damageSpellSlot.internalCooldown = 0;
   spellSlotContainer.spellSlots.push_back(damageSpellSlot);

   CollisionMesh cMesh = col_mesh;
   registry.collisionMeshes.insert(entity, cMesh);

   AnimationManager& animation_manager = registry.animation_managers.emplace(entity);

   Animation idle_animation(TEXTURE_ASSET_ID::RANGED_ENEMY_IDLE, 4, true);
   idle_animation.h_frames   = 4;
   idle_animation.v_frames   = 1;
   idle_animation.frame_rate = ANIMATION_FRAME_RATE / 2;

   Animation run_animation(TEXTURE_ASSET_ID::RANGED_ENEMY_RUN, 4, true);
   run_animation.h_frames   = 4;
   run_animation.v_frames   = 1;
   run_animation.frame_rate = ANIMATION_FRAME_RATE / 2;

   animation_manager.animations.insert({TEXTURE_ASSET_ID::RANGED_ENEMY_IDLE, idle_animation});
   animation_manager.animations.insert({TEXTURE_ASSET_ID::RANGED_ENEMY_RUN, run_animation});
   animation_manager.transition_to(idle_animation);


   segmentB_entity = entity;
}


void Boss_2::step(float elapsed_ms, Entity& )
{
    if (!registry.enemies.has(segmentA_entity) && !registry.enemies.has(segmentB_entity))
        return;

    // update timer
    time_since_last_state  += elapsed_ms;
    time_since_last_attack += elapsed_ms;
    time_since_last_switch += elapsed_ms;

    float stepSeconds = elapsed_ms / 1000.f;

    bool hasA = registry.enemies.has(segmentA_entity);
    bool hasB = registry.enemies.has(segmentB_entity);
    std::cout << "check a1" << std::endl;


    if (hasA && registry.motions.has(segmentA_entity))
    {
        std::cout << "check a2" << std::endl;

        Motion& mA = registry.motions.get(segmentA_entity);
        std::cout << "check a2.5" << std::endl;
        if (mA.is_dashing)
        std::cout << "check a3" << std::endl;
            handle_dashing_movement(mA, stepSeconds);
    }
    if (hasB && registry.motions.has(segmentB_entity))
    {
        std::cout << "check a4" << std::endl;

        Motion& mB = registry.motions.get(segmentB_entity);
        if (mB.is_dashing) {
            std::cout << "check a4.5" << std::endl;
            handle_dashing_movement(mB, stepSeconds);
        }
    }

    std::cout << "s1" << std::endl;
    if (state == BOSS_STATE::INACTIVE)
    {
        state = BOSS_STATE::CIRCLE_ATTACK;
        time_since_last_state = 0.f;
        time_since_last_attack= 0.f;
    }
    std::cout << "s1.2" << std::endl;


    if (state == BOSS_STATE::DASH_TOWARD_PLAYER && time_since_last_state >= 2000.f)
    {
        std::cout << "s2" << std::endl;

        state = BOSS_STATE::CIRCLE_ATTACK;
        time_since_last_state  = 0.f;
        time_since_last_attack = 0.f;
    }
    std::cout << "s1.3" << std::endl;


    if (state == BOSS_STATE::CIRCLE_ATTACK && time_since_last_state > 3000.f)
    {
        std::cout << "s3" << std::endl;

        state = BOSS_STATE::DASH_TOWARD_PLAYER;
        time_since_last_state  = 0.f;
        time_since_last_attack = 0.f;
    }

    std::cout << "s1.4" << std::endl;


    if (state == BOSS_STATE::DASH_TOWARD_PLAYER)
    {
        std::cout << "check b1" << std::endl;

        if (time_since_last_attack >= 1000.f) 
        {
            std::cout << "check b2" << std::endl;

            time_since_last_attack = 0.f;
            if (hasA && !hasB) {
                std::cout << "check b3" << std::endl;

                dashTowardPlayer(segmentA_entity);
            }
            else if (!hasA && hasB) {
                std::cout << "check b4" << std::endl;

                dashTowardPlayer(segmentB_entity);
            }
            else if (hasA && hasB) {
                std::cout << "check b5" << std::endl;

                dashTowardPlayer(segmentA_entity);
                dashTowardPlayer(segmentB_entity);
            }
        }
    }
        std::cout << "s1.3" << std::endl;

     if (state == BOSS_STATE::CIRCLE_ATTACK)
    {
        std::cout << "circle " << std::endl;

        if (time_since_last_attack >= 2000.f)
        {
            std::cout << "circl1 " << std::endl;

            time_since_last_attack = 0.f;
            if (hasA) circleAttack(segmentA_entity);
            std::cout << "circle2" << std::endl;

            if (hasB) circleAttack(segmentB_entity);
            std::cout << "circle3" << std::endl;

            radian_rotation += M_PI / 24.f; 
        }
    }


    if (hasA && !hasB)
    {
        active_segment = BOSS2_SEGMENT_ACTIVE::SEGMENT_A;
        auto& rrA = registry.renderRequests.get(segmentA_entity);
        rrA.colour_edge = true;     
        rrA.is_hitflash = false;

        if (registry.hitboxes.has(segmentA_entity)) {
            registry.hitboxes.get(segmentA_entity).layer = 0b10;
            
        }

        // M4 NEW
        time_since_last_attack += 8*elapsed_ms;
        // circleAttack(segmentA_entity);

        // End of M4 NEW

        // Add the next door lootable
        if (!registry.lootables.has(segmentA_entity)) {
            Interactable next_level_door;
            next_level_door.interactable_id = INTERACTABLE_ID::NEXT_LEVEL_ENTRY;
            next_level_door.game_screen_id = (int)GAME_SCREEN_ID::IN_BETWEEN;

            Lootable& lootable = registry.lootables.emplace(segmentA_entity);
            lootable.drop = next_level_door;
        }

        
    }
    // If only segmentB is alive:
    else if (!hasA && hasB)
    {
        active_segment = BOSS2_SEGMENT_ACTIVE::SEGMENT_B;
        auto& rrB = registry.renderRequests.get(segmentB_entity);
        rrB.colour_edge = true;      // highlight B
        rrB.is_hitflash = false;



        // M4 NEW

        time_since_last_attack += 8*elapsed_ms;
        // circleAttack(segmentB_entity);
        // End of M4 NEW

        if (registry.hitboxes.has(segmentB_entity)) {
            registry.hitboxes.get(segmentB_entity).layer = 0b10;
        }

        // Add the next door lootable
        if (!registry.lootables.has(segmentB_entity)) {
            Interactable next_level_door;
            next_level_door.interactable_id = INTERACTABLE_ID::NEXT_LEVEL_ENTRY;
            next_level_door.game_screen_id = (int)GAME_SCREEN_ID::IN_BETWEEN;

            Lootable& lootable = registry.lootables.emplace(segmentB_entity);
            lootable.drop = next_level_door;
        }
    }
    // If both are alive, toggle which is targetable every 5s
    else if (hasA && hasB)
    {
     if (time_since_last_switch > 5000.f)
        {
            time_since_last_switch = 0.f;
            // remain in whichever state we are, but toggle active
            if (active_segment == BOSS2_SEGMENT_ACTIVE::SEGMENT_A)
            {
                active_segment = BOSS2_SEGMENT_ACTIVE::SEGMENT_B;

            }
            else
            {
                active_segment = BOSS2_SEGMENT_ACTIVE::SEGMENT_A;
            }
        }

        // Now set the collision layers and color
        auto& rrA = registry.renderRequests.get(segmentA_entity);
        auto& rrB = registry.renderRequests.get(segmentB_entity);
        // rrA.colour_edge = false; 
        // rrB.colour_edge = false; 
        rrA.is_hitflash = false;
        rrB.is_hitflash = false;

        if (registry.hitboxes.has(segmentA_entity) && registry.hitboxes.has(segmentB_entity))
        {
            auto& hbA = registry.hitboxes.get(segmentA_entity);
            auto& hbB = registry.hitboxes.get(segmentB_entity);

            if (active_segment == BOSS2_SEGMENT_ACTIVE::SEGMENT_A)
            {
                // highlight the non-damagable enemy
                hbA.layer = 0b10; 
                hbA.mask = 0b00;
                hbB.layer = 0b00;
                hbB.mask = (int)COLLISION_MASK::WALL;
                rrB.is_hitflash = true;    
            }
            else
            {
                hbB.layer = 0b10;
                hbB.mask = 0b00;
                hbA.layer = 0b00;
                hbA.mask = (int)COLLISION_MASK::WALL;
                rrA.is_hitflash = true;
            }
        }
    }
}


void Boss_2::circleAttack(Entity& self)
{
    std::cout << "check ca1" << std::endl;

	Transformation& transform = registry.transforms.get(self);
    std::cout << "check ca2" << std::endl;

	SpellSlotContainer& ssc = registry.spellSlotContainers.get(self);
    std::cout << "check ca3" << std::endl;

	if (ssc.spellSlots.empty()) return; 
    std::cout << "check ca4" << std::endl;

	SpellSlot& spell_slot = ssc.spellSlots[0];
	int radius = 100;
    std::cout << "check ca5" << std::endl;


	for (int i = 0; i < 12; i++) {
        std::cout << "check ca6" << std::endl;

		double angleCW = 2 * M_PI * i / 12 + radian_rotation;
		vec2 direction = { radius * sin(angleCW), radius * cos(angleCW) };
        std::cout << "check ca8" << std::endl;

		SpellCastManager::castBossSpell(renderer, spell_slot, transform.position, direction, self);
        std::cout << "check ca9" << std::endl;

	}
}


void Boss_2::handle_dashing_movement(Motion& playerMotion, float stepSeconds)
{
    const float PLAYER_ACCELERATION = 300.f;
    std::cout << "check a5" << std::endl;


	if (glm::length(playerMotion.velocity) < PLAYER_ACCELERATION * stepSeconds) {
		playerMotion.velocity   = vec2(0, 0);
        playerMotion.is_dashing= false;
	}
	else {
		playerMotion.velocity -= glm::normalize(playerMotion.velocity) 
                                 * PLAYER_ACCELERATION 
                                 * stepSeconds;
	}
    std::cout << "check after dashing\n";
}


void Boss_2::dashTowardPlayer(Entity& self)
{
    if (!registry.motions.has(self)) return;


    Motion& motion = registry.motions.get(self);
    if (!registry.transforms.has(self)) return; 
    Transformation& transform = registry.transforms.get(self);

    if (registry.players.entities.empty()) return; 
    auto player  = registry.players.entities[0];
    if (!registry.transforms.has(player)) return;
    std::cout << "check c1" << std::endl;


    Transformation& player_transform = registry.transforms.get(player);
    vec2 angle = player_transform.position - transform.position;
    float dashSpeed = 700.f;

    motion.is_dashing = true;
    motion.velocity   = glm::normalize(angle) * dashSpeed;

    // Also do the "dash" spell if it triggers effects
    SpellSlotContainer& ssc = registry.spellSlotContainers.get(self);
    if (ssc.spellSlots.size() > (int)BOSS_2_SPELLS::DASH)
    {
        SpellSlot& dashSlot = ssc.spellSlots[(int)BOSS_2_SPELLS::DASH];
        SpellCastManager::castSpell(renderer, dashSlot, transform.position, angle, self);
        dashSlot.remainingCooldown = 0;
    }
    std::cout << "Dash initiated!\n";

}