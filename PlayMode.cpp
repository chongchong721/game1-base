#include "PlayMode.hpp"

#include "read_write_chunk.hpp"
#include <fstream>

//for the GL_ERRORS() macro:
#include "gl_errors.hpp"

//for glm::value_ptr() :
#include <glm/gtc/type_ptr.hpp>

#include <random>

PlayMode::PlayMode() {

	// Palette read
	
	std::vector<PPU466::Palette> palette_tables;
	std::ifstream in("./assets/runtime_palette",std::ios::binary);

	read_chunk<PPU466::Palette>(in,"pal0",&palette_tables);

	for (auto t : palette_tables){
		std::cout<< t.at(0).a << std::endl;
		
	}



	// Sprite read

	std::cout << sizeof(Sprite_Load_Data) << std::endl;

	std::vector<Sprite_Load_Data> sprites_load;
	std::ifstream in1("./assets/runtime_sprite",std::ios::binary);
	read_chunk<Sprite_Load_Data>(in1,"spr0",&sprites_load);

	std::cout << sprites_load.size() << std::endl;
	
	PPU466::Tile mytile;	

	for(auto sprite:sprites_load){
		if(sprite.type == Main){
			mytile.bit0 = sprite.current_tile.bit0;
			mytile.bit1 = sprite.current_tile.bit1;
		}
	}


	// Background read
	std::cout << sizeof(Background_Load_Data) << std::endl;
	std::vector<Background_Load_Data> bg;
	std::ifstream in2("./assets/runtime_bg",std::ios::binary);
	read_chunk<Background_Load_Data>(in2,"bkg1",&bg);

	//Item Location Read
	std::cout << sizeof(Item_Location_Load_Data) << std::endl;
	std::vector<Item_Location_Load_Data> items;
	std::ifstream in3("./assets/runtime_itemlocation",std::ios::binary);
	read_chunk<Item_Location_Load_Data>(in3,"iloc",&items);


	// Setting up palette table
	for (auto i = 0; i < (int)palette_tables.size();i++){
		ppu.palette_table[i] = palette_tables[i];
	}

	// For sprites

	// Setting up tile table and the map to palette/tile from sprite type
	for (auto i = 0; i < (int)sprites_load.size();i++){
		ppu.tile_table[i] = sprites_load[i].current_tile;
		auto type = Sprite_Type(sprites_load[i].type);
		type_tile_map[type] = i;
		type_palette_map[type] = sprites_load[i].idx_to_pallete;
	}

	// For background

	auto background_load = bg[0];
	auto bg_tile_offset = 254;
	//Last two element in tile table stores bg tile
	for(auto i = 0; i < background_load.ntiles; i++){
		ppu.tile_table[i + bg_tile_offset] = background_load.tiles[i];
	}

	// The last palette used in bg
	uint16_t palette_idx = palette_tables.size() - 1;

	// Set up ppu.background
	for(uint16_t i = 0; i < PPU466::BackgroundHeight; i++){
		for(uint16_t j = 0; j < PPU466::BackgroundWidth; j++){

			uint16_t tile_idx = background_load.tile_idx_table[i][j] + bg_tile_offset;

			uint16_t config = 0;

			config = tile_idx | palette_idx << 8;

			ppu.background[i * PPU466::BackgroundWidth + j] = config;
		}
	}

	

	// transparent tile? maybe
	ppu.tile_table[100].bit0 = {
		0b00000000,
		0b00000000,
		0b00000000,
		0b00000000,
		0b00000000,
		0b00000000,
		0b00000000,
		0b00000000,
	};
	ppu.tile_table[100].bit1 = {
		0b00000000,
		0b00000000,
		0b00000000,
		0b00000000,
		0b00000000,
		0b00000000,
		0b00000000,
		0b00000000,
	};
	ppu.tile_table[32].bit0 = mytile.bit0;
	ppu.tile_table[32].bit1 = mytile.bit1;


	// Setting up sprites and items? 

	for (auto i = 0; i < 64; i++){
		sprite_infos.emplace_back(new SpriteInfoExtra(i,false,Main));
	}

	// items:
	/**
	 Sprite[0]: player sprite

	 Sprite[54] - [56] -> apple used for jump higher
	 Sprite[57] - [60] -> battery used for move faster
	 Sprite[61] - [62] -> Portal used to teleport
	 Sprite[63] -> Bomb?
	*/

	uint8_t offset = 54;

	for (uint16_t i = 0; i < items.size(); i++){
		auto type = Sprite_Type(items[i].type);
		ppu.sprites[offset+i].index = type_tile_map[type];
		uint8_t attr = 0;
		attr = attr | type_palette_map[type];
		ppu.sprites[offset+i].attributes = attr;
		sprite_infos[offset+i]->is_item = true;
		sprite_infos[offset+i]->loc_x = items[i].x;
		sprite_infos[offset+i]->loc_y = items[i].y;
		sprite_infos[offset+i]->type = type;
	}

	//Player sprite
	auto type_main = Main;
	ppu.sprites[0].index = type_tile_map[type_main];
	uint8_t attr = 0;
	attr = attr | type_palette_map[type_main];
	ppu.sprites[0].attributes = attr;
	sprite_infos[0]->is_item = false;

	//Other moving sprites?

}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_LEFT) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_RIGHT) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_UP) {
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_DOWN) {
			down.downs += 1;
			down.pressed = true;
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_LEFT) {
			left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_RIGHT) {
			right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_UP) {
			up.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_DOWN) {
			down.pressed = false;
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {

	// If we wan't to keep the player always at a fixed location of the screen
	// We need to move the screen in the rreverse direction?

	constexpr float PlayerSpeed_x = 30.0f;
	// if (left.pressed) player_at.x -= PlayerSpeed * elapsed;
	// if (right.pressed) player_at.x += PlayerSpeed * elapsed;
	// if (down.pressed) player_at.y -= PlayerSpeed * elapsed;
	// if (up.pressed) player_at.y += PlayerSpeed * elapsed;

	// bt_at means where does the (0,0) of the bg

	if (left.pressed) bg_at.x += PlayerSpeed_x * player_info.speed_up * elapsed;
	if (right.pressed) bg_at.x -= PlayerSpeed_x * player_info.speed_up * elapsed;
	if (down.pressed) bg_at.y += PlayerSpeed * player_info.speed_up * elapsed;
	if (up.pressed) bg_at.y -= PlayerSpeed * player_info.speed_up * elapsed;

	// For items to stay still, they need to, move how?

	if (left.pressed) real_position.x -= PlayerSpeed_x * player_info.speed_up * elapsed;
	if (right.pressed) real_position.x += PlayerSpeed_x * player_info.speed_up * elapsed;
	if (down.pressed) real_position.y -= PlayerSpeed * player_info.speed_up * elapsed;
	if (up.pressed) real_position.y += PlayerSpeed * player_info.speed_up * elapsed;


	// if (real_position.x < 0)
	// 	real_position.x += 512;
	// else if(real_position.x > 512)
	// 	real_position.x -=512;

	// if (real_position.y < 0)
	// 	real_position.y += 480;
	// else if(real_position.y > 480)
	// 	real_position.y -=480;

	update_timer(elapsed);

	crop_real_position();

	handle_items();

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//--- set ppu state based on game state ---

	//background color will be some hsv-like fade:
	ppu.background_color = glm::u8vec4(
		std::min(255,std::max(0,int32_t(255 * 0.5f * (0.5f + std::sin( 2.0f * M_PI * (background_fade + 0.0f / 3.0f) ) ) ))),
		std::min(255,std::max(0,int32_t(255 * 0.5f * (0.5f + std::sin( 2.0f * M_PI * (background_fade + 1.0f / 3.0f) ) ) ))),
		std::min(255,std::max(0,int32_t(255 * 0.5f * (0.5f + std::sin( 2.0f * M_PI * (background_fade + 2.0f / 3.0f) ) ) ))),
		0xff
	);

	// the player always at the center?

	//background scroll:
	ppu.background_position.x = int32_t(bg_at.x);
	ppu.background_position.y = int32_t(bg_at.y);

	//player sprite:
	ppu.sprites[0].x = ppu.ScreenWidth/2;
	ppu.sprites[0].y = ppu.ScreenHeight/2;
	// ppu.sprites[0].index = 32;
	// ppu.sprites[0].attributes = 3;






	//--- actually draw ---
	ppu.draw(drawable_size);
}



bool  PlayMode::item_inside_screen(SpriteInfoExtra * item){
	auto position_upper_range_x = real_position.x + 256;
	if (position_upper_range_x > 512)
		position_upper_range_x -=512;

	auto position_upper_range_y = real_position.y + 240;
	if (position_upper_range_y > 480)
		position_upper_range_y -=480;

	bool x_within = false;
	bool y_within = false;

	if(position_upper_range_x < real_position.x){
		if((item->loc_x > real_position.x) || (item->loc_x < position_upper_range_x))
			x_within = true;
	}else{
		if((item->loc_x > real_position.x) && (item->loc_x < position_upper_range_x))
			x_within = true;
	}

	if(position_upper_range_y < real_position.y){
		if((item->loc_y > real_position.y) || (item->loc_y < position_upper_range_y))
			y_within = true;
	}else{
		if((item->loc_y > real_position.y) && (item->loc_y < position_upper_range_y))
			y_within = true;
	}



	if (x_within && y_within)
		return true;
	else
		return false;
	
}

void PlayMode::crop_real_position(){
	if (real_position.x < 0)
		real_position.x += 512;
	else if(real_position.x > 512)
		real_position.x -=512;

	if (real_position.y < 0)
		real_position.y += 480;
	else if(real_position.y > 480)
		real_position.y -=480;
}


void PlayMode::handle_items(){

	// Used by portal recalculation
	bool break_flag = false;
	// Showing items..
	// Handling buff/teleport
	for(auto i = 54; i < 64; i++){

		if(break_flag){
			break;
		}

		auto item_sprite = sprite_infos[i];



		if (item_sprite->is_invisible == true){
			continue;
		}
		

		// if(item_inside_screen(item_sprite)){
		// 	ppu.sprites[i].x = item_sprite.loc_x - real_position.x;
		// 	ppu.sprites[i].y = item_sprite.loc_y - real_position.y;		
		// }
		if(item_inside_screen(item_sprite)){
			auto test_x = item_sprite->loc_x - real_position.x;
			if (test_x < 0)
				test_x += 512;
			auto test_y = item_sprite->loc_y - real_position.y;
			if (test_y < 0)
				test_y += 480;

			// If character hit the item?

			auto tmp_x = (int)test_x - (int)ppu.ScreenWidth / 2;
			auto tmp_y = (int)test_y - (int)ppu.ScreenHeight / 2;

			// If it is disabled
			if (item_sprite->is_disabled){
				//display the item. Don't care the hit
				ppu.sprites[i].x = test_x;
				ppu.sprites[i].y = test_y;
				ppu.sprites[i].attributes &= 0x7F;
			}
			// If hit?
			else if ((tmp_x < 2 && tmp_x > -2) && (tmp_y < 2 && tmp_y > -2)){
				auto item_type = item_sprite->type;

				switch (item_type)
				{
				case Apple:
					player_info.speed_up = 2.0;
					player_info.speed_up_time_left = 20.0;
					sprite_infos[i]->is_invisible = true;
					ppu.sprites[i].attributes |= 0x80;
					break;
				case Battery:
					player_info.jump_up = 3.0;
					player_info.jump_up_time_left = 20.0;
					sprite_infos[i]->is_invisible = true;
					ppu.sprites[i].attributes |= 0x80;
					break;
				case Bomb:
					player_info.has_bomb = true;
					sprite_infos[i]->is_invisible = true;
					ppu.sprites[i].attributes |= 0x80;
					break;
				case Portal:
				{
					// Need to update location
					auto other_portal = i==61? sprite_infos[62] : sprite_infos[61];
					auto this_portal = item_sprite;

					int moving_distance_x = (int)other_portal->loc_x - (int)this_portal->loc_x;
					int moving_distance_y = (int)other_portal->loc_y - (int)this_portal->loc_y;

					bg_at.x -= moving_distance_x;
					bg_at.y -= moving_distance_y;

					real_position.x += moving_distance_x;
					real_position.y += moving_distance_y;

					other_portal->is_disabled = true;
					other_portal->disable_timer = 5.0;

					crop_real_position();

					// Seems like really bad code
					handle_items();

					break_flag = true;

					// Need to recompute all object location.
					break;
				}
				case Main:
					break;
				case Mouse_Boss:
					break;
				case Mouse_Normal:
					break;
				default:
					break;
				}
			}else{
				ppu.sprites[i].x = test_x;
				ppu.sprites[i].y = test_y;
				ppu.sprites[i].attributes &= 0x7F;
			}



		}else{
			ppu.sprites[i].attributes |= 0x80;
		}
	}
}


void PlayMode::update_timer(float elapsed){
	if(player_info.jump_up_time_left > 0.0){
		player_info.jump_up_time_left -= elapsed;
		if(player_info.jump_up_time_left < 0.0){
			player_info.jump_up_time_left = -1.0;
			player_info.jump_up = 1.0;
		}
	}

	if(player_info.speed_up_time_left > 0.0){
		player_info.speed_up_time_left -= elapsed;
		if(player_info.speed_up_time_left < 0.0){
			player_info.speed_up = 1.0;
			player_info.speed_up_time_left = -1.0;
		}
	}

	for (auto i = 61; i < 63; i++){
		auto portal = sprite_infos[i];
		if(portal->is_disabled){
			if (portal->disable_timer > 0.0){
				portal->disable_timer -= elapsed;
				if(portal->disable_timer < 0.0){
					portal->is_disabled = false;
					portal->disable_timer = -1.0;
				}
			}
		}
	}

}