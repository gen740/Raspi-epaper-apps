#include <QApplication>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>
#include <QWidget>
#include <QtConcurrent>
#include <QtGui/qevent.h>
#include <QtWidgets/qboxlayout.h>
#include <thread>

#include "ToolWindow.hh"
#include "dithering.hh"
#include "grpc_client.hh"
#include "imageAdjuster.hh"

auto heavy_calculation(int x) -> int {
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  return x * x;
}

class ImageDropLabel : public QLabel {
  Q_OBJECT

public:
  explicit ImageDropLabel(QWidget *parent = nullptr) : QLabel(parent) {
    setAlignment(Qt::AlignCenter);
    setText("Drop an image here");
    setStyleSheet("border: 2px dashed gray; padding: 10px;");
    setAcceptDrops(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMinimumSize(480, 480);
  }

signals:
  // void imageDropped(const QImage &image); //
  // ドロップされた画像を通知

protected:
  void dragEnterEvent(QDragEnterEvent *e) override {
    if (e->mimeData()->hasImage() || e->mimeData()->hasUrls()) {
      e->acceptProposedAction();
    }
  }

  void dropEvent(QDropEvent *e) override {
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

  void paintEvent(QPaintEvent *ev) override {
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

  [[nodiscard]] auto sizeHint() const -> QSize override { return {320, 240}; }

signals:
  void imageSelected(const QImage &image);

protected:
  void mousePressEvent(QMouseEvent *ev) override {
    if (image_.isNull() || ev->button() != Qt::LeftButton) {
      return;
    }
    origin_ = ev->pos();
    rubber_ = QRect(origin_, QSize());
    update();
  }

  void mouseMoveEvent(QMouseEvent *ev) override {
    if (rubber_.isNull()) {
      return;
    }
    rubber_.setBottomRight(ev->pos());
    update();
  }

private:
  QImage image_; // オリジナル画像（1 枚だけ保持）
  QPoint origin_;
  QRect rubber_;
};

class ImagePreviewLabel : public QLabel {
  Q_OBJECT

public:
  explicit ImagePreviewLabel(QWidget *parent = nullptr) : QLabel(parent) {
    setAlignment(Qt::AlignCenter);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMinimumSize(480, 480);
  }

public slots:
  void setImage(const QImage &image) {
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

  void adjustImage(const ImageAdjustParams &params) {
    if (scaled_buffer_.empty()) {
      return;
    }
    ::adjustImage(scaled_buffer_, buffer_, width_, height_, params);
    dither();
    update();
  }

  void sendImage() {
    std::vector<std::uint8_t> payload;
    payload.reserve(width_ * height_ / 2);
    std::vector<uint8_t> send_buffer;
    if (width_ > height_) {
      send_buffer = buffer_;
    } else {
      send_buffer = rotate90(buffer_, width_, height_);
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
    future_ = QtConcurrent::run(
        [p = std::move(payload), client] { client->Send(p); });
  }

protected:
  void paintEvent(QPaintEvent *ev) override {
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

private:
  void dither() {
    Atkinson(buffer_, width_, height_);
    image_ = QImage(buffer_.data(), width_, height_, QImage::Format_RGB888);
  };

private:
  QImage original_image_;
  QImage image_;
  int width_;
  int height_;
  std::vector<uint8_t> scaled_buffer_;
  std::vector<uint8_t> buffer_;
  QFuture<void> future_;
};

class MainWidget : public QWidget {
  Q_OBJECT

public:
  MainWidget() {
    image_label_ = new ImageDropLabel(this);
    image_preview_label_ = new ImagePreviewLabel(this);

    auto *lay = new QVBoxLayout(this);
    auto *hl = new QHBoxLayout();
    hl->addWidget(image_label_);
    hl->addWidget(image_preview_label_);
    connect(image_label_, &ImageDropLabel::imageSelected, image_preview_label_,
            &ImagePreviewLabel::setImage);

    lay->addLayout(hl);
    setLayout(lay);

    // ------★ 追加部分 ----------------------------------------
    tool_window_ = std::make_unique<ToolWindow>();
    tool_window_->show();
    tool_window_->raise();
    tool_window_->activateWindow();

    connect(tool_window_.get(), &ToolWindow::setParams, image_preview_label_,
            &ImagePreviewLabel::adjustImage);
    connect(tool_window_.get(), &ToolWindow::sendImage, image_preview_label_,
            &ImagePreviewLabel::sendImage);
  }

private:
  ImageDropLabel *image_label_;
  ImagePreviewLabel *image_preview_label_;
  QFuture<int> future_;
  QFutureWatcher<int> watcher_;
  std::unique_ptr<ToolWindow> tool_window_;
};

#include "main.moc"

auto main(int argc, char *argv[]) -> int {
  QApplication app(argc, argv);
  MainWidget w;
  w.setWindowTitle("Async Demo");
  w.resize(320, 140);
  w.show();
  return app.exec();
}
