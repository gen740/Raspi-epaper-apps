from PIL import Image
import numpy as np
from scipy.spatial import KDTree
import copy

# 6-color palette (RGB)
PALETTE = np.array(
    [
        (0, 0, 0),  # Black
        # (30, 30, 30),
        # (10, 10, 10),
        (255, 255, 255),  # White
        # (180, 180, 180),  # White
        # (245, 245, 245),  # White
        (255, 255, 0),  # Yellow
        # (200, 195, 65),  # Yellow
        # (245, 245, 12),  # Yellow
        (255, 0, 0),  # Red
        # (150, 50, 30),  # Red
        # (245, 12, 10),  # Red
        (0, 0, 255),  # Blue
        # (50, 80, 150),  # Blue
        # (10, 13, 245),  # Blue
        (0, 255, 0),  # Green
        # (50, 70, 60),  # Green
        # (10, 245, 12),  # Green
    ],
    dtype=np.uint8,
)
tree = KDTree(PALETTE)


# --- Step 1: 画像読み込みと中央トリミング ---
def scale_and_center_crop(im: Image.Image, size: tuple[int, int]) -> Image.Image:
    target_w, target_h = size
    src_w, src_h = im.size

    # 対象アスペクト比
    target_aspect = target_w / target_h
    src_aspect = src_w / src_h

    if src_aspect > target_aspect:
        # 横長なので左右をトリミング
        new_w = int(src_h * target_aspect)
        left = (src_w - new_w) // 2
        top = 0
        right = left + new_w
        bottom = src_h
    else:
        # 縦長なので上下をトリミング
        new_h = int(src_w / target_aspect)
        top = (src_h - new_h) // 2
        left = 0
        bottom = top + new_h
        right = src_w

    # 中央トリミングしてからリサイズ
    cropped = im.crop((left, top, right, bottom))
    resized = cropped.resize((target_w, target_h), Image.LANCZOS)
    return resized


# 入力画像
img = Image.open("input.jpg").convert("RGB")
img = scale_and_center_crop(img, (800, 480))
img.save("scaled_image.bmp", format="BMP")
img_np = np.array(img, dtype=np.int16)


height, width, _ = img_np.shape
for y in range(height):
    for x in range(width):
        old_pixel = copy.deepcopy(img_np[y, x])
        _, idx = tree.query(old_pixel)
        new_pixel = copy.deepcopy(PALETTE[idx])
        img_np[y, x] = new_pixel
        error = old_pixel - new_pixel

        # 拡散
        for dx, dy, coeff in [
            (1, 0, 7 / 16),
            (-1, 1, 3 / 16),
            (0, 1, 5 / 16),
            (1, 1, 1 / 16),
        ]:
            nx, ny = x + dx, y + dy
            if 0 <= nx < width and 0 <= ny < height:
                img_np[ny, nx] += (error * coeff).astype(np.int16)

# 画素値を正規化して画像化
img_np = np.clip(img_np, 0, 255).astype(np.uint8)
dithered_img = Image.fromarray(img_np)

# --- Step 3: BMPで保存 ---
dithered_img.save("output.bmp", format="BMP")
