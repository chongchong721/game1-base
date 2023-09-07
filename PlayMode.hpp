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

		float g = 10.0;

	MainSpriteInfoExtra(){
		this->has_bomb = false;
		this->speed_up = 1.0;
		this->jump_up = 1.0;
		this->speed_up_time_left = -1.0;
		this->jump_up_time_left = -1.0;
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
	} left, right, down, up;

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


	bool item_inside_screen(SpriteInfoExtra * item);

	// Make real pos always within 512,480
	void crop_real_position();


	void handle_items();

	void update_timer(float);

	std::array<bool,4> check_wall();
	std::array<bool,4> check_wall_easy();
	std::array<bool,4> check_wall_fine_grained();
	std::array<bool,4> check_wall_easy_fine_grained();


	std::vector<bool> is_wall;

	
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
