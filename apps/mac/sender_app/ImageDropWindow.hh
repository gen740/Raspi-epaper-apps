#pragma once

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

namespace Application {

class ImageDropLabel : public QLabel {
  Q_OBJECT

public:
  explicit ImageDropLabel(QWidget *parent = nullptr);

signals:
  // void imageDropped(const QImage &image); //
  // ドロップされた画像を通知

protected:
  void dragEnterEvent(QDragEnterEvent *e) override;

  void dropEvent(QDropEvent *e) override;

  void paintEvent(QPaintEvent *ev) override;

  [[nodiscard]] auto sizeHint() const -> QSize override { return {320, 240}; }

signals:
  void imageSelected(const QImage &image);

protected:
  void mousePressEvent(QMouseEvent *ev) override;

  void mouseMoveEvent(QMouseEvent *ev) override;

private:
  QImage image_; // オリジナル画像（1 枚だけ保持）
  QPoint origin_;
  QRect rubber_;
};

} // namespace Application
