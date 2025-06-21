#include "ImagePreviewWindow.hh"
#include "ImageProcess.hh"
#include "grpc_client.hh"

#include <QPainter>

namespace Application {

using GRPC::ImageClient;
using Processing::Atkinson;
using Processing::closest_epd_color;
using Processing::EPDColor;

ImagePreviewLabel::ImagePreviewLabel(QWidget *parent) : QLabel(parent) {
  setAlignment(Qt::AlignCenter);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  setMinimumSize(480, 480);
}

void ImagePreviewLabel::setImage(const QImage &image) {
  original_image_ = image;
  if (original_image_.height() > original_image_.width()) {
    height_ = 800;
    width_ = 480;
  } else {
    height_ = 480;
    width_ = 800;
  }
  QImage canvas(width_, height_, QImage::Format_ARGB32);
  QImage fit = original_image_.convertToFormat(QImage::Format_ARGB32)
                   .scaled(canvas.size(), Qt::KeepAspectRatioByExpanding,
                           Qt::SmoothTransformation);
  QPainter cp(&canvas);
  cp.drawImage((width_ - fit.width()) / 2, (height_ - fit.height()) / 2, fit);
  const QRgb *pix = reinterpret_cast<const QRgb *>(canvas.constBits());
  scaled_buffer_.resize(3UL * width_ * height_);
  buffer_.resize(3UL * width_ * height_);
  for (int i = 0; i < width_ * height_; ++i) {
    scaled_buffer_[i * 3 + 0] = static_cast<uint8_t>(qRed(pix[i]));
    scaled_buffer_[i * 3 + 1] = static_cast<uint8_t>(qGreen(pix[i]));
    scaled_buffer_[i * 3 + 2] = static_cast<uint8_t>(qBlue(pix[i]));
  }
  buffer_ = scaled_buffer_;
  dither();
  update();
}

void ImagePreviewLabel::adjustImage(
    const Processing::ImageAdjustParams &params) {
  if (scaled_buffer_.empty()) {
    return;
  }
  Processing::adjustImage(scaled_buffer_, buffer_, width_, height_, params);
  dither();
  update();
}

void ImagePreviewLabel::sendImage() {
  std::vector<std::uint8_t> payload;
  payload.reserve(width_ * height_ / 2);
  std::vector<uint8_t> send_buffer;
  if (width_ > height_) {
    send_buffer = buffer_;
  } else {
    send_buffer = Processing::rotate90(buffer_, width_, height_);
  }
  for (int i = 0; i < width_ * height_; i += 2) {
    EPDColor c1 =
        closest_epd_color(&send_buffer[static_cast<ptrdiff_t>(i * 3)]);
    EPDColor c2 =
        closest_epd_color(&send_buffer[(static_cast<ptrdiff_t>(i + 1) * 3)]);
    payload.push_back((static_cast<std::uint8_t>(c2) & 0x0F) |
                      (static_cast<std::uint8_t>(c1) << 4));
  }

  auto client = std::make_shared<ImageClient>(grpc::CreateChannel(
      "192.168.1.101:50051", grpc::InsecureChannelCredentials()));
  future_ =
      QtConcurrent::run([p = std::move(payload), client] { client->Send(p); });
}

void ImagePreviewLabel::paintEvent(QPaintEvent *ev) {
  QLabel::paintEvent(ev);
  if (original_image_.isNull()) {
    return;
  }
  QPainter p(this);
  p.setRenderHint(QPainter::SmoothPixmapTransform, true);
  auto target = image_.size();
  target.scale(size(), Qt::KeepAspectRatio);
  QRect rect((width() - target.width()) / 2, (height() - target.height()) / 2,
             target.width(), target.height());
  p.drawImage(rect, image_);
}

void ImagePreviewLabel::dither() {
  Atkinson(buffer_, width_, height_);
  image_ = QImage(buffer_.data(), width_, height_, QImage::Format_RGB888);
};

}; // namespace Application

#include "ImagePreviewWindow.moc"
