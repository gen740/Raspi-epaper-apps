#include "ImagePreviewWindow.hh"
#include "ImageProcess.hh"
#include "grpc_client.hh"

#include <QDir>
#include <QFileDialog>
#include <QImage>
#include <QMessageBox>
#include <QPainter>
#include <QProcess>
#include <QStringLiteral>
#include <format>

namespace Application {

using GRPC::ImageClient;
using Processing::closest_epd_color;
using Processing::EPDColor;

using Processing::Atkinson;
using Processing::Burkes;
using Processing::DBSDither;
using Processing::FloydSteinberg;
using Processing::JarvisJudiceNinke;
using Processing::Ordered;
using Processing::Random;
using Processing::Sierra;
using Processing::Sierra2;
using Processing::SierraLite;
using Processing::Stucki;
using Processing::Threshold;

ImagePreviewLabel::ImagePreviewLabel(QWidget *parent) : QLabel(parent) {
  setAlignment(Qt::AlignCenter);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  setMinimumSize(800, 800);
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
  Processing::adjustImage(scaled_buffer_, buffer_, width_, height_, params_);
  dither();
  update();
}

void ImagePreviewLabel::adjustImage(
    const Processing::ImageAdjustParams &params) {
  if (scaled_buffer_.empty()) {
    return;
  }
  params_ = params;
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

void ImagePreviewLabel::saveImage() {
  auto kFilter = QStringLiteral(
      "BMP (*.bmp);;PNG (*.png);;JPEG (*.jpg *.jpeg);;TIFF (*.tif *.tiff)");

  auto image_dir = QDir::homePath() + "/eink_images";

  if (!QDir(image_dir).exists()) {
    QDir().mkpath(image_dir);
  }
  QDir dir(image_dir);
  size_t file_count = dir.entryList(QDir::Files).count();
  const QString filePath = QFileDialog::getSaveFileName(
      this, QStringLiteral("画像を保存"),
      image_dir +
          QString(std::format("/image_{:03d}", file_count + 1).c_str()),
      kFilter);

  if (filePath.isEmpty()) {
    return;
  }

  if (!image_.save(filePath)) { // 保存失敗
    QMessageBox::warning(
        this, QStringLiteral("保存失敗"),
        QStringLiteral("ファイル %1 を保存できませんでした。").arg(filePath));
    return;
  }

  // Finder で該当ファイルを選択状態 (-R) で開く
  QProcess::startDetached(QStringLiteral("open"),
                          {QStringLiteral("-R"), filePath});
  return;
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

void Application::ImagePreviewLabel::dither() {

  switch (params_.dithering_algorithm) {
  case Processing::DitheringAlgorithm::ATKINSON:
    Atkinson(buffer_, width_, height_);
    break;
  case Processing::DitheringAlgorithm::FLOYD_STEINBERG:
    FloydSteinberg(buffer_, width_, height_);
    break;
  case Processing::DitheringAlgorithm::JARVIS_JUDICE_NINKE:
    JarvisJudiceNinke(buffer_, width_, height_);
    break;
  case Processing::DitheringAlgorithm::STUCKI:
    Stucki(buffer_, width_, height_);
    break;
  case Processing::DitheringAlgorithm::BURKES:
    Burkes(buffer_, width_, height_);
    break;
  case Processing::DitheringAlgorithm::SIERRA:
    Sierra(buffer_, width_, height_);
    break;
  case Processing::DitheringAlgorithm::SIERRA2:
    Sierra2(buffer_, width_, height_);
    break;
  case Processing::DitheringAlgorithm::SIERRA_LITE:
    SierraLite(buffer_, width_, height_);
    break;
  case Processing::DitheringAlgorithm::DBS:
    DBSDither(buffer_, width_, height_);
    break;
  case Processing::DitheringAlgorithm::ORDERED:
    Ordered(buffer_, width_, height_);
    break;
  case Processing::DitheringAlgorithm::RANDOM:
    Random(buffer_, width_, height_);
    break;
  case Processing::DitheringAlgorithm::THRESHOLD:
    Threshold(buffer_, width_, height_);
    break;
  default:
    break;
  }
  image_ = QImage(buffer_.data(), width_, height_, QImage::Format_RGB888);
};

}; // namespace Application

#include "ImagePreviewWindow.moc"
