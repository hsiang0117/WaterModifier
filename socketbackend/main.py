import subprocess
import sys

import torch
from matplotlib import pyplot as plt

from tilemodifier import *

os.environ["OPENCV_IO_ENABLE_OPENEXR"] = "1"
import cv2
import numpy as np

from segment_anything import sam_model_registry, SamPredictor

sam_checkpoint = "models/sam_vit_b_01ec64.pth"
model_type = "vit_b"

device = "cuda:0" if torch.cuda.is_available() else "cpu"

def get_image():
    img = cv2.imread('out.exr', cv2.IMREAD_UNCHANGED)
    if img is None:
        raise ValueError("Failed to load image. Check the file path and format.")
    img = img[:, :, :3]
    img = np.nan_to_num(img, nan=0.0, posinf=1.0, neginf=0.0)  # 将 NaN/Inf 替换为合理值
    img = np.maximum(img, 0)  # 确保没有负数
    img = cv2.normalize(img, None, 0, 1, cv2.NORM_MINMAX)
    img = np.power(img, 1.0 / 2.2)
    img = (img * 255).astype(np.uint8)
    img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
    predictor.set_image(img)
    return img


def segment(input_point, input_label, ortho_width):
    mask, _, _ = predictor.predict(
        point_coords=input_point,
        point_labels=input_label,
        multimask_output=False,
    )
    return mask.reshape(ortho_width, ortho_width)


def save_mask(mask):
    color = np.array([255 / 255, 255 / 255, 255 / 255, 0.5])
    h, w = mask.shape[-2:]
    mask_image = mask.reshape(h, w, 1) * color.reshape(1, 1, -1)
    plt.imsave("mask.png", mask_image)


import socket

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.bind(("", 10086))
s.listen(1)

def recv_all(sock, n):
    data = b''
    while len(data) < n:
        packet = sock.recv(n-len(data))
        if not packet:
            return None
        data += packet
    return data

def recv_len(sock):
    raw_bytes = recv_all(sock,4)
    if raw_bytes is None:
        return None
    if len(raw_bytes) != 4:
        return None
    int_value = struct.unpack("<i",raw_bytes)[0]
    return int_value

def recv_msg(sock):
    msg_length = recv_len(sock)
    if msg_length is None:
        return None
    raw_msg = recv_all(sock, msg_length)
    if raw_msg is None:
        return None
    return raw_msg.decode()

if __name__ == '__main__':
    #subprocess.Popen(["WaterModifier-Win64-Shipping.exe"])
    sam = sam_model_registry[model_type](checkpoint=sam_checkpoint)
    sam.to(device=device)
    predictor = SamPredictor(sam)
    image = None
    ortho_width = None
    mask = None
    while True:
        connection, address = s.accept()
        print("Connected by:", address)
        input_points = []
        input_labels = []
        pen_points = []
        try:
            while True:
                data = recv_msg(connection)
                content = ""
                if data == "OrthoWidth":
                    ortho_width = recv_msg(connection)
                    ortho_width = int(ortho_width)
                    mask = np.zeros(shape=(ortho_width,ortho_width), dtype=bool)
                elif data == "PositiveDot":
                    dot = recv_msg(connection)
                    coords = dot.split(" ")
                    coords[0] = float(coords[0])
                    coords[1] = float(coords[1])
                    input_points.append(coords)
                    input_labels.append(1)
                elif data == "NegativeDot":
                    dot = recv_msg(connection)
                    coords = dot.split(" ")
                    coords[0] = float(coords[0])
                    coords[1] = float(coords[1])
                    input_points.append(coords)
                    input_labels.append(0)
                elif data == "PenDot":
                    dot = recv_msg(connection)
                    coords = dot.split(" ")
                    coords[0] = float(coords[0])
                    coords[1] = float(coords[1])
                    pen_points.append(coords)
                elif data == "Segment":
                    input_points_array = np.array(input_points)
                    input_labels_array = np.array(input_labels)
                    pen_points_array = np.array(pen_points)
                    if len(input_points_array):
                        mask = segment(input_points_array, input_labels_array, ortho_width)
                    if len(pen_points_array):
                        mask = pen_process(pen_points_array, mask)
                    save_mask(mask)
                    content = "SegmentDone"
                elif data == "Modify":
                    cover = recv_msg(connection)
                    terrain_folder_path = recv_msg(connection)
                    lod = recv_msg(connection)
                    bottom_left = recv_msg(connection).split(" ")
                    offset = recv_msg(connection).split(" ")
                    orthowidth_and_tilesize = recv_msg(connection).split(" ")
                    array = analyse_mask(mask.tolist())
                    modify_tiles(array, terrain_folder_path, lod, bottom_left, offset, orthowidth_and_tilesize, connection, cover)
                    content = "ModifyDone"
                elif data == "ModifyWithoutRecursive":
                    cover = recv_msg(connection)
                    terrain_folder_path = recv_msg(connection)
                    lod = recv_msg(connection)
                    bottom_left = recv_msg(connection).split(" ")
                    offset = recv_msg(connection).split(" ")
                    orthowidth_and_tilesize = recv_msg(connection).split(" ")
                    array = analyse_mask(mask.tolist())
                    modify_without_recursive(array, terrain_folder_path, lod, bottom_left, offset, orthowidth_and_tilesize, connection, cover)
                    content = "ModifyDone"
                elif data == "ExportDone":
                    image = get_image()
                    content = "SetImageDone"
                elif data == "Clear":
                    image = None
                    mask = np.zeros(shape=(ortho_width,ortho_width), dtype=bool)
                    input_points = []
                    input_labels = []
                    pen_points = []
                    predictor.reset_image()

                if content == "":
                    content = "received"
                connection.sendall(content.encode())
        except ConnectionAbortedError:
            print("Connection aborted")
            connection.close()
            sys.exit(0)