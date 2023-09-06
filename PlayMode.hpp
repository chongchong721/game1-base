#include "PPU466.hpp"
#include "Mode.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

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
	glm::vec2 player_at = glm::vec2(0.0f);

	//----- drawing handled by PPU466 -----

	PPU466 ppu;
};

// Store whether it is solid
// Maybe store the item location?
struct Background_Meta{
	std::vector<std::vector<bool>> solid_bool;

	Background_Meta();
};


struct Sprite_Load_Data{
	PPU466::Palette current_pallete;
	PPU466::Tile current_tile;
	uint8_t type;
};

enum Sprite_Type{
	Main,
	Mouse_Normal,
	Mouse_Boss,

	Portal,
	Apple,
	Battery,
	Bomb
};


// Store other information for a sprite
struct Sprite_Meta{
	Sprite_Type type;
	float speed;
};