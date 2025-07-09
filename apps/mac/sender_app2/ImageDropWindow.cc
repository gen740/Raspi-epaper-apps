#include "ImageDropWindow.hh"

namespace Application {

ImageDropLabel::ImageDropLabel(QWidget *parent) : QLabel(parent) {
  setAlignment(Qt::AlignCenter);
  setText("Drop an image here");
  setStyleSheet("border: 2px dashed gray; padding: 10px;");
  setAcceptDrops(true);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  setMinimumSize(800, 800);
}

void ImageDropLabel::dragEnterEvent(QDragEnterEvent *e) {
  if (e->mimeData()->hasImage() || e->mimeData()->hasUrls()) {
    e->acceptProposedAction();
  }
}

void ImageDropLabel::dropEvent(QDropEvent *e) {
  if (e->mimeData()->hasImage()) {
    image_ = qvariant_cast<QImage>(e->mimeData()->imageData());
  } else if (e->mimeData()->hasUrls()) {
    const QUrl url = e->mimeData()->urls().first();
    if (url.isLocalFile()) {
      image_.load(url.toLocalFile());
    }
  }
  update();
  e->acceptProposedAction();
  setText(nullptr);
  emit imageSelected(image_);
}

void ImageDropLabel::paintEvent(QPaintEvent *ev) {
  QLabel::paintEvent(ev);
  if (image_.isNull()) {
    return;
  }
  QPainter p(this);
  p.setRenderHint(QPainter::SmoothPixmapTransform, true);
  QSize target = image_.size();
  target.scale(size(), Qt::KeepAspectRatio);

  QRect rect((width() - target.width()) / 2, (height() - target.height()) / 2,
             target.width(), target.height());

  setStyleSheet("border: 0px; padding: 10px;");
  p.drawImage(rect, image_);
  if (!rubber_.isNull()) {
    p.setPen(Qt::black);
    p.drawRect(rubber_.normalized());
  }
}

void ImageDropLabel::mousePressEvent(QMouseEvent *ev) {
  if (image_.isNull() || ev->button() != Qt::LeftButton) {
    return;
  }
  origin_ = ev->pos();
  rubber_ = QRect(origin_, QSize());
  update();
}

void ImageDropLabel::mouseMoveEvent(QMouseEvent *ev) {
  if (rubber_.isNull()) {
    return;
  }
  rubber_.setBottomRight(ev->pos());
  update();
}

}; // namespace Application
