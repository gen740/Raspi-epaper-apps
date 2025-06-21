#pragma once

#include <QLabel>
#include <QtConcurrent>

#include "ImageProcess.hh"

namespace Application {

class ImagePreviewLabel : public QLabel {
  Q_OBJECT

public:
  explicit ImagePreviewLabel(QWidget *parent = nullptr);

public slots:
  void setImage(const QImage &image);

  void adjustImage(const Processing::ImageAdjustParams &params);

  void sendImage();

protected:
  void paintEvent(QPaintEvent *ev) override;

private:
  void dither();

private:
  QImage original_image_;
  QImage image_;
  int width_;
  int height_;
  std::vector<uint8_t> scaled_buffer_;
  std::vector<uint8_t> buffer_;
  QFuture<void> future_;
};

}; // namespace Application
