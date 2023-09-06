from PIL import Image
import numpy as np
from enum import Enum
import sys

def if_palette_exist(palette_table,palette):
    for i in range(len(palette_table)):
        if np.array_equal(palette_table[i],palette):
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

    class Sprite_type(Enum):
        Main = 0
        Mouse_Normal = 1
        Mouse_Boss = 2
        Portal = 3
        Apple = 4
        Battery = 5
        Bomb = 6

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
        exist,palette_idx = if_palette_exist(palette_table,palette)
        if not exist:
            palette_table.append(palette)
            idx_to_palette_table.append(len(palette_table)-1)
        else:
            idx_to_palette_table.append(palette_idx)

        tile_table.append(idx_table)
        type_table.append(type_dict[path])


    
    return palette_table,tile_table,idx_to_palette_table,type_table

    
def process_background():

    ncolumn,nrow = 512,480
    ntile_column,ntile_row = 64,60

    im = Image.open("assets/bg.png")

    color_arr = np.array(im)

    # https://stackoverflow.com/questions/60539888/identify-the-color-values-of-an-image-with-a-color-palette-using-pil-pillow
    color_palette, counts = np.unique(color_arr.reshape(-1,4), axis=0, return_counts=1)

    if len(color_palette) < 4:
        new_palette = np.zeros((4,4))
        len_toadd = 4 - len(color_palette)
        for i in range(len_toadd):
            new_palette[i] = np.array([0,0,0,0])

        for i in range(len(color_palette)):
            new_palette[i + len_toadd] = color_palette[i]

    color_palette = new_palette

    # should have 64*60 elements, each of a (8,8,2)
    background_tile_table = []

    idx = np.zeros((8,8,2))

    for i in range(8):
        for j in range(8):
            pal_idx = find_idx_in_palette(color_palette,color_arr[i][j])
            bit0,bit1 = idx_to_bit(pal_idx)
            idx[i][j][0] = bit0
            idx[i][j][1] = bit1

    return color_palette,idx

def process_item_location():
    pass


def generate_sprites_runtime(palette_table,tile_table,idx_to_palette_table,type_dict):
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
                        
            



if __name__ == "__main__":
    #process_background()
    palette_table,tile_table,idx_to_palette_table,type_table = process_all_sprites()
    generate_sprites_runtime(palette_table,tile_table,idx_to_palette_table,type_table)

