#include "PPU466.hpp"
#include "Mode.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>
#include <map>

enum Sprite_Type{
	Main,
	Mouse_Normal,
	Mouse_Boss,

	Portal,
	Apple,
	Battery,
	Bomb
};


class SpriteInfoExtra{
	public:
		uint16_t sprite_idx; // idx to ppu.sprites[]
		bool is_item;
		glm::vec2 speed; // speed?
		bool is_invisible;
		bool is_insight;
		uint16_t loc_x;
		uint16_t loc_y;
		Sprite_Type type;

		// Disable check, used for portal, or other things that might have cds.
		bool is_disabled;
		float disable_timer;

		SpriteInfoExtra(uint16_t idx, bool is_item, Sprite_Type type ,glm::vec2 vec = glm::vec2(0.0),bool is_invisible = false){
			this->sprite_idx = idx;
			this->is_item = is_item;
			this->speed = vec;
			this->is_invisible = is_invisible;
			this->loc_x = 0;
			this->loc_y = 0;
			this->type = type;
			this->is_disabled = false;
			this->disable_timer = -1.0;
			this->is_insight = false;
		}
};


class MainSpriteInfoExtra{
	public:
		bool has_bomb;
		float speed_up;
		float jump_up;
		float speed_up_time_left;
		float jump_up_time_left;

		float current_velocity_y;
		float current_acceleration;

		float drop_speed;

		float jump_speed;
		float jump_time_left;

		// true = right fales = left
		bool face;

		uint8_t reverse_tile_idx;

	MainSpriteInfoExtra(){
		this->has_bomb = false;
		this->speed_up = 1.0;
		this->jump_up = 1.0;
		this->speed_up_time_left = -1.0;
		this->jump_up_time_left = -1.0;
		this->jump_speed = 0.0;
		this->drop_speed = 50.0;
		this->jump_time_left = -1;
		this->face = true;
		this->reverse_tile_idx = 0;
	}
};


class BombSpriteInfoExtra{
	public:
		float explode_timer;
		float up_timer;
		float white_timer; 
		float v_y;
		float v_x;
		bool in_flight;
		glm::vec2 position;

		BombSpriteInfoExtra(){
			this->explode_timer = 2.0;
			this->up_timer = 1.0;
			this->v_x = 0.0;
			this->v_y = 45;
			this->in_flight = false;
			this->white_timer = 2.0;
			this->position = glm::vec2(0,0);
		}
};

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up, space;

	//some weird background animation:
	float background_fade = 0.0f;

	//player position:
	glm::vec2 player_at = glm::vec2(ppu.ScreenWidth / 2, ppu.ScreenHeight/2);

	glm::vec2 bg_at = glm::vec2(0,0);

	glm::vec2 real_position = glm::vec2(0,0);

	//----- drawing handled by PPU466 -----

	PPU466 ppu;


	// Keep track of the map of sprite-type---tile idx
	std::map<Sprite_Type,uint8_t> type_tile_map;

	// Keep track of the map of sprite-type---palette idx
	std::map<Sprite_Type,uint8_t> type_palette_map;

	std::vector<SpriteInfoExtra *> sprite_infos;
	MainSpriteInfoExtra player_info;
	BombSpriteInfoExtra bomb_info;

	std::array< uint16_t, PPU466::BackgroundWidth * PPU466::BackgroundHeight > background_white;
	std::array< uint16_t, PPU466::BackgroundWidth * PPU466::BackgroundHeight > background_back;
	std::array< uint16_t, PPU466::BackgroundWidth * PPU466::BackgroundHeight > background_text_win;
	std::array< uint16_t, PPU466::BackgroundWidth * PPU466::BackgroundHeight > background_text_lose;

	bool item_inside_screen(SpriteInfoExtra * item);

	// Make real pos always within 512,480
	void crop_real_position();


	void handle_items();

	void update_timer(float);
	void update_jump_timer(float,bool);

	std::array<bool,4> check_wall();
	std::array<bool,4> check_wall_easy();
	std::array<bool,4> check_wall_fine_grained();
	std::array<bool,4> check_wall_easy_fine_grained();


	std::vector<bool> is_wall;

	void handle_bomb(uint8_t,float);

	// For update moving sprites other than main// rn only one
	void update_sprite_loc(float elapsed);


	void set_tile_to_transparent(uint8_t sprite_idx);

	
};

// Store whether it is solid
// Maybe store the item location?
struct Background_Load_Data{
	uint8_t ntiles; // This should be 2(hardcoded)
	std::array<PPU466::Tile,2> tiles;
	uint8_t tile_idx_table[60][64];
	uint8_t solid_table[60][64];
	uint8_t palette_idx;
};


struct Background_Text_Data{
	uint8_t ntiles; // This should be 2(hardcoded)
	std::array<PPU466::Tile,2> tiles;
	uint8_t tile_idx_table[60][64];
	uint8_t palette_idx;
};


struct Sprite_Load_Data{
	PPU466::Tile current_tile;
	uint8_t idx_to_pallete;
	uint8_t type;
};


// Store other information for a sprite
struct Sprite_Meta{
	Sprite_Type type;
	float speed;
};


struct __attribute__((__packed__)) Item_Location_Load_Data{
	uint16_t x;
	uint16_t y;
	uint8_t type;
};
