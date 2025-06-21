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

#include "ImageDropWindow.hh"
#include "ImagePreviewWindow.hh"
#include "ToolWindow.hh"

class MainWidget : public QWidget {
  Q_OBJECT

public:
  MainWidget() {
    image_label_ = new Application::ImageDropLabel(this);
    image_preview_label_ = new Application::ImagePreviewLabel(this);

    auto *lay = new QVBoxLayout(this);
    auto *hl = new QHBoxLayout();
    hl->addWidget(image_label_);
    hl->addWidget(image_preview_label_);
    connect(image_label_, &Application::ImageDropLabel::imageSelected,
            image_preview_label_, &Application::ImagePreviewLabel::setImage);

    lay->addLayout(hl);
    setLayout(lay);

    // ------★ 追加部分 ----------------------------------------
    tool_window_ = std::make_unique<Application::ToolWindow>();
    tool_window_->show();
    tool_window_->raise();
    tool_window_->activateWindow();

    connect(tool_window_.get(), &Application::ToolWindow::setParams,
            image_preview_label_, &Application::ImagePreviewLabel::adjustImage);
    connect(tool_window_.get(), &Application::ToolWindow::sendImage,
            image_preview_label_, &Application::ImagePreviewLabel::sendImage);
  }

private:
  Application::ImageDropLabel *image_label_;
  Application::ImagePreviewLabel *image_preview_label_;
  QFuture<int> future_;
  QFutureWatcher<int> watcher_;
  std::unique_ptr<Application::ToolWindow> tool_window_;
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
