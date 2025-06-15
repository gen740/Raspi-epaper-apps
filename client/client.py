import grpc
import numpy as np
from PIL import Image
import sys

from image_server_pb2 import DataRequest
from image_server_pb2_grpc import DataServiceStub
from image_server_pb2 import Status as DataStatus


class EPDColor:
    BLACK = 0x00
    WHITE = 0x01
    YELLOW = 0x02
    RED = 0x03
    BLUE = 0x05
    GREEN = 0x06


PALETTE = {
    EPDColor.BLACK: np.array([0, 0, 0]),
    EPDColor.WHITE: np.array([255, 255, 255]),
    EPDColor.YELLOW: np.array([255, 255, 0]),
    EPDColor.RED: np.array([255, 0, 0]),
    EPDColor.BLUE: np.array([0, 0, 255]),
    EPDColor.GREEN: np.array([0, 255, 0]),
}


def closest_epd_color(rgb: np.ndarray) -> int:
    diffs = {color: np.linalg.norm(rgb - val) for color, val in PALETTE.items()}
    return min(diffs, key=diffs.get)


def encode_image(image_path: str) -> bytes:
    image = Image.open(image_path).convert("RGB")
    np_img = np.array(image)

    if np_img.shape[:2] != (480, 800):
        raise ValueError(
            f"Expected 480x800 image, got {np_img.shape[0]}x{np_img.shape[1]}"
        )

    flat_rgb = np_img.reshape(-1, 3)
    encoded = []

    for i in range(0, len(flat_rgb), 2):
        c1 = closest_epd_color(flat_rgb[i])
        c2 = closest_epd_color(flat_rgb[i + 1])
        encoded.append((c1 << 4) | (c2 & 0x0F))

    return bytes(encoded)


def send_image_data(payload: bytes) -> None:
    with grpc.insecure_channel(
        "192.168.1.101:50051",
        options=[("grpc.primary_user_agent", "ipv4-client")],
        compression=None,
    ) as channel:
        try:
            grpc.channel_ready_future(channel).result(timeout=5)
            print("Channel connected.")
        except grpc.FutureTimeoutError:
            print("Failed to connect to server.")
            return

        stub = DataServiceStub(channel)

        print(f"Sending {len(payload)} bytes")
        request = DataRequest(payload=payload)
        response = stub.SendData(request)

        print("Received status:", DataStatus.Name(response.status))


def main() -> None:
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} /path/to/image.bmp")
        sys.exit(1)

    image_path = sys.argv[1]
    payload = encode_image(image_path)
    send_image_data(payload)


if __name__ == "__main__":
    main()
