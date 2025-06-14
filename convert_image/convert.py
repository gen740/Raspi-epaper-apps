from PIL import Image
import numpy as np
from scipy.spatial import KDTree
import copy

# 6-color palette (RGB)
PALETTE = np.array(
    [
        (0, 0, 0),  # Black
        (255, 255, 255),  # White
        (255, 255, 0),  # Yellow
        (255, 0, 0),  # Red
        (0, 0, 255),  # Blue
        (0, 255, 0),  # Green
    ],
    dtype=np.uint8,
)
tree = KDTree(PALETTE)


# --- Step 1: 画像読み込みと中央トリミング ---
def scale_and_center_crop(im: Image.Image, size: tuple[int, int]) -> Image.Image:
    target_w, target_h = size
    src_w, src_h = im.size

    # 拡大率を求める（アスペクト比維持・全体が入るように拡大）
    scale = max(target_w / src_w, target_h / src_h)
    new_w, new_h = int(src_w * scale), int(src_h * scale)

    # リサイズ（アスペクト比維持、全体が入る）
    resized = im.resize((new_w, new_h), Image.LANCZOS)

    # 中央から切り抜き
    left = (new_w - target_w) // 2
    top = (new_h - target_h) // 2
    right = left + target_w
    bottom = top + target_h

    return resized.crop((left, top, right, bottom))


# 入力画像
img = Image.open("input.png").convert("RGB")
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
