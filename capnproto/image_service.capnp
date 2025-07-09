@0xf8a8c9d2e3b4f5a6;

enum Status {
  ok @0;
  imageSizeMismatch @1;
  error @2;
}

struct ImageData {
  payload @0 :Data;
}

struct ImageResponse {
  status @0 :Status;
}

interface ImageService {
  sendImage @0 (imageData :ImageData) -> (response :ImageResponse);
}