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

	// Create a reverse tile for main sprite
	auto main_tile = ppu.tile_table[type_tile_map[Main]];

	// https://www.geeksforgeeks.org/write-an-efficient-c-program-to-reverse-bits-of-a-number/#
	auto reverse_bits = [](uint8_t num)->uint8_t{
		unsigned int count = sizeof(num) * 8 - 1;
    	unsigned int reverse_num = num;
 
    	num >>= 1;
		while (num) {
			reverse_num <<= 1;
			reverse_num |= num & 1;
			num >>= 1;
			count--;
		}
		reverse_num <<= count;
		return reverse_num;
	};

	PPU466::Tile reverse_main_tile;
	// Reverse every row of bit0 and bit1
	for (uint8_t i = 0; i < 8; i++){
		auto reverse_bit0 = reverse_bits(main_tile.bit0[i]);
		auto reverse_bit1 = reverse_bits(main_tile.bit1[i]);

		reverse_main_tile.bit0[i] = reverse_bit0;
		reverse_main_tile.bit1[i] = reverse_bit1;
	}

	// Set tile 99 to be this tile
	ppu.tile_table[99] = reverse_main_tile;
	player_info.reverse_tile_idx = 99;


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

	background_back = ppu.background;

	// Set up wall
	is_wall.clear();

	for(uint16_t i = 0; i < PPU466::BackgroundHeight; i++){
		for(uint16_t j = 0; j < PPU466::BackgroundWidth; j++){
			bool wall = background_load.solid_table[i][j];
			is_wall.push_back(wall);
		}
	}

	// Make an all white tile. palette[0][4] is white RGBA
	ppu.tile_table[98].bit0 = {
		0b11111111,
		0b11111111,
		0b11111111,
		0b11111111,
		0b11111111,
		0b11111111,
		0b11111111,
		0b11111111,
	};
	ppu.tile_table[98].bit1 = {
		0b11111111,
		0b11111111,
		0b11111111,
		0b11111111,
		0b11111111,
		0b11111111,
		0b11111111,
		0b11111111,
	};

	for(uint16_t i = 0; i < PPU466::BackgroundHeight; i++){
		for(uint16_t j = 0; j < PPU466::BackgroundWidth; j++){

			uint16_t tile_idx = 98;

			uint16_t config = 0;

			config = tile_idx | 0 << 8;

			background_white[i * PPU466::BackgroundWidth + j] = config;
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


	// Setting up sprites and items

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

	constexpr float PlayerSpeed = 30.0f;
	// if (left.pressed) player_at.x -= PlayerSpeed * elapsed;
	// if (right.pressed) player_at.x += PlayerSpeed * elapsed;
	// if (down.pressed) player_at.y -= PlayerSpeed * elapsed;
	// if (up.pressed) player_at.y += PlayerSpeed * elapsed;

	// bt_at means where does the (0,0) of the bg


	// l - r - up - bot
	auto check_result = check_wall_easy_fine_grained();



	// Not using physically based gravity here. Just use a constant speed for dropping


	// For jump, give it a constant speed for some time.
	// Handle jump

	// Timer needed before assigning speed, or check_result[3] will directly make speed 0
	update_jump_timer(elapsed,check_result[3]);

	if (up.pressed && !check_result[2]){
		// Not in jump state
		if (player_info.jump_speed == 0.0 && check_result[3]){
			player_info.jump_speed = 120.0;
			player_info.jump_time_left = 0.5;
		}
	}

	bg_at.y -= player_info.jump_speed * player_info.jump_up * elapsed;
	real_position.y += player_info.jump_speed * player_info.jump_up * elapsed;




	if (left.pressed && !check_result[0]){
		bg_at.x += PlayerSpeed * player_info.speed_up * elapsed;
		real_position.x -= PlayerSpeed * player_info.speed_up * elapsed;
		player_info.face = false;
	} 
	if (right.pressed && !check_result[1]){
		bg_at.x -= PlayerSpeed * player_info.speed_up * elapsed;
		real_position.x += PlayerSpeed * player_info.speed_up * elapsed;
		player_info.face = true;
	} 
	
	// Dropping
	if(!check_result[3]){
		bg_at.y += player_info.drop_speed * elapsed;
		real_position.y -= player_info.drop_speed * elapsed;
	}


	//Throw bomb?
	handle_bomb(down.pressed,elapsed);



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
	
	if (player_info.face){
		ppu.sprites[0].index = type_tile_map[Main];
	}else{
		ppu.sprites[0].index = player_info.reverse_tile_idx;
	}

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
			else if ((tmp_x < 8 && tmp_x > -8) && (tmp_y < 8 && tmp_y > -8)){
				auto item_type = item_sprite->type;

				switch (item_type)
				{
				case Apple:
					player_info.speed_up = 2.3;
					player_info.speed_up_time_left = 20.0;
					sprite_infos[i]->is_invisible = true;
					ppu.sprites[i].attributes |= 0x80;
					break;
				case Battery:
					player_info.jump_up = 1.5;
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

std::array<bool,4> PlayMode::check_wall(){

	// This test seems too strict

    float player_position_x = real_position.x + 128;
	float player_position_y = real_position.y + 120;

	if (player_position_x > 512)
		player_position_x-=512;
	if(player_position_y > 480)
		player_position_y-=480;

	// In most situation, will find 8 neighbor block to determine if there is wall
	

	// left-most -> player_position_x
	// bottom-most -> player_position_y
	// right-most -> player_position_x + 8
	// up-most -> player_position_y + 8

	float left_most = player_position_x;
	float bottom_most = player_position_y;
	float right_most = left_most + 8;
	float up_most = bottom_most + 8;

	if (right_most > 512) 
		right_most -= 512;
	if (up_most > 480)
		up_most -= 480;

	glm::ivec2 left_idx_bot((int)(left_most / 8),(int)(bottom_most/8));
	glm::ivec2 left_idx_up((int)(left_most / 8),(int)(up_most/8));
	glm::ivec2 right_idx_bot((int)(right_most / 8),(int)(bottom_most/8));
	glm::ivec2 right_idx_up((int)(right_most / 8),(int)(up_most/8));


	// If idx get to < 0, scroll it
	auto scroll = [](int idx, int limit)->int{
		if (idx < 0)
			return limit - 1;
		else if(idx >= limit)
			return 0;
		else 
			return idx;
	};

	glm::ivec2 left_test_idx_bot(scroll(left_idx_bot.x - 1,64),left_idx_bot.y);
	glm::ivec2 left_test_idx_up(scroll(left_idx_up.x - 1,64),left_idx_up.y);

	glm::ivec2 right_test_idx_bot(scroll(right_idx_bot.x + 1,64),right_idx_bot.y);
	glm::ivec2 right_test_idx_up(scroll(right_idx_up.x + 1,64),right_idx_up.y);

	glm::ivec2 top_test_idx_left(left_idx_up.x,scroll(left_idx_up.y + 1, 60));
	glm::ivec2 top_test_idx_right(right_idx_up.x,scroll(right_idx_up.y + 1, 60));

	glm::ivec2 bot_test_idx_left(left_idx_bot.x,scroll(left_idx_bot.y-1,60));
	glm::ivec2 bot_test_idx_right(right_idx_bot.x,scroll(right_idx_bot.y-1,60));

	
	// Test left side
	bool left_wall = is_wall[left_test_idx_bot.y * 64 + left_test_idx_bot.x] || is_wall[left_test_idx_up.y * 64 + left_test_idx_up.x];
	bool right_wall = is_wall[right_test_idx_bot.y * 64 + right_test_idx_bot.x] || is_wall[right_test_idx_up.y * 64 + right_test_idx_up.x];
	bool up_wall = is_wall[top_test_idx_left.y * 64 + top_test_idx_left.x] || is_wall[top_test_idx_right.y * 64 + top_test_idx_right.x];
	bool bot_wall = is_wall[bot_test_idx_left.y * 64 + bot_test_idx_left.x] || is_wall[bot_test_idx_right.y * 64 + bot_test_idx_right.x];

	std::array<bool,4> arr;

	arr[0] = left_wall;
	arr[1] = right_wall;
	arr[2] = up_wall;
	arr[3] = bot_wall;

	return arr;

}



std::array<bool,4> PlayMode::check_wall_easy(){

	// This test seems too strict

    float player_position_x = real_position.x + 128;
	float player_position_y = real_position.y + 120;

	if (player_position_x > 512)
		player_position_x-=512;
	if(player_position_y > 480)
		player_position_y-=480;

	// Easy case, just find four neighbors
	glm::ivec2 character_idx((int)(player_position_x / 8),(int)(player_position_y / 8));


	// If idx get to < 0, scroll it
	auto scroll = [](int idx, int limit)->int{
		if (idx < 0)
			return limit - 1;
		else if(idx >= limit)
			return 0;
		else 
			return idx;
	};

	glm::ivec2 left_test_idx(scroll(character_idx.x - 1,64),character_idx.y);


	glm::ivec2 right_test_idx(scroll(character_idx.x + 1,64),character_idx.y);


	glm::ivec2 top_test_idx(character_idx.x,scroll(character_idx.y + 1, 60));


	glm::ivec2 bot_test_idx(character_idx.x,scroll(character_idx.y-1,60));
	

		
	// Test left side
	bool left_wall = is_wall[left_test_idx.y * 64 + left_test_idx.x]; 
	bool right_wall = is_wall[right_test_idx.y * 64 + right_test_idx.x]; 
	bool up_wall = is_wall[top_test_idx.y * 64 + top_test_idx.x]; 
	bool bot_wall = is_wall[bot_test_idx.y * 64 + bot_test_idx.x]; 

	std::array<bool,4> arr;

	arr[0] = left_wall;
	arr[1] = right_wall;
	arr[2] = up_wall;
	arr[3] = bot_wall;

	return arr;

}


std::array<bool,4> PlayMode::check_wall_fine_grained(){

	// This test seems too strict

    float player_position_x = real_position.x + 128;
	float player_position_y = real_position.y + 120;

	if (player_position_x > 512)
		player_position_x-=512;
	if(player_position_y > 480)
		player_position_y-=480;


	float left_most = player_position_x;
	float bottom_most = player_position_y;
	float right_most = left_most + 8;
	float up_most = bottom_most + 8;

	if (right_most > 512) 
		right_most -= 512;
	if (up_most > 480)
		up_most -= 480;

	glm::vec2 left_idx_bot(left_most,bottom_most);
	glm::vec2 left_idx_up(left_most ,up_most);
	glm::vec2 right_idx_bot(right_most,bottom_most);
	glm::vec2 right_idx_up(right_most,up_most);

	// If idx get to < 0, scroll it
	auto scroll = [](float idx ,float limit)->float{
		if (idx < 0)
			return limit + idx;
		else if(idx >= limit)
			return idx - limit;
		else 
			return idx;
	};

	glm::vec2 left_test_idx_bot(scroll(left_idx_bot.x - 1,512),left_idx_bot.y);
	glm::vec2 left_test_idx_up(scroll(left_idx_up.x - 1,512),left_idx_up.y);

	glm::vec2 right_test_idx_bot(scroll(right_idx_bot.x + 1,512),right_idx_bot.y);
	glm::vec2 right_test_idx_up(scroll(right_idx_up.x + 1,512),right_idx_up.y);

	glm::vec2 top_test_idx_left(left_idx_up.x,scroll(left_idx_up.y + 1, 480));
	glm::vec2 top_test_idx_right(right_idx_up.x,scroll(right_idx_up.y + 1, 480));

	glm::vec2 bot_test_idx_left(left_idx_bot.x,scroll(left_idx_bot.y-1,480));
	glm::vec2 bot_test_idx_right(right_idx_bot.x,scroll(right_idx_bot.y-1,480));
	

		
	bool left_wall = is_wall[(int)(left_test_idx_bot.y/8) * 64 + (int)(left_test_idx_bot.x/8)] || is_wall[(int)(left_test_idx_up.y/8) * 64 + (int)(left_test_idx_up.x/8)];
	bool right_wall = is_wall[(int)(right_test_idx_bot.y/8) * 64 + (int)(right_test_idx_bot.x/8)] || is_wall[(int)(right_test_idx_up.y/8) * 64 + (int)(right_test_idx_up.x/8)];
	bool up_wall = is_wall[(int)(top_test_idx_left.y/8) * 64 + (int)(top_test_idx_left.x/8)] || is_wall[(int)(top_test_idx_right.y /8)* 64 + (int)(top_test_idx_right.x/8)];
	bool bot_wall = is_wall[(int)(bot_test_idx_left.y/8) * 64 + (int)(bot_test_idx_left.x/8)] || is_wall[(int)(bot_test_idx_right.y/8) * 64 + (int)(bot_test_idx_right.x/8)];

	std::array<bool,4> arr;

	arr[0] = left_wall;
	arr[1] = right_wall;
	arr[2] = up_wall;
	arr[3] = bot_wall;

	return arr;

}



std::array<bool,4> PlayMode::check_wall_easy_fine_grained(){

	// This test seems too strict

    float player_position_x = real_position.x + 128;
	float player_position_y = real_position.y + 120;

	if (player_position_x > 512)
		player_position_x-=512;
	if(player_position_y > 480)
		player_position_y-=480;


	float middle_x = player_position_x + 4;
	float middle_y = player_position_y + 4;

	if (middle_x > 512) 
		middle_x -= 512;
	if (middle_y > 480)
		middle_y -= 480;

	// If idx get to < 0, scroll it
	auto scroll = [](float idx ,float limit)->float{
		if (idx < 0)
			return limit + idx;
		else if(idx >= limit)
			return idx - limit;
		else 
			return idx;
	};


	glm::vec2 left_test_idx(scroll(middle_x - 8,512),middle_y);
	glm::vec2 right_test_idx(scroll(middle_x + 8,512),middle_y);
	glm::vec2 top_test_idx(middle_x,scroll(middle_y + 8,480));
	glm::vec2 bot_test_idx(middle_x,scroll(middle_y - 8, 480));


		
	bool left_wall = is_wall[(int)(left_test_idx.y/8) * 64 + (int)(left_test_idx.x/8)] ;
	bool right_wall = is_wall[(int)(right_test_idx.y/8) * 64 + (int)(right_test_idx.x/8)]; 
	bool up_wall = is_wall[(int)(top_test_idx.y/8) * 64 + (int)(top_test_idx.x/8)]; 
	bool bot_wall = is_wall[(int)(bot_test_idx.y/8) * 64 + (int)(bot_test_idx.x/8)]; 

	std::array<bool,4> arr;

	arr[0] = left_wall;
	arr[1] = right_wall;
	arr[2] = up_wall;
	arr[3] = bot_wall;

	return arr;

}



void PlayMode::update_jump_timer(float elapsed,bool bot_wall){
	if (bot_wall){
		player_info.jump_time_left = -1.0;
		player_info.jump_speed = 0.0;
		return;
	}

	if (player_info.jump_time_left > 0.0){
		player_info.jump_time_left -= elapsed;
		if(player_info.jump_time_left < 0.0){
			player_info.jump_speed = 0.0;
			player_info.jump_time_left = -1.0;
		}
	}
}

void PlayMode::handle_bomb(uint8_t pressed, float elapsed){
	if (bomb_info.in_flight){
		//update bomb flight path
		ppu.sprites[63].x += bomb_info.v_x * elapsed;
		ppu.sprites[63].y -= 30 * elapsed;

		if(bomb_info.up_timer > 0.0){
			ppu.sprites[63].y += bomb_info.v_y * elapsed;
		}



		bomb_info.up_timer -= elapsed;
		bomb_info.explode_timer -= elapsed;

		if(bomb_info.up_timer < 0.0){
			bomb_info.v_y = 0;
		}

		if(bomb_info.explode_timer < 0.0){
			ppu.background = background_white;
			bomb_info.white_timer -= elapsed;
			ppu.sprites[63].attributes |= 0x80;
		}

		if(bomb_info.white_timer < 0.0){
			ppu.background = background_back;
			bomb_info.in_flight = false;

		}


	}else{
		if (pressed == 0)
				return;
		else{
			if (player_info.has_bomb){
					//63 is the bomb sprite
					float v_x = player_info.face ? 45.0f : -45.0f;
					bomb_info.v_x = v_x;
					bomb_info.in_flight = true;
					ppu.sprites[63].x = PPU466::ScreenWidth / 2;
					ppu.sprites[63].y = PPU466::ScreenHeight / 2;
					ppu.sprites[63].attributes &= 0x7F;
					player_info.has_bomb = false;
				}else{
					return;
				}
	}
	
	}

}