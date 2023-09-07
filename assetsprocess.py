from PIL import Image
import numpy as np
from enum import Enum
import sys

class Sprite_type(Enum):
    Main = 0
    Mouse_Normal = 1
    Mouse_Boss = 2
    Portal = 3
    Apple = 4
    Battery = 5
    Bomb = 6


def int_to_bytes(num,bytelength):
    num = int(num)
    ret = num.to_bytes(bytelength,sys.byteorder)
    return ret
def string_to_bytes(mystr):
    str_bytes = bytearray(mystr,encoding='ascii')
    return str_bytes

def tile_to_bytes(tile):
    bit = [
        np.zeros(8,dtype=np.uint8),
        np.zeros(8,dtype=np.uint8)
    ]

    for nbit in range(2):
        # i is the index for row
        # Note that we read from top-left,but PPU needs bottom-left
        for i in range(8):
            # j is the index for columb
            for j in range(8):
                idx_image = 7 - i
                current_bit = tile[idx_image][j][nbit]
                bit_mask = current_bit << j
                bit[nbit][i] = bit[nbit][i] | bit_mask

    return bit

# Tell if a np array exist in a list
def if_exist_in_list(l,arr):
    for i in range(len(l)):
        if np.array_equal(l[i],arr):
            return True,i
        
    return False,-1



def find_idx_in_palette(color_palette, color):
    for i in range(4):
        if np.array_equal(color_palette[i],color):
            return i
    
    return -1

# Idx <= 3
def idx_to_bit(idx):
    # arr[0] -> bit0 arr[1]->bit1
    if idx == 0:
        return np.array([0,0],dtype=np.uint8)
    elif idx == 1:
        return np.array([1,0],dtype=np.uint8)
    elif idx == 2:
        return np.array([0,1],dtype=np.uint8)
    elif idx == 3:
        return np.array([1,1],dtype=np.uint8)
    else:
        return np.array([255,255],dtype=np.uint8)


## Return the palette and the tile
def process_sprite(png_path,type):

    im = Image.open(png_path)

    color_arr = np.array(im)

    # https://stackoverflow.com/questions/60539888/identify-the-color-values-of-an-image-with-a-color-palette-using-pil-pillow
    color_palette, counts = np.unique(color_arr.reshape(-1,4), axis=0, return_counts=1)

    idx = np.zeros((8,8,2),dtype=np.uint8)

    for i in range(8):
        for j in range(8):
            pal_idx = find_idx_in_palette(color_palette,color_arr[i][j])
            bit = idx_to_bit(pal_idx)
            idx[i][j][0] = bit[0]
            idx[i][j][1] = bit[1]

    return color_palette,idx


def process_all_sprites():
    paths = [
        "assets/apple.png",
        "assets/battery.png",
        "assets/bomenade.png",
        "assets/cat.png",
        "assets/mouse.png",
        "assets/mouse1.png",
        "assets/portal.png",
    ]

    type = Sprite_type

    type_dict = {}
    type_dict["assets/apple.png"] = type.Apple
    type_dict["assets/battery.png"] = type.Battery
    type_dict["assets/bomenade.png"] = type.Bomb
    type_dict["assets/cat.png"] = type.Main
    type_dict["assets/mouse.png"] = type.Mouse_Normal
    type_dict["assets/mouse1.png"] = type.Mouse_Boss
    type_dict["assets/portal.png"] = type.Portal


    palette_table = []
    tile_table = []
    idx_to_palette_table = []
    type_table = []


    for i in range(len(paths)):
        path = paths[i]
        palette,idx_table = process_sprite(path,type_dict[path])
        exist,palette_idx = if_exist_in_list(palette_table,palette)
        if not exist:
            palette_table.append(palette)
            idx_to_palette_table.append(len(palette_table)-1)
        else:
            idx_to_palette_table.append(palette_idx)

        tile_table.append(idx_table)
        type_table.append(type_dict[path])


    
    return palette_table,tile_table,idx_to_palette_table,type_table

    
def process_background():
    ## Hard-coded things
    solid_palette = np.array([84,33,6,255],dtype=np.uint8)

    ## We know this because we paint the bg
    nunique_tile = 2

    ncolumn,nrow = 512,480
    ntile_column,ntile_row = 64,60

    im = Image.open("assets/bg.png")

    color_arr = np.array(im)

    # https://stackoverflow.com/questions/60539888/identify-the-color-values-of-an-image-with-a-color-palette-using-pil-pillow
    color_palette, counts = np.unique(color_arr.reshape(-1,4), axis=0, return_counts=1)

    # Make color palette
    if len(color_palette) < 4:
        new_palette = np.zeros((4,4),dtype=np.uint8)
        len_toadd = 4 - len(color_palette)
        for i in range(len_toadd):
            new_palette[i] = np.array([0,0,0,0],dtype=np.uint8)

        for i in range(len(color_palette)):
            new_palette[i + len_toadd] = color_palette[i]

    color_palette = new_palette

    # should have 2 elements, each of a (8,8,2)
    background_tile_table = []


    break_flag = False
    for row in range(ntile_row):
        for col in range(ntile_column):
            idx = np.zeros((8,8,2),dtype=np.uint8)
            for i in range(8):
                idx_i = i + row * 8
                for j in range(8):
                    idx_j = j + col * 8
                    pal_idx = find_idx_in_palette(color_palette,color_arr[idx_i][idx_j])
                    bit0,bit1 = idx_to_bit(pal_idx)
                    idx[i][j][0] = bit0
                    idx[i][j][1] = bit1
            exist,list_idx =  if_exist_in_list(background_tile_table,idx)
            if not exist:
                background_tile_table.append(idx)
                if len(background_tile_table) == nunique_tile:
                    break_flag = True
                    break
            else:
                continue
        
        if break_flag:
            break

    ## Generate a map for each 8*8 pixel block to one of the tile
    tile_bg_map =  np.zeros((60,64),dtype=np.uint8)
    for row in range(ntile_row):
            for col in range(ntile_column):
                idx = np.zeros((8,8,2),dtype=np.uint8)
                for i in range(8):
                    idx_i = i + row * 8
                    for j in range(8):
                        idx_j = j + col * 8
                        pal_idx = find_idx_in_palette(color_palette,color_arr[idx_i][idx_j])
                        bit0,bit1 = idx_to_bit(pal_idx)
                        idx[i][j][0] = bit0
                        idx[i][j][1] = bit1

                exist, list_idx = if_exist_in_list(background_tile_table,idx)
                if not exist:
                    # 191 311? blue line
                    print("Something is wrong")
                else:
                    tile_bg_map[row][col] = list_idx

    exist, solid_idx = if_exist_in_list(color_palette,solid_palette)




    def get_tile_palette_idx(tile,x=0,y=0):
        bit0 = tile[x][y][0]
        bit1 = tile[x][y][1]
        return bit1 << 1 | bit0
    
    bg_tile_table_solid_idx = 255
    for i in range(nunique_tile):
        tmpidx = get_tile_palette_idx(background_tile_table[i])
        if tmpidx == solid_idx:
            bg_tile_table_solid_idx = i
            break

    ## Generate solid bits?
    # Solid(60,64,dtype = uint8)
    solid = np.zeros((60,64),dtype=np.uint8)
    for row in range(ntile_row):
            for col in range(ntile_column):
                tile_idx = tile_bg_map[row][col]
                if tile_idx == bg_tile_table_solid_idx:
                    solid[row][col] = 1

    print(np.array_equal(tile_bg_map,solid))
    
    return color_palette,background_tile_table,tile_bg_map,solid

def process_item_location():
    pass


def generate_sprites_runtime(palette_table,tile_table,idx_to_palette_table,type_table):
    # Try to generate the first one
    # First 4 * 4 bytes store palette data
    # Next 8 * 8 / 8 * 2 bytes store tile data
    # Then 1 byte store type data
    # Possibly store location data
    with open("assets/runtime_sprite","wb") as f:
        magic = "spr0"

        str_bytes = bytearray(magic,encoding='ascii')

        size = int(33)

        size_bytes = size.to_bytes(4,sys.byteorder)

        f.write(str_bytes)
        f.write(size_bytes)

        for n in range(1):
            palette = palette_table[idx_to_palette_table[n]]

            palette_bytes = bytearray(palette)

            tile = tile_table[n]

            bit = [
                np.zeros(8,dtype=np.uint8),
                np.zeros(8,dtype=np.uint8)
            ]

            for nbit in range(2):
                # i is the index for row
                # Note that we read from top-left,but PPU needs bottom-left
                for i in range(8):
                    # j is the index for columb
                    for j in range(8):
                        idx_image = 7 - i
                        current_bit = tile[idx_image][j][nbit]
                        bit_mask = current_bit << j
                        bit[nbit][i] = bit[nbit][i] | bit_mask

            bit0_bytes = bytearray(bit[0])
            print(bit0_bytes)
            bit1_bytes = bytearray(bit[1])

            type = type_table[n].value
            type = int(type)

            type_bytes = type.to_bytes(1,sys.byteorder)

            print(type_bytes)

            f.write(palette_bytes)
            f.write(bit0_bytes)
            f.write(bit1_bytes)
            f.write(type_bytes)
                        

# For both background and sprites
def generate_palettes_and_tiles(bginfo,spriteinfo):
    palette_bg, bg_tiles, tile_bg_map,solid = bginfo[0],bginfo[1],bginfo[2],bginfo[3]
    palette_table_sprite,tile_table,idx_to_palette_table,type_table = spriteinfo[0],spriteinfo[1],spriteinfo[2],spriteinfo[3]

    palette_table = palette_table_sprite
    palette_table.append(palette_bg)

    # generate palette tables
    # palette for bg is at the end of the palette table
    with open("assets/runtime_palette","wb") as f:
        magic = "pal0"
        size = int(16 * len(palette_table))

        str_bytes = bytearray(magic,encoding='ascii')
        size_bytes = size.to_bytes(4,sys.byteorder)

        f.write(str_bytes)
        f.write(size_bytes)

        for i in range(len(palette_table)):
            palette = palette_table[i]
            palette_bytes = bytearray(palette)
            f.write(palette_bytes)

    # generate sprites informations
    # 16 bytes of tile data
    # 1 byte of palette table idx
    # 1 byte of type
    # Possibly other??
    with open("assets/runtime_sprite","wb") as f:
        magic = "spr0"
        size = int(18 * len(tile_table))
        str_bytes = bytearray(magic,encoding='ascii')
        size_bytes = size.to_bytes(4,sys.byteorder)

        f.write(str_bytes)
        f.write(size_bytes)

        for n in range(len(tile_table)):
            tile = tile_table[n]

            bit = tile_to_bytes(tile)

            bit0_bytes = bytearray(bit[0])
            #print(bit0_bytes)
            bit1_bytes = bytearray(bit[1])

            palette_idx = idx_to_palette_table[n]
            palette_bytes = int_to_bytes(palette_idx,1)

            type = type_table[n].value

            type_bytes = int_to_bytes(type,1)

            f.write(bit0_bytes)
            f.write(bit1_bytes)
            f.write(palette_bytes)
            f.write(type_bytes)

    #### Generate bg informations

    # 1 byte of number of tiles
    # n * 16 bytes of tiles
    # 60 * 64 bytes of tile_idx(0 for row in range(60):
    # 60 * 64 bytes of solid bool
    # 1 byte of palette idx

    with open("assets/runtime_bg","wb") as f:

        #0. get number of tiles
        ntiles = len(bg_tiles)

        magic = "bkg1"
        size = int(1 + 16 * ntiles + 60 * 64 * 2 + 1)

        f.write(string_to_bytes(magic))
        f.write(int_to_bytes(size,4))
        f.write(int_to_bytes(ntiles,1))

        #1. write n tiles
        for i in range(ntiles):
            tile = bg_tiles[i]
            bit = tile_to_bytes(tile)
            bit0_bytes = bytearray(bit[0])
            #print(bit0_bytes)
            bit1_bytes = bytearray(bit[1])

            f.write(bit0_bytes)
            f.write(bit1_bytes)

        #resoncstruct tile info and solid info from bottom left
        tile_info = np.zeros((60,64),dtype=np.uint8)
        solid_info = np.zeros((60,64),dtype=np.uint8)

        for row in range(60):
            row_reverse = 59 - row
            tile_info[row] = tile_bg_map[row_reverse]
            solid_info[row] = solid[row_reverse]
    
        #2. write tile_idx and solid_bool
        tile_idx_bytes = bytearray(tile_info)
        solid_bytes = bytearray(solid_info)
        f.write(tile_idx_bytes)
        f.write(solid_bytes)

        #3. write palette index 

        pal_idx = len(palette_table) - 1
        f.write(int_to_bytes(pal_idx,1))


        ## Provide item locations

        with open("assets/runtime_itemlocation","wb") as f:

            # Location: (uint16t,uint16t) 4 bytes  in (x,y) form
            # item: uint8t 1 bytes

            magic = "iloc"
            size = int(5 * 10)

            f.write(string_to_bytes(magic))
            f.write(int_to_bytes(size,4))

            location =[
                np.array([184,104],dtype=np.uint16), 
                np.array([112,320],dtype=np.uint16), 
                np.array([448,368],dtype=np.uint16), 
                np.array([176,104],dtype=np.uint16), 
                np.array([256,344],dtype=np.uint16), 
                np.array([72,416],dtype=np.uint16),
                np.array([48,216],dtype=np.uint16),
                np.array([120,104],dtype=np.uint16), 
                np.array([336,176],dtype=np.uint16),
                np.array([464,304],dtype=np.uint16),
            ] 

            # Flip to adapt PPU
            for i in range(len(location)):
                location[i][1] = 480 - location[i][1]

            enum = Sprite_type

            type = [
                enum.Apple,
                enum.Apple,
                enum.Apple,
                enum.Battery,
                enum.Battery,
                enum.Battery,
                enum.Battery,
                enum.Portal,
                enum.Portal,
                enum.Bomb,
            ]

            for i in range((len(location))):
                loc_bytes = bytearray(location[i])
                type_val = type[i].value
                type_bytes = int_to_bytes(type_val,1)

                f.write(loc_bytes)
                f.write(type_bytes)

        



if __name__ == "__main__":
    color_palette,background_tile_table,tile_bg_map,solid =  process_background()
    palette_table,tile_table,idx_to_palette_table,type_table = process_all_sprites()
    #generate_sprites_runtime(palette_table,tile_table,idx_to_palette_table,type_table)
    generate_palettes_and_tiles([color_palette,background_tile_table,tile_bg_map,solid],[palette_table,tile_table,idx_to_palette_table,type_table])
