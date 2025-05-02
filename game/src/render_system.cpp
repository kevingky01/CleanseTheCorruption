#include <SDL.h>
#include <glm/trigonometric.hpp>
#include <iostream>
#include <algorithm>

// internal
#include "render_system.hpp"
#include "tinyECS/registry.hpp"
#include "spells.hpp"
#include <glm/gtc/type_ptr.hpp>
#include "relics.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H


/* This function has several prerequisites:
* 1. This only works if every entity in componentContainer has an associated Transformation Component
* 2. The shader referenced in shader_id must have the following fields:
*   a. in_position (vec3)
*   b. in_texcoord (vec2)
*   c. instance_matrix (mat3)
*/
void RenderSystem::drawTiles(std::vector<Entity> entities, TEXTURE_ASSET_ID texture_asset_id, const mat3& projection) {

	int entityCount = entities.size();

	TileInfo* tile_info = new TileInfo[entityCount];

	ivec2& texture_dimension = texture_dimensions[(GLuint)texture_asset_id];
	// TODO: I think we can extract this out of the loop / out of this function
	for (int i = 0; i < entityCount; i++) {

		Entity& current_entity = entities[i];
		
		if (!registry.transforms.has(current_entity)) continue;

		Transformation& current_transform = registry.transforms.get(current_entity);

		vec2 trueScale = vec2(current_transform.scale.x * texture_dimension.x * PIXEL_SCALE_FACTOR,
			current_transform.scale.y * texture_dimension.y * PIXEL_SCALE_FACTOR);

		Transform e_transform;
		e_transform.translate(current_transform.position);
		e_transform.scale(trueScale);
		e_transform.rotate(radians(current_transform.angle));

		tile_info[i].transform_matrix = e_transform.mat;


		if (registry.walls.has(current_entity)) {
			Tile tile = registry.tiles.get(current_entity);
			tile_info[i].tilecoord = tile.tilecoord;
		}
		else if (registry.floors.has(current_entity)) {
			Tile tile = registry.tiles.get(current_entity);
			tile_info[i].tilecoord = tile.tilecoord;
		}
	}

	const GLuint program = (GLuint)effects[(GLuint)EFFECT_ASSET_ID::ENVIRONMENT];

	// Setting shaders
	glUseProgram(program);
	gl_has_errors();

  


	const GLuint vbo = vertex_buffers[(GLuint)GEOMETRY_BUFFER_ID::BACKGROUND];
	const GLuint ibo = index_buffers[(GLuint)GEOMETRY_BUFFER_ID::BACKGROUND];

	// Setting vertex and index buffers
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	gl_has_errors();

	// texture-mapped entities - use data location as in the vertex buffer
	GLint in_position_loc = 0;
	//!!!!!
	glBindAttribLocation(program, in_position_loc, "in_position");

	GLint in_texcoord_loc = 1;
	glBindAttribLocation(program, in_texcoord_loc, "in_texcoord");
	gl_has_errors();
	assert(in_texcoord_loc >= 0);

	glEnableVertexAttribArray(in_position_loc);
	glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE,
		sizeof(TexturedVertex), (void*)0);
	gl_has_errors();

	glEnableVertexAttribArray(in_texcoord_loc);
	glVertexAttribPointer(
		in_texcoord_loc, 2, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex),
		(void*)sizeof(
			vec3)); // note the stride to skip the preceeding vertex position
	// Q: How does stride work ? I'd figure the offset is enough, why do we need to do this?
	// A: The vertex buffer is an Array of TexturedVertex, and TexturedVertex is made of 2 parts, position and texcoords. 
	//	  The offset is so we access texcoords properly, and then the stride makes it so we "jump" to the next texcoord in the array, and not the position or something

	// NEW CODE:
	// references:
	// https://stackoverflow.com/questions/17355051/using-a-matrix-as-vertex-attribute-in-opengl3-core-profile
	// https://learnopengl.com/Advanced-OpenGL/Instancing

	// bind new vbo and attach to it transformation data-- this is where we load the position data in
	const GLuint instanceVBO = instance_buffers[(int)GEOMETRY_BUFFER_ID::BACKGROUND]; // always the first instance_buffer


	glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(TileInfo) * entityCount, &tile_info[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vbo); //rebind original vbo before moving forward	

	// this is normal stuff-- binding the instance_matrix_loc to attribute location 2, as specified in vertex shader
	GLint instance_matrix_loc = 2;
	glBindAttribLocation(program, instance_matrix_loc, "instance_matrix");

	int pos0 = instance_matrix_loc + 0;
	int pos1 = instance_matrix_loc + 1;
	int pos2 = instance_matrix_loc + 2;
	glEnableVertexAttribArray(pos0);
	glEnableVertexAttribArray(pos1);
	glEnableVertexAttribArray(pos2);

	glBindBuffer(GL_ARRAY_BUFFER, instanceVBO); // rebind 

	// binding attributes-- a mat3 is passed in as 3 vec3s cause shaders only directly accept up to vec4 size
	int vec3Size = sizeof(vec3);
	glVertexAttribPointer(pos0, 3, GL_FLOAT, GL_FALSE, sizeof(TileInfo), (void*)0);
	glVertexAttribPointer(pos1, 3, GL_FLOAT, GL_FALSE, sizeof(TileInfo), (void*)(1 * sizeof(vec3)));
	glVertexAttribPointer(pos2, 3, GL_FLOAT, GL_FALSE, sizeof(TileInfo), (void*)(2 * sizeof(vec3)));

	// sets these to instance variables-- the 1 indicates the repetition frequency (0 for non-instance)
	glVertexAttribDivisor(pos0, 1);
	glVertexAttribDivisor(pos1, 1);
	glVertexAttribDivisor(pos2, 1);

	

	GLint tilecoord_loc = pos2 + 1;
	glBindAttribLocation(program, tilecoord_loc, "in_tilecoord");

	glEnableVertexAttribArray(tilecoord_loc);

	glVertexAttribPointer(tilecoord_loc, 2, GL_FLOAT, GL_FALSE, sizeof(TileInfo), (void*)(3 * sizeof(vec3)));

	glVertexAttribDivisor(tilecoord_loc, 1);


	glBindBuffer(GL_ARRAY_BUFFER, vbo); // Finished the new code section-- set vbo back to normal


	// Set uniforms
	if (entities.size() > 0) {
		if (registry.walls.has(entities[0])) {
			GLint h_frame_uloc = glGetUniformLocation(program, "h_tiles");
			glUniform1f(h_frame_uloc, (float)NUM_WALL_TILES_H);
			gl_has_errors();

			// Getting uniform locations for glUniform* calls
			GLint v_frame_uloc = glGetUniformLocation(program, "v_tiles");
			glUniform1f(v_frame_uloc, (float)NUM_WALL_TILES_V);
			gl_has_errors();
		}
		else if (registry.floors.has(entities[0])) {
			GLint h_frame_uloc = glGetUniformLocation(program, "h_tiles");
			glUniform1f(h_frame_uloc, (float)NUM_FLOOR_TILES_H);
			gl_has_errors();

			// Getting uniform locations for glUniform* calls
			GLint v_frame_uloc = glGetUniformLocation(program, "v_tiles");
			glUniform1f(v_frame_uloc, (float)NUM_FLOOR_TILES_V);
			gl_has_errors();
		}
	}
	

	




	// Enabling and binding texture to slot 0
	glActiveTexture(GL_TEXTURE0);
	gl_has_errors();

	GLuint texture_id =
		texture_gl_handles[(GLuint)texture_asset_id];

	glBindTexture(GL_TEXTURE_2D, texture_id);
	gl_has_errors();

	// Get number of indices from index buffer, which has elements uint16_t
	GLint size = 0;
	glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
	gl_has_errors();

	GLsizei num_indices = size / sizeof(uint16_t);
	// GLsizei num_triangles = num_indices / 3;

	GLint currProgram;
	glGetIntegerv(GL_CURRENT_PROGRAM, &currProgram);

	GLuint projection_loc = glGetUniformLocation(currProgram, "projection");
	glUniformMatrix3fv(projection_loc, 1, GL_FALSE, (float*)&projection);
	gl_has_errors();

	// Munn: setting texture filter to NEAREST, instead of the defeault LINEAR (for pixel art)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // when scaling down
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); // when scaling up

	// Drawing of num_indices/3 triangles specified in the index buffer
	glDrawElementsInstanced(GL_TRIANGLES, num_indices, GL_UNSIGNED_SHORT, nullptr, entityCount);
	gl_has_errors();

	// This is really important-- unassigns buffer and therefore stops memory leak
	//glDeleteBuffers(1, &instanceVBO); // TODO: small optimization by reusing a single instanceVBO over multiple draw calls
	delete[]tile_info;

	// Munn: without reseting these, the particles were no longer showing up for me
	glVertexAttribDivisor(pos0, 0);
	glVertexAttribDivisor(pos1, 0);
	glVertexAttribDivisor(pos2, 0);
	glVertexAttribDivisor(tilecoord_loc, 0);
}



void RenderSystem::drawEnvironment(const mat3& projection) {

	// TODO: do the frustrum culling here based on rooms, accessing a higher-scoped
	// vectors (that I've yet to create) called "floorsToRender" and "wallsToRender"

	// these vectors should eventually hold pointers to entities that we want to render. 

	// Create vectors of unculled floors/walls
	std::vector<Entity> floors = registry.floors.entities;
	std::vector<Entity> walls = registry.walls.entities;

	// Draw floor
	drawTiles(floors, TEXTURE_ASSET_ID::FLOOR, projection);

	// Draw "doors"
	for (Entity entity : registry.enemyRoomManagers.entities) {
		EnemyRoomManager room_manager = registry.enemyRoomManagers.get(entity);

		for (Entity wall_entity : room_manager.wall_entities) {
			drawTexturedMesh(wall_entity, projection);
		}
	}

	for (Entity entity : registry.goalManagers.entities) {
		GoalManager goal_manager = registry.goalManagers.get(entity);

		for (Entity wall_entity : goal_manager.wall_entities) {
			drawTexturedMesh(wall_entity, projection);
		}
	}

	// Draw walls
	drawTiles(walls, TEXTURE_ASSET_ID::WALL, projection);
}




void RenderSystem::drawTexturedMesh(Entity entity,
	const mat3& projection)
{
	// Get transform
	if (!registry.transforms.has(entity)) {
		return;
	}
	Transformation& entity_transform = registry.transforms.get(entity);

	// Get render request
	if (!registry.renderRequests.has(entity)) {
		return;
	}
	const RenderRequest& render_request = registry.renderRequests.get(entity);

	// Get texture dimension
	ivec2& texture_dimension = texture_dimensions[(GLuint)render_request.used_texture];

	Transform transform;
	transform.translate(entity_transform.position);

	vec2 trueScale = vec2(entity_transform.scale.x * texture_dimension.x * PIXEL_SCALE_FACTOR,
		entity_transform.scale.y * texture_dimension.y * PIXEL_SCALE_FACTOR);
	transform.scale(trueScale);

	transform.rotate(radians(entity_transform.angle));

	const GLuint used_effect_enum = (GLuint)render_request.used_effect;
	assert(used_effect_enum != (GLuint)EFFECT_ASSET_ID::EFFECT_COUNT);
	const GLuint program = (GLuint)effects[used_effect_enum];

	// Setting shaders
	glUseProgram(program);
	gl_has_errors();

	assert(render_request.used_geometry != GEOMETRY_BUFFER_ID::GEOMETRY_COUNT);
	const GLuint vbo = vertex_buffers[(GLuint)render_request.used_geometry];
	const GLuint ibo = index_buffers[(GLuint)render_request.used_geometry];

	// Setting vertex and index buffers
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	gl_has_errors();

	GLint in_position_loc = glGetAttribLocation(program, "in_position");
	GLint in_texcoord_loc = glGetAttribLocation(program, "in_texcoord");
	gl_has_errors();
	assert(in_texcoord_loc >= 0);

	glEnableVertexAttribArray(in_position_loc);
	glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE,
		sizeof(TexturedVertex), (void*)0);
	gl_has_errors();

	glEnableVertexAttribArray(in_texcoord_loc);
	glVertexAttribPointer(
		in_texcoord_loc, 2, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex),
		(void*)sizeof(
			vec3)); // note the stride to skip the preceeding vertex position

	// Enabling and binding texture to slot 0
	glActiveTexture(GL_TEXTURE0);
	gl_has_errors();

	assert(registry.renderRequests.has(entity));
	GLuint texture_id =
		texture_gl_handles[(GLuint)registry.renderRequests.get(entity).used_texture];

	glBindTexture(GL_TEXTURE_2D, texture_id);
	gl_has_errors();

	// texture-mapped entities - use data location as in the vertex buffer
	if (render_request.used_effect == EFFECT_ASSET_ID::TEXTURED)
	{
		// Do nothing special lol
	}
	else if (render_request.used_effect == EFFECT_ASSET_ID::ANIMATED) {
		// Get animation
		AnimationManager& animation_manager = registry.animation_managers.get(entity);
		Animation& animation = animation_manager.current_animation;

		// Get current frame
		int current_frame = ((int)animation.current_time % animation.num_frames);

		// Getting uniform locations for glUniform* calls
		GLint num_frames_uloc = glGetUniformLocation(program, "current_frame");
		glUniform1f(num_frames_uloc, (float)current_frame);
		gl_has_errors();

		// Getting uniform locations for glUniform* calls
		GLint h_frame_uloc = glGetUniformLocation(program, "h_frames");
		glUniform1f(h_frame_uloc, (float)animation.h_frames);
		gl_has_errors();

		// Getting uniform locations for glUniform* calls
		GLint v_frame_uloc = glGetUniformLocation(program, "v_frames");
		glUniform1f(v_frame_uloc, (float)animation.v_frames);
		gl_has_errors();
	}
	else if (render_request.used_effect == EFFECT_ASSET_ID::OUTLINE) {
		// Pass in texture size
		GLint texture_size_uloc = glGetUniformLocation(program, "texture_size");

		vec2 texture_size = texture_dimensions[(int)render_request.used_texture];
		glUniform2fv(texture_size_uloc, 1, (float*)&texture_size);
		gl_has_errors();


		if (registry.interactables.has(entity)) {
			bool is_outline_active = registry.interactables.get(entity).can_interact;
			GLint is_active_location = glGetUniformLocation(program, "is_outline_active");

			glUniform1i(is_active_location, is_outline_active);
			gl_has_errors();
		}
	}
	else if (render_request.used_effect == EFFECT_ASSET_ID::ANIMATED_OUTLINE) {
		// Pass in texture size
		GLint texture_size_uloc = glGetUniformLocation(program, "texture_size");

		vec2 texture_size = texture_dimensions[(int)render_request.used_texture];
		glUniform2fv(texture_size_uloc, 1, (float*)&texture_size);
		gl_has_errors();


		if (registry.interactables.has(entity)) {
			bool is_outline_active = registry.interactables.get(entity).can_interact;
			GLint is_active_location = glGetUniformLocation(program, "is_outline_active");

			glUniform1i(is_active_location, is_outline_active);
			gl_has_errors();
		}

		// Get animation
		AnimationManager& animation_manager = registry.animation_managers.get(entity);
		Animation& animation = animation_manager.current_animation;

		// Get current frame
		int current_frame = ((int)animation.current_time % animation.num_frames);

		// Getting uniform locations for glUniform* calls
		GLint num_frames_uloc = glGetUniformLocation(program, "current_frame");
		glUniform1f(num_frames_uloc, (float)current_frame);
		gl_has_errors();

		// Getting uniform locations for glUniform* calls
		GLint h_frame_uloc = glGetUniformLocation(program, "h_frames");
		glUniform1f(h_frame_uloc, (float)animation.h_frames);
		gl_has_errors();

		// Getting uniform locations for glUniform* calls
		GLint v_frame_uloc = glGetUniformLocation(program, "v_frames");
		glUniform1f(v_frame_uloc, (float)animation.v_frames);
		gl_has_errors();
	}
	else if (render_request.used_effect == EFFECT_ASSET_ID::TILE) {

		vec2 tilecoord = vec2();
		int h_tiles = 0;
		int v_tiles = 0;
		if (registry.tiles.has(entity)) {
			Tile tile = registry.tiles.get(entity);

			tilecoord = tile.tilecoord;
			h_tiles = tile.h_tiles;
			v_tiles = tile.v_tiles;
		}
		
		/*std::cout << "Tile coord: " << tilecoord.x << ", " << tilecoord.y << std::endl;
		std::cout << "h_tiles: " << h_tiles << std::endl;
		std::cout << "v_tiles: " << v_tiles << std::endl;*/
		// Getting uniform locations for glUniform* calls
		GLint h_tiles_uloc = glGetUniformLocation(program, "h_tiles");
		glUniform1f(h_tiles_uloc, (float)h_tiles);
		gl_has_errors();

		// Getting uniform locations for glUniform* calls
		GLint v_frame_uloc = glGetUniformLocation(program, "v_tiles");
		glUniform1f(v_frame_uloc, (float)v_tiles);
		gl_has_errors();

		GLint tilecoord_uloc = glGetUniformLocation(program, "tilecoord");
		glUniform2fv(tilecoord_uloc, 1, (float*)&tilecoord);
		gl_has_errors();
	}
	else if (render_request.used_effect == EFFECT_ASSET_ID::MINIMAP) {
		// Pass num tiles x and y, and also which tiles are revealed

		Minimap minimap = registry.minimaps.components[0];

		std::vector<vec2> revealed_walls = minimap.wall_positions;
		
		// HARD CODED MAP SIZES
		GLint x_tiles_uloc = glGetUniformLocation(program, "num_x_tiles");
		GLint y_tiles_uloc = glGetUniformLocation(program, "num_y_tiles");
		glUniform1f(x_tiles_uloc, (float)100);
		glUniform1f(y_tiles_uloc, (float)100);
		gl_has_errors();

		// Pass in revealed tiles
		for (int i = 0; i < revealed_walls.size(); i++) {
			std::string var_name = "revealed_walls[" + std::to_string(i) + "]";
			GLint revealed_walls_uloc = glGetUniformLocation(program, var_name.c_str());
			//std::cout << revealed_walls_uloc << std::endl;
			glUniform2f(revealed_walls_uloc, revealed_walls[i].x, revealed_walls[i].y);
		}

		GLint num_walls_uloc = glGetUniformLocation(program, "num_revealed_walls");
		glUniform1f(num_walls_uloc, (float)revealed_walls.size());
		gl_has_errors();


	}
	else {
		assert(false && "Type of render request not supported");
	}

	// Getting uniform locations for glUniform* calls
	GLint color_uloc = glGetUniformLocation(program, "fcolor");
	const vec3 color = registry.colors.has(entity) ? registry.colors.get(entity) : vec3(1);
	glUniform3fv(color_uloc, 1, (float*)&color);
	gl_has_errors();


	GLint is_hitflash_uloc = glGetUniformLocation(program, "is_hitflash");
	if (is_hitflash_uloc > -1) {
		const bool is_hitflash = render_request.is_hitflash;
		glUniform1i(is_hitflash_uloc, is_hitflash);
	}

// new
	GLint colour_edge_uloc = glGetUniformLocation(program, "colour_edge");
	if (colour_edge_uloc > -1) {
		const bool colour_edge = render_request.colour_edge;
		glUniform1i(colour_edge_uloc, colour_edge);
	}
	
	// Get number of indices from index buffer, which has elements uint16_t
	GLint size = 0;
	glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
	gl_has_errors();

	GLsizei num_indices = size / sizeof(uint16_t);
	// GLsizei num_triangles = num_indices / 3;

	GLint currProgram;
	glGetIntegerv(GL_CURRENT_PROGRAM, &currProgram);
	// Setting uniform values to the currently bound program
	GLuint transform_loc = glGetUniformLocation(currProgram, "transform");
	glUniformMatrix3fv(transform_loc, 1, GL_FALSE, (float*)&transform.mat);
	gl_has_errors();

	GLuint projection_loc = glGetUniformLocation(currProgram, "projection");
	glUniformMatrix3fv(projection_loc, 1, GL_FALSE, (float*)&projection);
	gl_has_errors();

	// Munn: setting texture filter to NEAREST, instead of the defeault LINEAR (for pixel art)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // when scaling down
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); // when scaling up

	// Drawing of num_indices/3 triangles specified in the index buffer
	glDrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_SHORT, nullptr);
	gl_has_errors();

	if (registry.interactables.has(entity)) {
		Interactable interactable = registry.interactables.get(entity);

		if (interactable.interactable_id == INTERACTABLE_ID::PROJECTILE_SPELL_DROP
			|| interactable.interactable_id == INTERACTABLE_ID::MOVEMENT_SPELL_DROP
			|| interactable.interactable_id == INTERACTABLE_ID::RELIC_DROP) {
			drawIconOnInteractable(entity, projection);
		}
	}
}

void RenderSystem::drawShadow(Entity entity, const mat3& projection) {
	GLuint program = effects[(GLuint)EFFECT_ASSET_ID::TEXTURED];
	glUseProgram(program);
	gl_has_errors();

	// bind the geometry buffer (quad) for our health bars
	GLuint vbo = vertex_buffers[(int)GEOMETRY_BUFFER_ID::SPRITE];
	GLuint ibo = index_buffers[(int)GEOMETRY_BUFFER_ID::SPRITE];
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);


	GLint in_position_loc = glGetAttribLocation(program, "in_position");
	glEnableVertexAttribArray(in_position_loc);
	glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE,
		sizeof(TexturedVertex), (void*)0);

	GLint in_texcoord_loc = glGetAttribLocation(program, "in_texcoord");
	glEnableVertexAttribArray(in_texcoord_loc);
	glVertexAttribPointer(
		in_texcoord_loc, 2, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex),
		(void*)sizeof(vec3));

	if (!registry.renderRequests.has(entity)) {
		return;
	}

	int asset_id = (int)registry.renderRequests.get(entity).used_texture;
	int shadow_id = (int)TEXTURE_ASSET_ID::SHADOW;

	Transformation entity_transform = registry.transforms.get(entity);

	// Get texture dimension
	ivec2& texture_dimension = texture_dimensions[(GLuint)asset_id];
	ivec2& shadow_dimension = texture_dimensions[(GLuint)shadow_id];

	Transform transform;
	transform.translate(entity_transform.position); // Translate to entity

	float scale_divisor = 1;
	if (registry.animation_managers.has(entity)) {
		AnimationManager animation_manager = registry.animation_managers.get(entity);
		Animation animation = animation_manager.current_animation;

		scale_divisor = animation.v_frames;
	}

	// Translate down to entity's "feet"
	float y_offset = (entity_transform.scale.y * texture_dimension.y * PIXEL_SCALE_FACTOR) / 2.0f / scale_divisor - (1 * PIXEL_SCALE_FACTOR);
	transform.translate(vec2(0, y_offset));

	vec2 trueScale = vec2(entity_transform.scale.x * shadow_dimension.x * PIXEL_SCALE_FACTOR,
		entity_transform.scale.y * shadow_dimension.y * PIXEL_SCALE_FACTOR);
	transform.scale(trueScale);

	// Getting uniform locations for glUniform* calls
	GLint color_uloc = glGetUniformLocation(program, "fcolor");
	vec3 color = vec3(1);
	glUniform3fv(color_uloc, 1, (float*)&color);
	gl_has_errors();

	GLint is_hitflash_uloc = glGetUniformLocation(program, "is_hitflash");
	if (is_hitflash_uloc > -1) {
		glUniform1i(is_hitflash_uloc, false);
	}

	// Setting uniform values to the currently bound program
	GLuint transform_loc = glGetUniformLocation(program, "transform");
	glUniformMatrix3fv(transform_loc, 1, GL_FALSE, (float*)&transform.mat);
	gl_has_errors();

	GLuint projection_loc = glGetUniformLocation(program, "projection");
	glUniformMatrix3fv(projection_loc, 1, GL_FALSE, (float*)&projection);
	gl_has_errors();


	// Enabling and binding texture to slot 0
	glActiveTexture(GL_TEXTURE0);
	GLuint texture_id = texture_gl_handles[shadow_id];
	glBindTexture(GL_TEXTURE_2D, texture_id);
	gl_has_errors();

	// Get number of indices from index buffer, which has elements uint16_t
	GLint size = 0;
	glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
	gl_has_errors();

	GLsizei num_indices = size / sizeof(uint16_t);

	// Munn: setting texture filter to NEAREST, instead of the defeault LINEAR (for pixel art)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // when scaling down
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); // when scaling up

	// Drawing of num_indices/3 triangles specified in the index buffer
	glDrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_SHORT, nullptr);
	gl_has_errors();
}

void RenderSystem::drawIconOnInteractable(Entity entity, const mat3& projection) {
	GLuint program = effects[(GLuint)EFFECT_ASSET_ID::TEXTURED];
	glUseProgram(program);
	gl_has_errors();

	// bind the geometry buffer (quad) for our health bars
	GLuint vbo = vertex_buffers[(int)GEOMETRY_BUFFER_ID::SPRITE];
	GLuint ibo = index_buffers[(int)GEOMETRY_BUFFER_ID::SPRITE];
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);


	GLint in_position_loc = glGetAttribLocation(program, "in_position");
	glEnableVertexAttribArray(in_position_loc);
	glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE,
		sizeof(TexturedVertex), (void*)0);

	GLint in_texcoord_loc = glGetAttribLocation(program, "in_texcoord");
	glEnableVertexAttribArray(in_texcoord_loc);
	glVertexAttribPointer(
		in_texcoord_loc, 2, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex),
		(void*)sizeof(vec3));

	// Munn: This function assumes the entity you pass in is a spell drop interactable
	Interactable interactable = registry.interactables.get(entity);

	int asset_id = 0;
	if (interactable.interactable_id == INTERACTABLE_ID::PROJECTILE_SPELL_DROP) {
		ProjectileSpell* spell = projectile_spells[(int)interactable.spell_id];
		asset_id = (int)spell->getAssetID();
	}
	if (interactable.interactable_id == INTERACTABLE_ID::MOVEMENT_SPELL_DROP) {
		MovementSpell* spell = movement_spells[(int)interactable.spell_id];
		asset_id = (int)spell->getAssetID();
	}
	if (interactable.interactable_id == INTERACTABLE_ID::RELIC_DROP) {
		Relic* relic = relics[(int)interactable.relic_id];
		asset_id = (int)relic->getIconAsset();
	}

	Transformation entity_transform = registry.transforms.get(entity);

	// Get texture dimension
	ivec2& texture_dimension = texture_dimensions[(GLuint)asset_id];

	Transform transform;
	transform.translate(entity_transform.position);

	vec2 trueScale = vec2(entity_transform.scale.x * texture_dimension.x * PIXEL_SCALE_FACTOR,
		entity_transform.scale.y * texture_dimension.y * PIXEL_SCALE_FACTOR);
	transform.scale(trueScale);

	transform.rotate(radians(entity_transform.angle));

	// Getting uniform locations for glUniform* calls
	GLint color_uloc = glGetUniformLocation(program, "fcolor");
	vec3 color = vec3(1);
	glUniform3fv(color_uloc, 1, (float*)&color);
	gl_has_errors();

	// Setting uniform values to the currently bound program
	GLuint transform_loc = glGetUniformLocation(program, "transform");
	glUniformMatrix3fv(transform_loc, 1, GL_FALSE, (float*)&transform.mat);
	gl_has_errors();

	GLuint projection_loc = glGetUniformLocation(program, "projection");
	glUniformMatrix3fv(projection_loc, 1, GL_FALSE, (float*)&projection);
	gl_has_errors();


	// Enabling and binding texture to slot 0
	glActiveTexture(GL_TEXTURE0);
	GLuint texture_id = texture_gl_handles[(GLuint)asset_id];
	glBindTexture(GL_TEXTURE_2D, texture_id);
	gl_has_errors();

	// Get number of indices from index buffer, which has elements uint16_t
	GLint size = 0;
	glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
	gl_has_errors();

	GLsizei num_indices = size / sizeof(uint16_t);

	// Munn: setting texture filter to NEAREST, instead of the defeault LINEAR (for pixel art)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // when scaling down
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); // when scaling up

	// Drawing of num_indices/3 triangles specified in the index buffer
	glDrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_SHORT, nullptr);
	gl_has_errors();
}




// first draw to an intermediate texture,
// apply the "vignette" texture, when requested
// then draw the intermediate texture
void RenderSystem::drawToScreen()
{
	// Setting shaders
	// get the vignette texture, sprite mesh, and program
	glUseProgram(effects[(GLuint)EFFECT_ASSET_ID::VIGNETTE]);
	gl_has_errors();

	// Clearing backbuffer
	int w, h;
	glfwGetFramebufferSize(window, &w, &h); // Note, this will be 2x the resolution given to glfwCreateWindow on retina displays
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, w, h);
	glDepthRange(0, 10);
	glClearColor(0, 0, 0, 1.0);
	glClearDepth(1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	gl_has_errors();
	// Enabling alpha channel for textures
	glDisable(GL_BLEND);
	// glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);

	// Draw the screen texture on the quad geometry
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffers[(GLuint)GEOMETRY_BUFFER_ID::SCREEN_TRIANGLE]);
	glBindBuffer(
		GL_ELEMENT_ARRAY_BUFFER,
		index_buffers[(GLuint)GEOMETRY_BUFFER_ID::SCREEN_TRIANGLE]); // Note, GL_ELEMENT_ARRAY_BUFFER associates
																	 // indices to the bound GL_ARRAY_BUFFER
	gl_has_errors();

	// add the "vignette" effect
	const GLuint vignette_program = effects[(GLuint)EFFECT_ASSET_ID::VIGNETTE];

	// set clock
	GLuint time_uloc       = glGetUniformLocation(vignette_program, "time");
	GLuint screen_darkness_uloc = glGetUniformLocation(vignette_program, "darken_screen_factor");

	GLuint vignette_factor_uloc = glGetUniformLocation(vignette_program, "vignette_factor");
	GLboolean pause_status = glGetUniformLocation(vignette_program, "is_paused");

	glUniform1f(time_uloc, (float)(glfwGetTime() * 10.0f));

	ScreenState &screen = registry.screenStates.get(screen_state_entity);

	Entity player_entity = registry.players.entities[0];
	Health player_health = registry.healths.get(player_entity);

	float vignette_intensity = max(1.0f - (player_health.currentHealth / player_health.maxHealth), 0.5f);

	
	glUniform1f(screen_darkness_uloc, screen.darken_screen_factor);
	glUniform1f(vignette_factor_uloc, clamp(screen.vignette_factor, 0.0f, 1.0f) * vignette_intensity);
	glUniform1f(pause_status, screen.is_paused);

	gl_has_errors();

	// Set the vertex position and vertex texture coordinates (both stored in the
	// same VBO)
	GLint in_position_loc = glGetAttribLocation(vignette_program, "in_position");
	glEnableVertexAttribArray(in_position_loc);
	glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void *)0);
	gl_has_errors();

	// Bind our texture in Texture Unit 0
	glActiveTexture(GL_TEXTURE0);

	glBindTexture(GL_TEXTURE_2D, off_screen_render_buffer_color);
	gl_has_errors();

	// Draw
	glDrawElements(
		GL_TRIANGLES, 3, GL_UNSIGNED_SHORT,
		nullptr); // one triangle = 3 vertices; nullptr indicates that there is
				  // no offset from the bound index buffer
	gl_has_errors();
}


// Render particles using instanced rendering
// Munn: At the moment, it uses one draw call for each particle_emitter, but we can change this to be one draw call for all the particles if we run into performance issues
void RenderSystem::drawParticles(const mat3& projection) {

	// Enable alpha
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	// Use particle shader
	const GLuint program = (GLuint)effects[(int)EFFECT_ASSET_ID::PARTICLE];
	glUseProgram(program);
	gl_has_errors();

	// Get and set vertex and index buffers
	const GLuint vbo = vertex_buffers[(GLuint)GEOMETRY_BUFFER_ID::SPRITE];
	const GLuint ibo = index_buffers[(GLuint)GEOMETRY_BUFFER_ID::SPRITE];

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	gl_has_errors();

	// Set position and texcoord values
	GLint in_position_loc = glGetAttribLocation(program, "in_position");
	GLint in_texcoord_loc = glGetAttribLocation(program, "in_texcoord");
	gl_has_errors();
	assert(in_texcoord_loc >= 0);

	glEnableVertexAttribArray(in_position_loc);
	glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE,
		sizeof(TexturedVertex), (void*)0);
	gl_has_errors();

	glEnableVertexAttribArray(in_texcoord_loc);
	glVertexAttribPointer(
		in_texcoord_loc, 2, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex),
		(void*)sizeof(vec3)); // note the stride to skip the preceeding vertex position

	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); 


	for (ParticleEmitterContainer& particle_emitter_container : registry.particle_emitter_containers.components) { // Get Container
		for (auto& pair : particle_emitter_container.particle_emitter_map) { // Get each particle_emitter from map
			ParticleEmitter& particle_emitter = pair.second;
			int num_alive_particles = 0;

			for (Particle particle : particle_emitter.particles) {
				if (particle.lifetime > 0) {
					num_alive_particles++;
				}
			}

			if (num_alive_particles == 0) {
				continue;
			}

            std::vector<ParticleInfo> particle_info(num_alive_particles);

            int alive_index = 0;
            for (const Particle& particle : particle_emitter.particles) {
                if (particle.lifetime > 0) {
                    Transform transform;
                    transform.translate(particle.position);

					vec2 texture_dimension = texture_dimensions[(int)particle_emitter.sprite_id];
					vec2 trueScale = vec2(particle.scale.x * texture_dimension.x * PIXEL_SCALE_FACTOR,
						particle.scale.y * texture_dimension.y * PIXEL_SCALE_FACTOR);
					transform.scale(trueScale);


					particle_info[alive_index].transform_matrix = transform.mat;
					particle_info[alive_index].color = particle.color;
                    alive_index++;
                }
            }

            // Enabling and binding texture to slot 0
            glActiveTexture(GL_TEXTURE0);
            GLuint texture_id = texture_gl_handles[(GLuint)particle_emitter.sprite_id];
            glBindTexture(GL_TEXTURE_2D, texture_id);
            gl_has_errors();

			transform_vbo = instance_buffers[(int)GEOMETRY_BUFFER_ID::PARTICLE];
            glBindBuffer(GL_ARRAY_BUFFER, transform_vbo);
			glBufferData(GL_ARRAY_BUFFER, sizeof(ParticleInfo) * particle_info.size(), particle_info.data(), GL_DYNAMIC_DRAW);


            GLint transform_loc = glGetAttribLocation(program, "in_transform_matrix");
            for (int i = 0; i < 3; i++) {
                glEnableVertexAttribArray(transform_loc + i);
                glVertexAttribPointer(transform_loc + i, 3, GL_FLOAT, GL_FALSE, 
                    sizeof(ParticleInfo), (void*)(sizeof(float) * 3 * i));
                glVertexAttribDivisor(transform_loc + i, 1);
            }

            // Setup color attribute
            GLint color_loc = glGetAttribLocation(program, "in_color");
            glEnableVertexAttribArray(color_loc);
            glVertexAttribPointer(color_loc, 4, GL_FLOAT, GL_FALSE, sizeof(ParticleInfo), (void*)(sizeof(float) * 3 * 3));
            glVertexAttribDivisor(color_loc, 1);

            GLuint projection_loc = glGetUniformLocation(program, "projection");
            glUniformMatrix3fv(projection_loc, 1, GL_FALSE, (float*)&projection);

            GLint size = 0;
            glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
            GLsizei num_indices = size / sizeof(uint16_t);
            glDrawElementsInstanced(GL_TRIANGLES, num_indices, GL_UNSIGNED_SHORT, nullptr, num_alive_particles);
            gl_has_errors();

            // Reset attribute divisors
            for (int i = 0; i < 3; i++) {
                glVertexAttribDivisor(transform_loc + i, 0);
            }
            glVertexAttribDivisor(color_loc, 0);
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}


void RenderSystem::drawMinimap() {
	// Draw the Minimap
	if (registry.minimaps.size() == 0) {
		return;
	}

	Entity minimap_entity = registry.minimaps.entities[0];
	mat3 screenMatrix = createScreenMatrix();
	drawTexturedMesh(minimap_entity, screenMatrix);


	//////////////////////////
	// Draw the player icon //
	//////////////////////////

	// Get transform
	Entity entity = registry.players.entities[0];
	Transformation& entity_transform = registry.transforms.get(entity);
	Motion& entity_motion = registry.motions.get(entity);

	// Get texture dimension
	ivec2& texture_dimension = texture_dimensions[(GLuint)TEXTURE_ASSET_ID::PLAYER_ICON];

	Transformation minimap_transform = registry.transforms.get(minimap_entity);

	Transform transform;

	// CALCULATING RELATIVE POSITION ON THE SCREEN TO RENDER THE PLAYER
	// Get minimap position
	vec2 minimap_pos = vec2(minimap_transform.position);

	// Get map size
	vec2 MAP_SIZE = vec2(100, 100) * (float)TILE_SIZE; // HARD CODED MAP SIZE

	// Get player position relative to map size (0-1)
	vec2 relative_map_pos = vec2(
		entity_transform.position.x / MAP_SIZE.x,
		entity_transform.position.y / MAP_SIZE.y
	);

	vec2 minimap_texture_dimension = texture_dimensions[(GLuint)TEXTURE_ASSET_ID::MINIMAP];
	minimap_texture_dimension *= minimap_transform.scale.x * PIXEL_SCALE_FACTOR;

	// Scale to minimap position (-minimap_scale to minimap_scale)
	vec2 offset = vec2(
		relative_map_pos.x * minimap_texture_dimension.x - (minimap_texture_dimension.x / 2.0f),
		relative_map_pos.y * minimap_texture_dimension.y - (minimap_texture_dimension.y / 2.0f)
	);

	// Set position
	transform.translate(minimap_pos + offset);

	// Flip player icon according to movement direction
	float sign = 1.0;
	if (entity_motion.velocity.x < 0) {
		sign = -1.0;
	}

	// Scale player icon to arbitrary amount
	vec2 trueScale = vec2(
		texture_dimension.x * sign,
		texture_dimension.y
	) * 2.0f;
	transform.scale(trueScale);

	// The rest is just rendering stuff, pretty much drawTexturedMesh copied over

	const GLuint program = effects[(GLuint)EFFECT_ASSET_ID::TEXTURED];

	// Setting shaders
	glUseProgram(program);
	gl_has_errors();

	const GLuint vbo = vertex_buffers[(GLuint)GEOMETRY_BUFFER_ID::SPRITE];
	const GLuint ibo = index_buffers[(GLuint)GEOMETRY_BUFFER_ID::SPRITE];

	// Setting vertex and index buffers
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	gl_has_errors();

	GLint in_position_loc = glGetAttribLocation(program, "in_position");
	GLint in_texcoord_loc = glGetAttribLocation(program, "in_texcoord");
	gl_has_errors();
	assert(in_texcoord_loc >= 0);

	glEnableVertexAttribArray(in_position_loc);
	glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE,
		sizeof(TexturedVertex), (void*)0);
	gl_has_errors();

	glEnableVertexAttribArray(in_texcoord_loc);
	glVertexAttribPointer(
		in_texcoord_loc, 2, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex),
		(void*)sizeof(
			vec3)); // note the stride to skip the preceeding vertex position

	// Enabling and binding texture to slot 0
	glActiveTexture(GL_TEXTURE0);
	gl_has_errors();

	assert(registry.renderRequests.has(entity));
	GLuint texture_id = texture_gl_handles[(GLuint)TEXTURE_ASSET_ID::PLAYER_ICON];

	glBindTexture(GL_TEXTURE_2D, texture_id);
	gl_has_errors();

	// Getting uniform locations for glUniform* calls
	GLint color_uloc = glGetUniformLocation(program, "fcolor");
	const vec3 color = registry.colors.has(entity) ? registry.colors.get(entity) : vec3(1);
	glUniform3fv(color_uloc, 1, (float*)&color);
	gl_has_errors();

	// Get number of indices from index buffer, which has elements uint16_t
	GLint size = 0;
	glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
	gl_has_errors();

	GLsizei num_indices = size / sizeof(uint16_t);
	// GLsizei num_triangles = num_indices / 3;

	GLint currProgram;
	glGetIntegerv(GL_CURRENT_PROGRAM, &currProgram);
	// Setting uniform values to the currently bound program
	GLuint transform_loc = glGetUniformLocation(currProgram, "transform");
	glUniformMatrix3fv(transform_loc, 1, GL_FALSE, (float*)&transform.mat);
	gl_has_errors();

	GLuint projection_loc = glGetUniformLocation(currProgram, "projection");

	glUniformMatrix3fv(projection_loc, 1, GL_FALSE, (float*)&screenMatrix);
	gl_has_errors();

	// Munn: setting texture filter to NEAREST, instead of the defeault LINEAR (for pixel art)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // when scaling down
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); // when scaling up

	// Drawing of num_indices/3 triangles specified in the index buffer
	glDrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_SHORT, nullptr);
	gl_has_errors();
}







const float UI_CORNER_OFFSET = 16 * PIXEL_SCALE_FACTOR;

void RenderSystem::drawPlayerHUD() {
	
	drawHealthPlayerBar();

	vec2 left_position = vec2(
		UI_CORNER_OFFSET + 2 * PIXEL_SCALE_FACTOR,
		WINDOW_HEIGHT_PX - UI_CORNER_OFFSET
	);
	vec2 right_position = vec2(
		WINDOW_WIDTH_PX - (UI_CORNER_OFFSET + 2 * PIXEL_SCALE_FACTOR),
		WINDOW_HEIGHT_PX - UI_CORNER_OFFSET
	);

	Entity player_entity = registry.players.entities[0];
	SpellSlotContainer player_spell_container = registry.spellSlotContainers.get(player_entity);
	SpellSlot projectile_spell_slot = player_spell_container.spellSlots[0];
	SpellSlot movement_spell_slot = player_spell_container.spellSlots[1];

	ProjectileSpell* projectile_spell = projectile_spells[(int)projectile_spell_slot.spell_id];
	TEXTURE_ASSET_ID projectile_spell_id = projectile_spell->getAssetID();

	MovementSpell* movement_spell = movement_spells[(int)movement_spell_slot.spell_id];
	TEXTURE_ASSET_ID movement_spell_id = movement_spell->getAssetID();
	drawSpellUI(projectile_spell_id, left_position);
	drawSpellUI(movement_spell_id, right_position);

	drawFloorGoals(createScreenMatrix());

	drawMinimap();
}




std::string formatTime(float time) {
	int mins = int(time) / 60;
	int secs = int(time - mins * 60);

	std::string secs_string = std::to_string(secs);
	if (secs < 10) {
		secs_string = "0" + secs_string;
	}

	return std::to_string(mins) + ":" + secs_string;
}


// positive means success, 0 means in progress, negative means failed
int getGoalStatus(FloorGoal floor_goal) {
	GoalManager& goal_manager = registry.goalManagers.components[0];

	switch (floor_goal.goal_type) {
	case GoalType::TIME:
		if (goal_manager.timer_active) {
			return 0;
		}
		return floor_goal.time_to_beat - goal_manager.current_time;
	case GoalType::KILLS:
		if (goal_manager.current_kills >= floor_goal.num_kills_needed) {
			return 1;
		}

		if (goal_manager.timer_active) {
			return 0;
		}
		
		return -1;;
	case GoalType::TIMES_HIT:
		if (goal_manager.current_times_hit >= floor_goal.num_times_hit) {
			return -1;
		}

		if (goal_manager.timer_active) {
			return 0;
		}

		return 1;
	default:
		std::cout << "Invalid goal type in getGoalStatus!" << std::endl;
	}
	return -1;
}

vec3 getTextColour(FloorGoal floor_goal) {
	GoalManager& goal_manager = registry.goalManagers.components[0];

	vec3 finished_color = vec3(70, 130, 50) / 255.f;
	vec3 failed_color = vec3(165, 48, 48) / 255.f;
	vec3 default_color = vec3(1, 1, 1);
	
	int result = getGoalStatus(floor_goal);
	
	if (result == 0) {
		return default_color;
	}
	else if (result > 0) {
		return finished_color;
	}
	else {
		return failed_color;
	}
}

std::string getGoalText(GoalManager goal_manager, FloorGoal goal) {

	switch (goal.goal_type) {
	case GoalType::KILLS:
		return "Kill Enemies: " + std::to_string(goal_manager.current_kills) + "/" + std::to_string(goal.num_kills_needed);
	case GoalType::TIME:
		return "Find exit before: " + formatTime(goal.time_to_beat);
	case GoalType::TIMES_HIT:
		return "Dont get hit: " + std::to_string(goal_manager.current_times_hit) + "/" + std::to_string(goal.num_times_hit);
	}

	return "Error: Invalid goal type";
}

void RenderSystem::drawFloorGoals(const mat3 projection) {
	GoalManager& goal_manager = registry.goalManagers.components[0];

	if (goal_manager.goals.size() != NUM_FLOOR_GOALS) {
		return;
	}

 
	Transform transform;
	transform.translate(vec2(30, 125));
	transform.scale(vec2(0.4));

	FloorGoal easy = goal_manager.goals[0];
	FloorGoal medium = goal_manager.goals[1];
	FloorGoal hard = goal_manager.goals[2]; 

	vec3 text_color = vec3(222, 158, 65) / 255.f;
	std::string goal_text = "GOALS";
	drawText(goal_text, text_color, transform, projection);

	transform.translate(vec2(0, 100));

	std::string easy_text = getGoalText(goal_manager, easy);
	drawText(easy_text, getTextColour(easy), transform, projection);

	transform.translate(vec2(0, 100));

	std::string medium_text = getGoalText(goal_manager, medium);
	drawText(medium_text, getTextColour(medium), transform, projection);

	transform.translate(vec2(0, 100));

	std::string hard_text = getGoalText(goal_manager, hard);
	drawText(hard_text, getTextColour(hard), transform, projection);

	transform.translate(vec2(0, 100)); 

	std::string current_time = "Current Time: " + formatTime(goal_manager.current_time);
	drawText(current_time, text_color, transform, projection);
}

void RenderSystem::drawHealthPlayerBar() {
	// 
	// Drawing the Health bar Bottom
	// 

	GLuint program = effects[(GLuint)EFFECT_ASSET_ID::TEXTURED];
	glUseProgram(program);
	gl_has_errors();

	// bind the geometry buffer (quad) for our health bars
	GLuint vbo = vertex_buffers[(int)GEOMETRY_BUFFER_ID::SPRITE];
	GLuint ibo = index_buffers[(int)GEOMETRY_BUFFER_ID::SPRITE];
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);


	GLint in_position_loc = glGetAttribLocation(program, "in_position");
	glEnableVertexAttribArray(in_position_loc);
	glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE,
		sizeof(TexturedVertex), (void*)0);

	GLint in_texcoord_loc = glGetAttribLocation(program, "in_texcoord");
	glEnableVertexAttribArray(in_texcoord_loc);
	glVertexAttribPointer(
		in_texcoord_loc, 2, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex),
		(void*)sizeof(vec3));

	// Munn: for screen space coordinates, not world coordinates
	mat3 projection = createScreenMatrix();

	Transform transform;

	vec2 texture_dimension = texture_dimensions[(int)TEXTURE_ASSET_ID::HEALTH_BAR_BOTTOM];

	transform.translate(vec2(texture_dimension.x / 2.0f + UI_CORNER_OFFSET + 16 * PIXEL_SCALE_FACTOR, UI_CORNER_OFFSET));

	vec2 trueScale = vec2(texture_dimension.x * PIXEL_SCALE_FACTOR, texture_dimension.y * PIXEL_SCALE_FACTOR);
	transform.scale(trueScale);

	// Getting uniform locations for glUniform* calls
	GLint color_uloc = glGetUniformLocation(program, "fcolor");
	vec3 color = vec3(1);
	glUniform3fv(color_uloc, 1, (float*)&color);
	gl_has_errors();

	GLint is_hitflash_uloc = glGetUniformLocation(program, "is_hitflash");
	if (is_hitflash_uloc > -1) { 
		glUniform1i(is_hitflash_uloc, false);
	}

	// Setting uniform values to the currently bound program
	GLuint transform_loc = glGetUniformLocation(program, "transform");
	glUniformMatrix3fv(transform_loc, 1, GL_FALSE, (float*)&transform.mat);
	gl_has_errors();

	GLuint projection_loc = glGetUniformLocation(program, "projection");
	glUniformMatrix3fv(projection_loc, 1, GL_FALSE, (float*)&projection);
	gl_has_errors();


	// Enabling and binding texture to slot 0
	glActiveTexture(GL_TEXTURE0);
	GLuint texture_id = texture_gl_handles[(GLuint)TEXTURE_ASSET_ID::HEALTH_BAR_BOTTOM];
	glBindTexture(GL_TEXTURE_2D, texture_id);
	gl_has_errors();

	// Get number of indices from index buffer, which has elements uint16_t
	GLint size = 0;
	glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
	gl_has_errors();

	GLsizei num_indices = size / sizeof(uint16_t);

	// Munn: setting texture filter to NEAREST, instead of the defeault LINEAR (for pixel art)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // when scaling down
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); // when scaling up

	// Drawing of num_indices/3 triangles specified in the index buffer
	glDrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_SHORT, nullptr);
	gl_has_errors();
	

	// 
	// Drawing the Health bar Fill
	// 

	program = effects[(GLuint)EFFECT_ASSET_ID::HEALTH_BAR];
	glUseProgram(program);
	gl_has_errors();

	// bind the geometry buffer (quad) for our health bars
	vbo = vertex_buffers[(int)GEOMETRY_BUFFER_ID::SPRITE];
	ibo = index_buffers[(int)GEOMETRY_BUFFER_ID::SPRITE];
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);


	in_position_loc = glGetAttribLocation(program, "in_position");
	glEnableVertexAttribArray(in_position_loc);
	glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)0);

	in_texcoord_loc = glGetAttribLocation(program, "in_texcoord");
	glEnableVertexAttribArray(in_texcoord_loc);
	glVertexAttribPointer(in_texcoord_loc, 2, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)sizeof(vec3));

	texture_dimension = texture_dimensions[(int)TEXTURE_ASSET_ID::HEALTH_BAR_FILL];

	Entity player_entity = registry.players.entities[0];
	Health player_health = registry.healths.get(player_entity);

	// Getting uniform locations for glUniform* calls
	GLint health_uloc = glGetUniformLocation(program, "health_percent");
	float health_percent = max(0.f, player_health.currentHealth / player_health.maxHealth);
	glUniform1fv(health_uloc, 1, (float*)&health_percent);
	gl_has_errors();

	transform = Transform();

	transform.translate(vec2(texture_dimension.x / 2.0f + UI_CORNER_OFFSET + 16 * PIXEL_SCALE_FACTOR, UI_CORNER_OFFSET));
	transform.translate(vec2(-2 * texture_dimension.x * (1 - health_percent), 0)); 

	trueScale = vec2(texture_dimension.x * PIXEL_SCALE_FACTOR, texture_dimension.y * PIXEL_SCALE_FACTOR);
	transform.scale(trueScale);

	

	color_uloc = glGetUniformLocation(program, "fcolor");
	transform_loc = glGetUniformLocation(program, "transform");
	projection_loc = glGetUniformLocation(program, "projection");

	color = vec3(1);
	glUniform3fv(color_uloc, 1, (float*)&color);
	glUniformMatrix3fv(transform_loc, 1, GL_FALSE, (float*)&transform.mat);
	glUniformMatrix3fv(projection_loc, 1, GL_FALSE, (float*)&projection);
	gl_has_errors();


	// Enabling and binding texture to slot 0
	glActiveTexture(GL_TEXTURE0);
	texture_id = texture_gl_handles[(GLuint)TEXTURE_ASSET_ID::HEALTH_BAR_FILL];
	glBindTexture(GL_TEXTURE_2D, texture_id);
	gl_has_errors();

	// Get number of indices from index buffer, which has elements uint16_t
	size = 0;
	glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
	gl_has_errors();

	num_indices = size / sizeof(uint16_t);

	// Munn: setting texture filter to NEAREST, instead of the defeault LINEAR (for pixel art)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // when scaling down
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); // when scaling up

	// Drawing of num_indices/3 triangles specified in the index buffer
	glDrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_SHORT, nullptr);
	gl_has_errors();



	// 
	// Draw Heart Container
	// 
	program = effects[(GLuint)EFFECT_ASSET_ID::ANIMATED];
	glUseProgram(program);
	gl_has_errors();

	// bind the geometry buffer (quad) for our health bars
	vbo = vertex_buffers[(int)GEOMETRY_BUFFER_ID::SPRITE];
	ibo = index_buffers[(int)GEOMETRY_BUFFER_ID::SPRITE];
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);


	in_position_loc = glGetAttribLocation(program, "in_position");
	glEnableVertexAttribArray(in_position_loc);
	glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)0);

	in_texcoord_loc = glGetAttribLocation(program, "in_texcoord");
	glEnableVertexAttribArray(in_texcoord_loc);
	glVertexAttribPointer(in_texcoord_loc, 2, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)sizeof(vec3));

	texture_dimension = texture_dimensions[(int)TEXTURE_ASSET_ID::HEART_CONTAINER];

	transform = Transform();

	transform.translate(vec2(UI_CORNER_OFFSET - (2 * PIXEL_SCALE_FACTOR), UI_CORNER_OFFSET));

	trueScale = vec2(texture_dimension.x * PIXEL_SCALE_FACTOR, texture_dimension.y * PIXEL_SCALE_FACTOR);
	transform.scale(trueScale);

	int current_frame_loc = glGetUniformLocation(program, "current_frame");
	int h_frames_loc = glGetUniformLocation(program, "h_frames");
	int v_frames_loc = glGetUniformLocation(program, "v_frames");

	int num_heart_stages = 5;
	if (health_percent < 0) {
		health_percent = 0;
	}

	is_hitflash_uloc = glGetUniformLocation(program, "is_hitflash");
	if (is_hitflash_uloc > -1) { 
		glUniform1i(is_hitflash_uloc, false);
	}

	glUniform1f(current_frame_loc, (int)((1 - health_percent) * (num_heart_stages - 1)));
	glUniform1f(h_frames_loc, num_heart_stages);
	glUniform1f(v_frames_loc, 1);

	transform_loc = glGetUniformLocation(program, "transform");
	projection_loc = glGetUniformLocation(program, "projection");

	glUniformMatrix3fv(transform_loc, 1, GL_FALSE, (float*)&transform.mat);
	glUniformMatrix3fv(projection_loc, 1, GL_FALSE, (float*)&projection);
	gl_has_errors();


	// Enabling and binding texture to slot 0
	glActiveTexture(GL_TEXTURE0);
	texture_id = texture_gl_handles[(GLuint)TEXTURE_ASSET_ID::HEART_CONTAINER];
	glBindTexture(GL_TEXTURE_2D, texture_id);
	gl_has_errors();

	// Get number of indices from index buffer, which has elements uint16_t
	size = 0;
	glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
	gl_has_errors();

	num_indices = size / sizeof(uint16_t);

	// Munn: setting texture filter to NEAREST, instead of the defeault LINEAR (for pixel art)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // when scaling down
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); // when scaling up

	// Drawing of num_indices/3 triangles specified in the index buffer
	glDrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_SHORT, nullptr);
	gl_has_errors();
}

void RenderSystem::drawSpellUI(TEXTURE_ASSET_ID asset_id, vec2 screen_position) {
	//
	// Draw container
	//
	GLuint program = effects[(GLuint)EFFECT_ASSET_ID::TEXTURED];
	glUseProgram(program);
	gl_has_errors();

	// bind the geometry buffer (quad) for our health bars
	GLuint vbo = vertex_buffers[(int)GEOMETRY_BUFFER_ID::SPRITE];
	GLuint ibo = index_buffers[(int)GEOMETRY_BUFFER_ID::SPRITE];
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);


	GLint in_position_loc = glGetAttribLocation(program, "in_position");
	glEnableVertexAttribArray(in_position_loc);
	glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE,
		sizeof(TexturedVertex), (void*)0);

	GLint in_texcoord_loc = glGetAttribLocation(program, "in_texcoord");
	glEnableVertexAttribArray(in_texcoord_loc);
	glVertexAttribPointer(
		in_texcoord_loc, 2, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex),
		(void*)sizeof(vec3));

	// Munn: for screen space coordinates, not world coordinates
	mat3 projection = createScreenMatrix();

	Transform transform;

	vec2 texture_dimension = texture_dimensions[(int)TEXTURE_ASSET_ID::SPELL_CONTAINER_UI];

	transform.translate(screen_position);

	vec2 trueScale = vec2(texture_dimension.x * PIXEL_SCALE_FACTOR, texture_dimension.y * PIXEL_SCALE_FACTOR);
	transform.scale(trueScale);

	// Getting uniform locations for glUniform* calls
	GLint color_uloc = glGetUniformLocation(program, "fcolor");
	vec3 color = vec3(1);
	glUniform3fv(color_uloc, 1, (float*)&color);
	gl_has_errors();

	GLint is_hitflash_uloc = glGetUniformLocation(program, "is_hitflash");
	if (is_hitflash_uloc > -1) {
		glUniform1i(is_hitflash_uloc, false);
	}


	// Setting uniform values to the currently bound program
	GLuint transform_loc = glGetUniformLocation(program, "transform");
	glUniformMatrix3fv(transform_loc, 1, GL_FALSE, (float*)&transform.mat);
	gl_has_errors();

	GLuint projection_loc = glGetUniformLocation(program, "projection");
	glUniformMatrix3fv(projection_loc, 1, GL_FALSE, (float*)&projection);
	gl_has_errors();


	// Enabling and binding texture to slot 0
	glActiveTexture(GL_TEXTURE0);
	GLuint texture_id = texture_gl_handles[(GLuint)TEXTURE_ASSET_ID::SPELL_CONTAINER_UI];
	glBindTexture(GL_TEXTURE_2D, texture_id);
	gl_has_errors();

	// Get number of indices from index buffer, which has elements uint16_t
	GLint size = 0;
	glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
	gl_has_errors();

	GLsizei num_indices = size / sizeof(uint16_t);

	// Munn: setting texture filter to NEAREST, instead of the defeault LINEAR (for pixel art)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // when scaling down
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); // when scaling up

	// Drawing of num_indices/3 triangles specified in the index buffer
	glDrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_SHORT, nullptr);
	gl_has_errors();



	//
	// Draw Icon
	//
	program = effects[(GLuint)EFFECT_ASSET_ID::TEXTURED];
	glUseProgram(program);
	gl_has_errors();

	// bind the geometry buffer (quad) for our health bars
	vbo = vertex_buffers[(int)GEOMETRY_BUFFER_ID::SPRITE];
	ibo = index_buffers[(int)GEOMETRY_BUFFER_ID::SPRITE];
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);


	in_position_loc = glGetAttribLocation(program, "in_position");
	glEnableVertexAttribArray(in_position_loc);
	glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE,
		sizeof(TexturedVertex), (void*)0);

	in_texcoord_loc = glGetAttribLocation(program, "in_texcoord");
	glEnableVertexAttribArray(in_texcoord_loc);
	glVertexAttribPointer(
		in_texcoord_loc, 2, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex),
		(void*)sizeof(vec3));

	// Munn: for screen space coordinates, not world coordinates
	projection = createScreenMatrix();

	transform = Transform();

	texture_dimension = texture_dimensions[(int)asset_id];

	transform.translate(screen_position);

	trueScale = vec2(texture_dimension.x * PIXEL_SCALE_FACTOR, texture_dimension.y * PIXEL_SCALE_FACTOR);
	transform.scale(trueScale);

	// Getting uniform locations for glUniform* calls
	color_uloc = glGetUniformLocation(program, "fcolor");
	color = vec3(1);
	glUniform3fv(color_uloc, 1, (float*)&color);
	gl_has_errors();

	is_hitflash_uloc = glGetUniformLocation(program, "is_hitflash");
	if (is_hitflash_uloc > -1) {
		glUniform1i(is_hitflash_uloc, false);
	}


	// Setting uniform values to the currently bound program
	transform_loc = glGetUniformLocation(program, "transform");
	glUniformMatrix3fv(transform_loc, 1, GL_FALSE, (float*)&transform.mat);
	gl_has_errors();

	projection_loc = glGetUniformLocation(program, "projection");
	glUniformMatrix3fv(projection_loc, 1, GL_FALSE, (float*)&projection);
	gl_has_errors();


	// Enabling and binding texture to slot 0
	glActiveTexture(GL_TEXTURE0);
	texture_id = texture_gl_handles[(GLuint)asset_id];
	glBindTexture(GL_TEXTURE_2D, texture_id);
	gl_has_errors();

	// Get number of indices from index buffer, which has elements uint16_t
	size = 0;
	glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
	gl_has_errors();

	num_indices = size / sizeof(uint16_t);

	// Munn: setting texture filter to NEAREST, instead of the defeault LINEAR (for pixel art)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // when scaling down
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); // when scaling up

	// Drawing of num_indices/3 triangles specified in the index buffer
	glDrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_SHORT, nullptr);
	gl_has_errors();
}

void RenderSystem::drawText(std::string text, const glm::vec3& color, Transform trans, const glm::mat3& projection, float alpha, TEXT_PIVOT pivot)
{
	// Munn: For some reason, flip transform matrix on Y axis
	trans.scale(vec2(1, -1)); 

	// activate the shader program
	GLuint program = effects[(GLuint)EFFECT_ASSET_ID::FONT];
	glUseProgram(program);
	gl_has_errors();

	// get shader uniforms
	GLint textColor_location =
		glGetUniformLocation(program, "textColor");
	glUniform3f(textColor_location, color.x, color.y, color.z);

	GLint alphaLoc = glGetUniformLocation(program, "alpha");
	glUniform1f(alphaLoc, alpha);

	GLint transformLoc =
		glGetUniformLocation(program, "transform");
	glUniformMatrix3fv(transformLoc, 1, GL_FALSE, glm::value_ptr(trans.mat));

	GLint projectionLoc =
		glGetUniformLocation(program, "projection");
	glUniformMatrix3fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

	glBindVertexArray(m_font_VAO);
	gl_has_errors();
	
	// Get the width of the entire word
	float text_width = 0;

	std::string::const_iterator c1;
	for (c1 = text.begin(); c1 != text.end(); c1++)
	{
		Character ch = m_ftCharacters[*c1];

		text_width += (ch.Advance >> 6);
	}

	float offset = 0;
	if (pivot == TEXT_PIVOT::LEFT) {
		offset = 0;
	}
	else if (pivot == TEXT_PIVOT::CENTER) {
		offset = -text_width / 2;
	}
	else if (pivot == TEXT_PIVOT::RIGHT) {
		offset = -text_width;
	}

	float x = offset;
	float y = 0;
	// iterate through all characters
	std::string::const_iterator c;
	for (c = text.begin(); c != text.end(); c++)
	{
		Character ch = m_ftCharacters[*c];

		float xpos = x + ch.Bearing.x;
		float ypos = y + (ch.Size.y - ch.Bearing.y);

		float w = ch.Size.x;
		float h = ch.Size.y;
		// update VBO for each character
		float vertices[6][4] = {
			{ xpos,     ypos + h,   0.0f, 0.0f },
			{ xpos,     ypos,       0.0f, 1.0f },
			{ xpos + w, ypos,       1.0f, 1.0f },

			{ xpos, ypos + h, 0.0f, 0.0f },
			{ xpos + w, ypos,       1.0f, 1.0f },
			{ xpos + w, ypos + h,   1.0f, 0.0f }
		};

		// render glyph texture over quad
		glBindTexture(GL_TEXTURE_2D, ch.TextureID);
		// std::cout << "binding texture: " << ch.character << " = " << ch.TextureID << std::endl;
		gl_has_errors();

		// update content of VBO memory
		const GLuint vbo = vertex_buffers[(GLuint)GEOMETRY_BUFFER_ID::FONT];
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		gl_has_errors();

		// render quad
		glDrawArrays(GL_TRIANGLES, 0, 6);
		gl_has_errors();

		// now advance cursors for next glyph (note that advance is number of 1/64 pixels)
		x += (ch.Advance >> 6); // bitshift by 6 to get value in pixels (2^6 = 64)
	}

	glBindVertexArray(m_vao);
	glBindTexture(GL_TEXTURE_2D, 0);
	gl_has_errors();
}










// Render our game world
// http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-14-render-to-texture/
void RenderSystem::draw(GAME_SCREEN_ID game_screen)
{
	// Getting size of window
	int w, h;


	glfwGetFramebufferSize(window, &w, &h); // Note, this will be 2x the resolution given to glfwCreateWindow on retina displays

	// First render to the custom framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);
	gl_has_errors();

	// clear backbuffer
	glViewport(0, 0, w, h);
	glDepthRange(0.00001, 10);

	// white background
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	glClearDepth(10.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST); // native OpenGL does not work with a depth buffer
	// and alpha blending, one would have to sort
	// sprites back to front
	gl_has_errors();

	mat3 projection_2D = createProjectionMatrix();
	mat3 screen_2D = createScreenMatrix();


	for (Entity entity : registry.backgroundImages.entities) {
		drawTexturedMesh(entity, screen_2D);
	}

	for (Entity entity : registry.dialogueBoxes.entities) {
		drawTexturedMesh(entity, screen_2D);
	}


	// Draw background first
	drawEnvironment(projection_2D);


	// Then Draw particles
	drawParticles(projection_2D);


	for (Entity entity : registry.floorDecors.entities) {
		if (isFrustumCulled(entity)) {
			continue;
		}
		drawTexturedMesh(entity, projection_2D);
	}



	// Draw moving entities y-sorted (Munn: Not projectiles though, since that's... weird?)
	std::vector<Entity> y_sort_entities;

	// Add Player
	for (Entity entity : registry.players.entities) {
		// Don't render player in cutscenes
		if ((int)game_screen >= (int)GAME_SCREEN_ID::CUTSCENE_INTRO) {
			break;
		}
		y_sort_entities.push_back(entity);
	}
	// Add enemies
	for (Entity entity : registry.enemies.entities) {

		// Munn: Cull here, so we don't sort the enemies that will be culled. Maybe cull again in drawTexturedMesh?
		if (isFrustumCulled(entity)) {
			continue;
		}

		y_sort_entities.push_back(entity);
	}
	// Add interactables
	for (Entity entity : registry.interactables.entities) {
		if (isFrustumCulled(entity)) {
			continue;
		}

		y_sort_entities.push_back(entity);
	}
	for (Entity entity : registry.chests.entities) {
		if (isFrustumCulled(entity)) {
			continue;
		}

		y_sort_entities.push_back(entity);
	}
	// Add environment objects
	for (Entity entity : registry.environmentObjects.entities) {
		if (isFrustumCulled(entity)) {
			continue;
		}

		y_sort_entities.push_back(entity);
	}

	

	// Sort entities
	y_sort_entities = ySort(y_sort_entities);

	// Render sorted entities
	for (Entity entity : y_sort_entities) {
		if (registry.interactables.has(entity)) {
			Interactable interactable = registry.interactables.get(entity);

			// Don't draw shadows for the following
			if (interactable.interactable_id == INTERACTABLE_ID::NEXT_LEVEL_ENTRY ||
				interactable.interactable_id == INTERACTABLE_ID::FOUNTAIN ||
				interactable.interactable_id == INTERACTABLE_ID::SACRIFICE_FOUNTAIN) {
				drawTexturedMesh(entity, projection_2D);
				continue;
			}
		}


		// Only draw shadows for enemies and interactables
		if (registry.enemies.has(entity) || registry.interactables.has(entity)) {
			drawShadow(entity, projection_2D);
			drawTexturedMesh(entity, projection_2D);
			continue;
		}

		drawTexturedMesh(entity, projection_2D);
	}

	// Render projectiles
	for (Entity entity : registry.projectiles.entities) {
		if (isFrustumCulled(entity)) {
			continue;
		}

		drawTexturedMesh(entity, projection_2D);
	}

	// NEW
	// Mark: In intro screen 
	// DO NOT DRAW PLAYER UI IN INTRO, OR DURING CUTSCENES
	if (game_screen != GAME_SCREEN_ID::INTRO && (int)game_screen < (int)GAME_SCREEN_ID::CUTSCENE_INTRO) {
		drawHealthBars();
		drawPlayerHUD();
	}

	// Create a transform matrix
	//Transform text_transform;
	//text_transform.translate(vec2(WINDOW_WIDTH_PX / 2, WINDOW_HEIGHT_PX / 2));


	for (Entity entity : registry.textPopups.entities) {
		TextPopup text_popup = registry.textPopups.get(entity);

		Transform transform;
		transform.translate(*text_popup.translation);
		transform.scale(text_popup.scale);
		transform.rotate(text_popup.rotation);

		drawText(text_popup.text, text_popup.color, transform, text_popup.in_screen ? screen_2D : projection_2D, *text_popup.alpha, text_popup.pivot);

		//std::cout << "Yerp" << text_popup.translation.x << ", " << text_popup.translation.y << std::endl;
	}

	//// Create string
	//std::string test_text = "I am on your screen!";

	//// Draw text
	//drawText(test_text, vec3(1.0, 0.0, 0.0), text_transform, screen_2D); 
	

	// Example of text projected into the world space
	/*Transform floor_text_transform;
	floor_text_transform.translate(vec2(3, 3) * (float)TILE_SIZE);

	std::string test_floor_text = "I am in the world!";
	drawText(test_floor_text, vec3(1.0, 0.0, 0.0), floor_text_transform, projection_2D);*/
	if (game_screen == GAME_SCREEN_ID::INTRO)
	{
		for (Entity text_entity : registry.texts.entities)
		{
			Text& text = registry.texts.get(text_entity);
			Transform text_transform;
			text_transform.scale(text.scale);
			text_transform.rotate(text.rotation);
			
			if (!text.mouse_pressed) {
				text_transform.translate(text.translation);
				drawText(text.text, text.color, text_transform, text.in_screen ? screen_2D : projection_2D);
			}
			else {
				text_transform.translate(text.mouse_pressing_translation);
				drawText(text.text, text.mouse_pressing_color, text_transform, text.in_screen ? screen_2D : projection_2D);
			}

		}
	}
	else {
		for (Entity text_entity : registry.texts.entities)
		{
			Text& text = registry.texts.get(text_entity);
			Transform text_transform;
			text_transform.scale(text.scale);
			text_transform.rotate(text.rotation);

			if (!text.mouse_pressed) {
				text_transform.translate(text.translation);
				drawText(text.text, text.color, text_transform, text.in_screen ? screen_2D : projection_2D);
			}
			else {
				text_transform.translate(text.mouse_pressing_translation);
				drawText(text.text, text.mouse_pressing_color, text_transform, text.in_screen ? screen_2D : projection_2D);
			}

		}
	}

	// draw framebuffer to screen
	// adding "vignette" effect when applied
	drawToScreen();

	// Draw text when game pause
	

	Entity screen_state_entity = registry.screenStates.entities[0];
	ScreenState& screen_state = registry.screenStates.get(screen_state_entity);
	if (screen_state.is_paused) {

		glEnable(GL_BLEND); // Enable GL_BLEND again
		gl_has_errors();

		for (Entity text_entity : registry.texts.entities)
		{
			Text& text = registry.texts.get(text_entity);
			Transform text_transform;
			text_transform.scale(text.scale);
			text_transform.rotate(text.rotation);

			if (!text.mouse_pressed) {
				text_transform.translate(text.translation);
				drawText(text.text, text.color, text_transform, text.in_screen ? screen_2D : projection_2D);
			}
			else {
				text_transform.translate(text.mouse_pressing_translation);
				drawText(text.text, text.mouse_pressing_color, text_transform, text.in_screen ? screen_2D : projection_2D);
			}

		}

		for (Entity bar_entity : registry.slideBars.entities) { 
			drawTexturedMesh(bar_entity, screen_2D);
		}
		for (Entity block_entity : registry.slideBlocks.entities) { 
			drawTexturedMesh(block_entity, screen_2D);
		}
	}
	
	// flicker-free display with a double buffer
	glfwSwapBuffers(window);
	gl_has_errors();
}


















void RenderSystem::drawHealthBars()
{
	// select the "COLOUREED" shader program that we will be using
    GLuint program = effects[(GLuint)EFFECT_ASSET_ID::COLOURED];
    glUseProgram(program);
    gl_has_errors();

	// bind the geometry buffer (quad) for our health bars
    GLuint vbo = vertex_buffers[(int)GEOMETRY_BUFFER_ID::HEALTH_BAR];
    GLuint ibo = index_buffers[(int)GEOMETRY_BUFFER_ID::HEALTH_BAR];
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);

	
    GLint in_position_loc = glGetAttribLocation(program, "in_position");
    glEnableVertexAttribArray(in_position_loc);
    glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE,
                          sizeof(ColoredVertex), (void*)0);


	// create a projection matrix for placing the bar onto the screen
    mat3 projection_2D = createProjectionMatrix();

	// iterate through all of the entities that have a heath bar component
    for (uint i = 0; i < registry.healths.size(); i++)
    {
        Entity e = registry.healths.entities[i];
        Health& health  = registry.healths.components[i];

		if (e == registry.players.entities[0]) {
			continue;
		}

        if (!registry.transforms.has(e))
            continue; 

        // compute the fraction of health remaining (normalized to a range [0,1])
        float ratio = health.currentHealth / health.maxHealth;
        if (ratio < 0.f) ratio = 0.f;
        if (ratio > 1.f) ratio = 1.f;

		// get the position of the entity and decide how/where we want to draw the health bar 
        auto& transformComp = registry.transforms.get(e);
        vec2 entityPos      = transformComp.position;
		//  the location and size of health bar 
        float barOffsetY    = 20.f; 
		
        float barWidth      = 40.f;
        float barHeight     = 5.f;

		float barOffsetX = barWidth / 2.0f;

        // 1. Draw a red "background" behind the bar 
        drawHealthBarSegment(entityPos, barWidth, barHeight,
                             vec3(1.f, 0.f, 0.f), barOffsetX, barOffsetY, 1.f, program, projection_2D);

        // 2. Draw a green "foreground" scaled by ratio
        drawHealthBarSegment(entityPos, barWidth * ratio, barHeight,
                             vec3(0.f, 1.f, 0.f), barOffsetX, barOffsetY, 1.f, program, projection_2D);
    }
}
void RenderSystem::drawHealthBarSegment(vec2 entityPos, float w, float h, vec3 color, float offsetX, float offsetY, float depth, GLuint program, const mat3& projection)
{
	// create a transformation for the bar 
    Transform transform;
	// translate to (entity.Pos.x, entityPos.y - offsetY)
    transform.translate({entityPos.x - offsetX, entityPos.y - offsetY});
	// scale to (w,h)
    transform.scale({w, h});

	// send the chosen colour to the uniform in fragment shader
    GLint color_loc = glGetUniformLocation(program, "color");
    glUniform3fv(color_loc, 1, (float*)&color);

	// send the transformation matrix to the vertex shader
    GLint transform_loc = glGetUniformLocation(program, "transform");
    glUniformMatrix3fv(transform_loc, 1, GL_FALSE, (float*)&transform.mat);

	// send the projection matrix to the vertex shader
    GLint projection_loc = glGetUniformLocation(program, "projection");
    glUniformMatrix3fv(projection_loc, 1, GL_FALSE, (float*)&projection);
    gl_has_errors();

	// draw the rectangle 
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);
    gl_has_errors();
}


mat3 RenderSystem::createProjectionMatrix()
{  
	vec2 camera_pos; 
	if (!registry.cameras.entities.empty()) {
		Entity camera = registry.cameras.entities[0];
		Transformation& camera_movement = registry.transforms.get(camera);
		camera_pos = camera_movement.position;
	}  

	// determine the left, right, top, and bottom boundaries of the "view rectangle"
	// the view triangle determines which part of the world that the camera sees
	// the player is at the center of view triangle, and the view traingle updates as the player moves
	float left   = camera_pos.x - (WINDOW_WIDTH_PX / 2.f);
	float right  = camera_pos.x + (WINDOW_WIDTH_PX / 2.f);
	float top    = camera_pos.y - (WINDOW_HEIGHT_PX / 2.f);
	float bottom = camera_pos.y + (WINDOW_HEIGHT_PX / 2.f);

	// fake projection matrix, scaled to window coordinates
	// float left   = 0.f;
	// float top    = 0.f;
	// float right  = (float) WINDOW_WIDTH_PX;
	// float bottom = (float) WINDOW_HEIGHT_PX;

	float sx = 2.f / (right - left);
	float sy = 2.f / (top - bottom);
	float tx = -(right + left) / (right - left);
	float ty = -(top + bottom) / (top - bottom);

	return {
		{ sx, 0.f, 0.f},
		{0.f,  sy, 0.f},
		{ tx,  ty, 1.f}
	};
}

// AABB collision check with camera bounds to tell if should be culled or not, returns true if entity is NOT within camera bounds
bool RenderSystem::isFrustumCulled(Entity entity) {
	if (!registry.transforms.has(entity) || !registry.renderRequests.has(entity)) {
		return true;
	}

	// Get transform and render requests for position + scale
	Transformation& entity_transform = registry.transforms.get(entity);
	RenderRequest& render_request = registry.renderRequests.get(entity);

	vec2 texture_dimension = texture_dimensions[(int)render_request.used_texture];

	// Get camera position
	vec2 camera_pos = vec2(0, 0);
	if (!registry.cameras.entities.empty()) {
		Entity camera = registry.cameras.entities[0];
		Transformation& camera_movement = registry.transforms.get(camera);
		camera_pos = camera_movement.position;
	}

	// Create bounding boxes
	vec2 camera_bounding_box = vec2(WINDOW_WIDTH_PX, WINDOW_HEIGHT_PX) * (float)PIXEL_SCALE_FACTOR / 2.0f;
	vec2 entity_bounding_box = vec2(entity_transform.scale.x * texture_dimension.x, entity_transform.scale.y * texture_dimension.x) * (float)PIXEL_SCALE_FACTOR / 2.0f;

	// Calculate overlap
	bool rightOverlap = camera_pos.x + camera_bounding_box.x > entity_transform.position.x - entity_bounding_box.x;
	bool leftOverlap = camera_pos.x - camera_bounding_box.x < entity_transform.position.x + entity_bounding_box.x;
	bool topOverlap = camera_pos.y + camera_bounding_box.y > entity_transform.position.y - entity_bounding_box.y;
	bool bottomOverlap = camera_pos.y - camera_bounding_box.y < entity_transform.position.y + entity_bounding_box.y;
	
	// Return true if not "colliding" with camera
	return !(topOverlap && bottomOverlap && rightOverlap && leftOverlap); 
}

// Sorts and returns a vector of entities based on the y position of the bottom of their sprite
std::vector<Entity> RenderSystem::ySort(std::vector<Entity> entities) {
	
	// Munn: just using built in sort with a lambda function - apparently std::sort generally runs in O(nlog(n)), hooray!
	// https://stackoverflow.com/questions/1840121/which-type-of-sorting-is-used-in-the-stdsort
	std::sort(entities.begin(), entities.end(), [this](Entity a, Entity b) { // Munn: need to capture "this" to access texture dimensions

		// Get components for position + scale
		Transformation a_transform = registry.transforms.get(a);
		Transformation b_transform = registry.transforms.get(b);
		RenderRequest a_rr = registry.renderRequests.get(a);
		RenderRequest b_rr = registry.renderRequests.get(b);

		vec2 a_scale_divisor = vec2(1);
		vec2 b_scale_divisor = vec2(1);
		if (registry.animation_managers.has(a)) {
			AnimationManager animation_manager = registry.animation_managers.get(a);

			Animation animation = animation_manager.current_animation;
			a_scale_divisor = vec2(
				animation.h_frames,
				animation.v_frames
			);
		}
		if (registry.animation_managers.has(b)) {
			AnimationManager animation_manager = registry.animation_managers.get(b);

			Animation animation = animation_manager.current_animation;
			b_scale_divisor = vec2(
				animation.h_frames,
				animation.v_frames
			);
		}
		if (registry.tiles.has(a)) {
			Tile tile = registry.tiles.get(a);
			a_scale_divisor = vec2(
				tile.h_tiles,
				tile.v_tiles
			);
		}
		if (registry.tiles.has(b)) {
			Tile tile = registry.tiles.get(b);
			b_scale_divisor = vec2(
				tile.h_tiles,
				tile.v_tiles
			);
		}

		// Calculate where the bottom of the sprite is
		float a_bottom = a_transform.position.y + (texture_dimensions[(int)a_rr.used_texture].y * a_transform.scale.y * PIXEL_SCALE_FACTOR / 2.0f / a_scale_divisor.y);
		float b_bottom = b_transform.position.y + (texture_dimensions[(int)b_rr.used_texture].y * b_transform.scale.y * PIXEL_SCALE_FACTOR / 2.0f / b_scale_divisor.y);

		// Sort descending, we want highest Y pos to be first, so entities that are higher up are rendered first
		return a_bottom < b_bottom; 
		});

	return entities;
}

mat3 RenderSystem::createScreenMatrix()
{
  
	//float left = WINDOW_WIDTH_PX / 2.f;
	//float right = WINDOW_WIDTH_PX / 2.f;
	//float top = -WINDOW_WIDTH_PX / 2.f;
	//float bottom = -WINDOW_WIDTH_PX / 2.f;

	// fake projection matrix, scaled to window coordinates
	 float left   = 0.f;
	 float top    = 0.f;
	 float right  = (float) WINDOW_WIDTH_PX;
	 float bottom = (float) WINDOW_HEIGHT_PX;

	float sx = 2.f / (right - left);
	float sy = 2.f / (top - bottom);
	float tx = -(right + left) / (right - left);
	float ty = -(top + bottom) / (top - bottom);

	return {
		{ sx, 0.f, 0.f},
		{0.f,  sy, 0.f},
		{ tx,  ty, 1.f}
	};
}