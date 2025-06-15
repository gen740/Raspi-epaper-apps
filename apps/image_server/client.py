import grpc
import random
import time

from image_server_pb2 import DataRequest
from image_server_pb2_grpc import DataServiceStub
from image_server_pb2 import Status as DataStatus


def generate_random_bytes(length: int) -> bytes:
    return bytes(random.getrandbits(8) for _ in range(length))


def main() -> None:
    with grpc.insecure_channel("192.168.1.101:50051") as channel:
        stub = DataServiceStub(channel)

        payload = generate_random_bytes(16)  # 任意のバイト長
        print(f"Sending {len(payload)} bytes:", payload.hex())

        request = DataRequest(payload=payload)
        response = stub.SendData(request)

        print("Received status:", DataStatus.Name(response.status))


if __name__ == "__main__":
    main()
