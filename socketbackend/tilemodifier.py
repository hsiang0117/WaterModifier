import os
import struct
import threading
from math import floor

import numpy as np
from scipy.ndimage import binary_closing, binary_opening
from scipy.signal import convolve2d

unsigned_int_format = '<I'
unsigned_char_format = 'B'


# convert mask from bool list to byte list.
def analyse_mask(mask):
    for i in range(len(mask)):
        for j in range(len(mask[0])):
            if mask[i][j]:
                mask[i][j] = b'\xff'
            else:
                mask[i][j] = b'\x00'
    return mask


# Seek for watermask extension.
# Returns the position of watermask in the terrain file.
# Returns -1 if there's no watermask.
def get_watermask_pos(file_path):
    file = open(file_path, "rb")
    file_size = os.fstat(file.fileno()).st_size
    file.seek(88)
    vertex_count = file.read(4)
    vertex_count = struct.unpack(unsigned_int_format, vertex_count)[0]
    file.seek(2 * vertex_count * 3, 1)
    triangle_count = file.read(4)
    triangle_count = struct.unpack(unsigned_int_format, triangle_count)[0]
    indices_size = 2 if triangle_count < 65536 else 4
    file.seek(indices_size * triangle_count * 3, 1)
    west_vertex_count = file.read(4)
    west_vertex_count = struct.unpack(unsigned_int_format, west_vertex_count)[0]
    file.seek(2 * west_vertex_count, 1)
    south_vertex_count = file.read(4)
    south_vertex_count = struct.unpack(unsigned_int_format, south_vertex_count)[0]
    file.seek(2 * south_vertex_count, 1)
    east_vertex_count = file.read(4)
    east_vertex_count = struct.unpack(unsigned_int_format, east_vertex_count)[0]
    file.seek(2 * east_vertex_count, 1)
    north_vertex_count = file.read(4)
    north_vertex_count = struct.unpack(unsigned_int_format, north_vertex_count)[0]
    file.seek(2 * north_vertex_count, 1)
    while file.tell() < file_size:
        extension_type = file.read(1)
        extension_type = struct.unpack(unsigned_char_format, extension_type)[0]
        if extension_type == 2:
            return file.tell()
        else:
            extension_length = file.read(4)
            extension_length = struct.unpack(unsigned_int_format, extension_length)[0]
            file.seek(extension_length, 1)
    return -1


# Read current watermask from terrain file.
def read_watermask(file_path, pos):
    file = open(file_path, 'rb')
    if pos == -1:
        return -1
    else:
        file.seek(pos)
        watermask_length = file.read(4)
        watermask_length = struct.unpack(unsigned_int_format, watermask_length)[0]
        watermask_bytearray = file.read(watermask_length)
        return watermask_length, watermask_bytearray


# Get watermask bytearray that will be written back to the terrain file.
# This bytearray obtains by originate watermask from the terrain file and the segment result through an algorithm
# which considers the relative position of target tile and the area covered by segment result.
def get_new_watermask(file_path, mask, i, j, ortho_width, tile_size, offset, cover):
    pos = get_watermask_pos(file_path)
    new_mask = b''
    if pos == -1:
        for x in range(0, tile_size):
            for y in range(0, tile_size):
                if offset[1] <= x + i * tile_size <= offset[1] + ortho_width:
                    if offset[0] <= y + j * tile_size <= offset[0] + ortho_width:
                        new_mask += mask[x + i * tile_size - offset[1] - 1][y + j * tile_size - offset[0] - 1]
                    else:
                        new_mask += b'\x00'
                else:
                    new_mask += b'\x00'
    else:
        origin_length, origin_mask = read_watermask(file_path, pos)
        if origin_length == 1:
            for x in range(0, tile_size):
                for y in range(0, tile_size):
                    if offset[1] <= x + i * tile_size <= offset[1] + ortho_width:
                        if offset[0] <= y + j * tile_size <= offset[0] + ortho_width:
                            if cover == "Fill":
                                if mask[x + i * tile_size - offset[1]][
                                    y + j * tile_size - offset[0]] != b'\x00':
                                    new_mask += mask[x + i * tile_size - offset[1]][
                                        y + j * tile_size - offset[0]]
                                else:
                                    if origin_mask == 0:
                                        new_mask += b'\x00'
                                    else:
                                        new_mask += b'\xff'
                            else:
                                new_mask += mask[x + i * tile_size - offset[1] - 1][
                                    y + j * tile_size - offset[0] - 1]
                        else:
                            if origin_mask == 0:
                                new_mask += b'\x00'
                            else:
                                new_mask += b'\xff'
                    else:
                        if origin_mask == 0:
                            new_mask += b'\x00'
                        else:
                            new_mask += b'\xff'
        else:
            for x in range(0, tile_size):
                for y in range(0, tile_size):
                    if offset[1] <= x + i * tile_size <= offset[1] + ortho_width and offset[0] <= y + j * tile_size <= \
                            offset[0] + ortho_width:
                        if cover == "Fill":
                            if mask[x + i * tile_size - offset[1] - 1][
                                y + j * tile_size - offset[0] - 1] != b'\x00':
                                new_mask += mask[x + i * tile_size - offset[1] - 1][
                                    y + j * tile_size - offset[0] - 1]
                            else:
                                new_mask += struct.pack(unsigned_char_format, origin_mask[x * tile_size + y])
                        else:
                            new_mask += mask[x + i * tile_size - offset[1] - 1][
                                y + j * tile_size - offset[0] - 1]
                    else:
                        new_mask += struct.pack(unsigned_char_format, origin_mask[x * tile_size + y])

    return new_mask


# Consider a point (x,y) is inside a polygon or not by ray casting method.
# Poly is a list [(x0, y0), (x1, y1), ..., (xn-1, yn-1)] indicates a closed route.
def point_in_polygon(x, y, poly):
    inside = False
    n = len(poly)
    p1x, p1y = poly[0]
    for i in range(1, n + 1):
        p2x, p2y = poly[i % n]
        if min(p1y, p2y) < y <= max(p1y, p2y):
            xinters = (y - p1y) * (p2x - p1x) / (p2y - p1y + 1e-10) + p1x
            if x <= xinters:
                inside = not inside
        p1x, p1y = p2x, p2y
    return inside


# Do photoshop-like pen process.
# Manually select water area.
def pen_process(points, mask):
    for y in range(mask.shape[0]):
        for x in range(mask.shape[1]):
            if point_in_polygon(x + 0.5, y + 0.5, points):
                mask[y][x] = True
    return mask


# Called before written back to terrain file.
# Including opening, closing and convolution
def morphological_process(mask):
    opening_and_closing_filter = np.array([
        [1, 1, 1],
        [1, 1, 1],
        [1, 1, 1]
    ])

    convolution_filter = np.array([
        [0, 0, 0, 1 / 25, 0, 0, 0],
        [0, 0, 1 / 25, 1 / 25, 1 / 25, 0, 0],
        [0, 1 / 25, 1 / 25, 1 / 25, 1 / 25, 1 / 25, 0],
        [1 / 25, 1 / 25, 1 / 25, 1 / 25, 1 / 25, 1 / 25, 1 / 25],
        [0, 1 / 25, 1 / 25, 1 / 25, 1 / 25, 1 / 25, 0],
        [0, 0, 1 / 25, 1 / 25, 1 / 25, 0, 0],
        [0, 0, 0, 1 / 25, 0, 0, 0]
    ])

    byte_array = np.frombuffer(mask, dtype=np.uint8).reshape(256, 256)
    binary_image = (byte_array != 0).astype(bool)

    opened_image = binary_opening(
        binary_image,
        structure=opening_and_closing_filter,
        iterations=2
    )

    closed_image = binary_closing(
        opened_image,
        structure=opening_and_closing_filter,
        iterations=2,
        border_value=1
    )
    closed_image = np.where(closed_image, 0xff, 0x00).astype(np.uint8)

    convolved_image = convolve2d(closed_image, convolution_filter, mode='same', boundary='symm')
    convolved_image = convolve2d(convolved_image, convolution_filter, mode='same', boundary='symm')
    result = np.where(closed_image == 255, convolved_image, 0)
    result_array = np.asarray(result, dtype=int).flatten()

    output_array = b''
    for i in result_array:
        output_array += struct.pack("<B", i)
    return output_array


num_modified = 0


# Write watermask back to terrain file.
def write_back(file_path, new_mask):
    new_mask = morphological_process(new_mask)
    pos = get_watermask_pos(file_path)
    if pos == -1:
        file = open(file_path, 'ab')
        file.write(b'\x02\x00\x00\x01\x00')
        file.write(new_mask)
        file.close()
    else:
        file = open(file_path, 'rb+')
        data_before_water = bytearray(file.read(pos))
        data_remain = bytearray(file.read())
        watermask_length = data_remain[0:4]
        watermask_length = struct.unpack(unsigned_int_format, watermask_length)[0]
        if watermask_length == 1:
            data_remain[0:4] = b'\x00\x00\x01\x00'
            del data_remain[4]
            data_remain += new_mask
        else:
            del data_remain[4:]
            data_remain += new_mask
        file.close()

        # write back
        with open(file_path, 'wb') as file:
            data = data_before_water + data_remain
            file.write(data)
    global num_modified
    num_modified += 1
    print(file_path + " done")


# Modify watermask within a single tile with the new mask which come from get_watermask().
# If there's no watermask (input pos = -1) then add watermask to the terrain file.
def modify_watermask(file_path, mask, i, j, ortho_width, tile_size, offset, cover):
    new_mask = get_new_watermask(file_path, mask, i, j, ortho_width, tile_size, offset, cover)
    write_back(file_path, new_mask)


# Expand a mask by nearest interpolation.
# For example from 128*128 to 256*256.
def mask_interpolation(mask):
    expanded_mask = b''
    for i in range(256):
        for j in range(256):
            expanded_mask += struct.pack(unsigned_char_format, mask[floor(i / 2) * 128 + floor(j / 2)])
    return expanded_mask


def modify_child(parent_path, file_path, corner):
    parent_pos = get_watermask_pos(parent_path)
    parent_length, parent_mask = read_watermask(parent_path, parent_pos)
    origin_mask = b''
    if corner == 0:
        for i in range(128, 256):
            origin_mask += parent_mask[i * 256:i * 256 + 128]
    elif corner == 1:
        for i in range(128, 256):
            origin_mask += parent_mask[i * 256 + 128:i * 256 + 256]
    elif corner == 2:
        for i in range(128):
            origin_mask += parent_mask[i * 256:i * 256 + 128]
    else:
        for i in range(128):
            origin_mask += parent_mask[i * 256 + 128:i * 256 + 256]
    new_mask = mask_interpolation(origin_mask)
    write_back(file_path, new_mask)


# Modify child tiles of higher lod level after modifying a tile
def recursive_downward_modify(terrain_folder_path, lod, X, Y):
    parent_path = terrain_folder_path + str(lod) + "\\" + str(X) + "\\" + str(Y) + ".terrain"
    for i in range(2):
        for j in range(2):
            child_path = terrain_folder_path + str(lod + 1) + "\\" + str(X * 2 + j) + "\\" + str(Y * 2 + i) + ".terrain"
            if os.path.exists(child_path):
                modify_child(parent_path, child_path, 2 * i + j)
                recursive_downward_modify(terrain_folder_path, lod + 1, X * 2 + j, Y * 2 + i)


should_send = False


def send_num_modified(connection):
    connection.sendall(str(num_modified).encode())
    if should_send:
        timer = threading.Timer(0.5, send_num_modified, args=(connection,))
        timer.start()


# Modify tiles that are covered by the segment result.
def modify_tiles(mask, terrain_folder_path, lod, bottom_left, offset, orthowidth_and_tilesize, connection, cover):
    StartX = int(bottom_left[0])
    StartY = int(bottom_left[1])
    offset[0] = int(offset[0])
    offset[1] = int(offset[1])
    ortho_width = int(orthowidth_and_tilesize[0])
    tile_size = int(orthowidth_and_tilesize[1])
    viewport_scale = int(ortho_width / tile_size)

    timer = threading.Timer(0.5, send_num_modified, args=(connection,))
    global should_send
    should_send = True
    timer.start()

    for i in range(0, viewport_scale + 1):
        for j in range(0, viewport_scale + 1):
            file_path = terrain_folder_path + lod + "\\" + str(StartX + j) + "\\" + str(
                StartY + viewport_scale - i) + ".terrain"
            if os.path.exists(file_path):
                modify_watermask(file_path, mask, i, j, ortho_width, tile_size, offset, cover)
                recursive_downward_modify(terrain_folder_path, int(lod), int(StartX + j),
                                          int(StartY + viewport_scale - i))

    should_send = False
    global num_modified
    num_modified = 0
    print("modify finished")


def modify_without_recursive(mask, terrain_folder_path, lod, bottom_left, offset, orthowidth_and_tilesize, connection,
                             cover):
    StartX = int(bottom_left[0])
    StartY = int(bottom_left[1])
    offset[0] = int(offset[0])
    offset[1] = int(offset[1])
    ortho_width = int(orthowidth_and_tilesize[0])
    tile_size = int(orthowidth_and_tilesize[1])
    viewport_scale = int(ortho_width / tile_size)

    timer = threading.Timer(0.5, send_num_modified, args=(connection,))
    global should_send
    should_send = True
    timer.start()

    for i in range(0, viewport_scale + 1):
        for j in range(0, viewport_scale + 1):
            file_path = terrain_folder_path + lod + "\\" + str(StartX + j) + "\\" + str(
                StartY + viewport_scale - i) + ".terrain"
            if os.path.exists(file_path):
                modify_watermask(file_path, mask, i, j, ortho_width, tile_size, offset, cover)

    should_send = False
    global num_modified
    num_modified = 0
    print("modify finished")
