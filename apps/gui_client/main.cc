// main.cc — Qt6 GUI application that leverages gui_client.hh for palette, d
// thering, and gRPC. Build with CMake (CMAKE_AUTOMOC ON).  Only GUI‑specific l
// gic lives here.
// ---------------------------------------------------------------------------------

#include <QApplication>
#include <QColor>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFutureWatcher>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QLoggingCategory>
#include <QMimeData>
#include <QPixmap>
#include <QPushButton>
#include <QSlider>
#include <QTemporaryDir>
#include <QTransform>
#include <QVBoxLayout>
#include <QWidget>
#include <QtConcurrent/QtConcurrent>

#include "gui_client.hh" // palette, Atkinson, encode_image, ImageClient

using namespace Apps::Common;

// ────────────────────────────────────────────────────────────────────────────────
//  Drag‑and‑drop label
// ────────────────────────────────────────────────────────────────────────────────
class DragLabel : public QLabel {
  Q_OBJECT
public:
  explicit DragLabel(QWidget *parent = nullptr) : QLabel(parent) {
    setAcceptDrops(true);
    setAlignment(Qt::AlignCenter);
    setStyleSheet("border: 2px dashed #aaa;");
  }

signals:
  void imageDropped(const QString &path);

protected:
  void dragEnterEvent(QDragEnterEvent *e) override {
    if (e->mimeData()->hasUrls())
      e->acceptProposedAction();
  }
  void dropEvent(QDropEvent *e) override {
    for (const QUrl &u : e->mimeData()->urls()) {
      if (u.isLocalFile()) {
        emit imageDropped(u.toLocalFile());
        break;
      }
    }
  }
};

// ────────────────────────────────────────────────────────────────────────────────
//  Main editor widget
// ────────────────────────────────────────────────────────────────────────────────
class ImageEditor : public QWidget {
  Q_OBJECT
public:
  ImageEditor() {
    // ---- UI skeleton --------------------------------------------------
    auto *main = new QHBoxLayout(this);

    // --- Left: drag area + sliders
    auto *left = new QVBoxLayout;
    drop = new DragLabel;
    drop->setMinimumSize(240, 240);
    left->addWidget(drop, 1);

    auto *sliderBox = new QVBoxLayout;
    addSlider("露光", &sExposure, sliderBox);
    addSlider("コントラスト", &sContrast, sliderBox);
    addSlider("ハイライト", &sHighlight, sliderBox);
    addSlider("シャドウ", &sShadow, sliderBox);
    addSlider("彩度", &sSaturation, sliderBox);
    addSlider("色温度", &sTemperature, sliderBox);
    addSlider("色合い", &sHue, sliderBox);
    left->addLayout(sliderBox);

    main->addLayout(left, 1);

    // --- Right: previews + buttons
    origLabel = makePreview();
    adjLabel = makePreview();
    dithLabel = makePreview();

    auto *pre = new QHBoxLayout;
    pre->addWidget(origLabel, 1);
    pre->addWidget(adjLabel, 1);
    pre->addWidget(dithLabel, 1);

    auto *btnRow = new QHBoxLayout;
    btnSave = new QPushButton("保存");
    btnSend = new QPushButton("送信");
    btnRow->addWidget(btnSave);
    btnRow->addWidget(btnSend);

    auto *right = new QVBoxLayout;
    right->addLayout(pre);
    right->addLayout(btnRow);
    main->addLayout(right, 2);

    // ---- Connections --------------------------------------------------
    connect(drop, &DragLabel::imageDropped, this, &ImageEditor::loadImage);
    for (QSlider *s : {sExposure, sContrast, sHighlight, sShadow, sSaturation,
                       sTemperature, sHue})
      connect(s, &QSlider::valueChanged, this, &ImageEditor::scheduleProcess);

    connect(&watcher, &QFutureWatcher<void>::finished, this,
            &ImageEditor::onProcessed);
    connect(btnSave, &QPushButton::clicked, this, &ImageEditor::saveDithered);
    connect(btnSend, &QPushButton::clicked, this, &ImageEditor::sendDithered);
  }

private slots:
  // ---------------------------------------------------------------------
  void loadImage(const QString &path) {
    QImage img(path);
    if (img.isNull()) {
      qWarning() << "Failed to load" << path;
      return;
    }
    srcPath = path;
    srcImage = img.convertToFormat(QImage::Format_RGB888);
    showFit(origLabel, srcImage);
    scheduleProcess();
  }

  void scheduleProcess() {
    if (srcImage.isNull())
      return;
    params = curParams();
    if (processing) {
      pending = true;
      return;
    }
    startProcess();
  }

  void onProcessed() {
    processing = false;
    adjImage = processedAdj;
    dithImage = processedDith;
    showFit(adjLabel, adjImage);
    showFit(dithLabel, dithImage);
    if (pending) {
      pending = false;
      startProcess();
    }
  }

  void saveDithered() {
    if (dithImage.isNull())
      return;
    QString p =
        QFileDialog::getSaveFileName(this, "Save BMP", {}, "BMP Files (*.bmp)");
    if (!p.isEmpty())
      dithImage.save(p, "BMP");
  }

  void sendDithered() {
    if (dithImage.isNull())
      return;
    QImage toSend = dithImage;
    if (toSend.width() == 480 && toSend.height() == 800) {
      QTransform t;
      t.rotate(-90);
      toSend = toSend.transformed(t);
    }

    QTemporaryDir tmp;
    QString tmpBmp = tmp.filePath("send.bmp");
    if (!toSend.save(tmpBmp, "BMP")) {
      qWarning() << "temp save failed";
      return;
    }

    std::vector<std::uint8_t> payload;
    try {
      payload = encode_image(tmpBmp.toStdString());
    } catch (const std::exception &e) {
      qWarning() << e.what();
      return;
    }

    auto client = std::make_shared<ImageClient>(grpc::CreateChannel(
        "192.168.1.101:50051", grpc::InsecureChannelCredentials()));
    QtConcurrent::run([p = std::move(payload), client] {
      client->Send(p);
    }).waitForFinished();
  }

private:
  // ---------------------------------------------------------------------
  struct Params {
    int ex, co, hi, sh, sa, te, hu;
  } params;

  Params curParams() const {
    return {sExposure->value(), sContrast->value(),   sHighlight->value(),
            sShadow->value(),   sSaturation->value(), sTemperature->value(),
            sHue->value()};
  }

  void startProcess() {
    processing = true;
    QImage srcCopy = srcImage;
    Params p = params;
    bool horiz = srcCopy.width() >= srcCopy.height();

    watcher.setFuture(QtConcurrent::run([this, srcCopy, p, horiz] {
      processedAdj = adjustImage(srcCopy, p);
      processedDith = dither(processedAdj, horiz);
    }));
  }

  // --- image adjustments (simple approximations) -----------------------
  static QImage adjustImage(QImage img, const Params &p) {
    int w = img.width(), h = img.height();
    for (int y = 0; y < h; ++y) {
      uchar *line = img.scanLine(y);
      for (int x = 0; x < w; ++x) {
        uchar *px = line + 3 * x;
        float r = px[0], g = px[1], b = px[2];

        // exposure
        float expF = 1.f + p.ex / 100.f;
        r *= expF;
        g *= expF;
        b *= expF;
        // contrast
        float cF = 1.f + p.co / 100.f;
        r = (r - 128.f) * cF + 128.f;
        g = (g - 128.f) * cF + 128.f;
        b = (b - 128.f) * cF + 128.f;

        // HSV adjustments via QColor
        QColor colVal = QColor::fromRgb(std::clamp(int(r), 0, 255),
                                        std::clamp(int(g), 0, 255),
                                        std::clamp(int(b), 0, 255));
        float hF, sF, vF;
        colVal.getHsvF(&hF, &sF, &vF);
        hF += p.hu / 360.f;
        if (hF < 0)
          hF += 1.f;
        if (hF > 1)
          hF -= 1.f;
        sF = std::clamp<float>(sF * (1.f + p.sa / 100.f), 0.f, 1.f);
        colVal.setHsvF(std::fmod(hF < 0.0 ? hF + 1.0 : hF, 1.0),
                       std::clamp<float>(sF, 0.0, 1.0),
                       std::clamp<float>(vF, 0.0, 1.0));
        r = colVal.red();
        g = colVal.green();
        b = colVal.blue();

        // temperature (warm/cool)
        r += p.te;
        b -= p.te;

        // highlights / shadows (very rough)
        float lum = 0.299f * r + 0.587f * g + 0.114f * b;
        if (lum > 128.f) {
          float f = 1.f + p.hi / 100.f;
          r = 128.f + (r - 128.f) * f;
          g = 128.f + (g - 128.f) * f;
          b = 128.f + (b - 128.f) * f;
        } else {
          float f = 1.f + p.sh / 100.f;
          r *= f;
          g *= f;
          b *= f;
        }

        px[0] = std::clamp<int>(r, 0, 255);
        px[1] = std::clamp<int>(g, 0, 255);
        px[2] = std::clamp<int>(b, 0, 255);
      }
    }
    return img;
  }

  static QImage dither(const QImage &img, bool horizontal) {
    const int targetW = horizontal ? WIDTH : HEIGHT;
    const int targetH = horizontal ? HEIGHT : WIDTH;

    // アスペクト比を保ったまま、短辺が target に収まるようスケーリング（cover）
    QImage scaled = img.scaled(targetW, targetH, Qt::KeepAspectRatioByExpanding,
                               Qt::SmoothTransformation);

    // スケーリング後の中心から targetW×targetH を切り出す
    int x = (scaled.width() - targetW) / 2;
    int y = (scaled.height() - targetH) / 2;
    QImage cropped = scaled.copy(x, y, targetW, targetH);

    // RGB データをバッファにコピー
    std::vector<std::uint8_t> buf(targetW * targetH * 3);
    for (int row = 0; row < targetH; ++row) {
      memcpy(buf.data() + 3 * targetW * row, cropped.constScanLine(row),
             3 * targetW);
    }

    Atkinson(buf, targetW, targetH);
    QImage out(buf.data(), targetW, targetH, QImage::Format_RGB888);
    return out.copy();
  }
  static QLabel *makePreview() {
    QLabel *l = new QLabel;
    l->setAlignment(Qt::AlignCenter);
    l->setMinimumSize(240, 240);
    l->setStyleSheet("border:1px solid #555;");
    return l;
  }
  static void showFit(QLabel *l, const QImage &img) {
    l->setPixmap(QPixmap::fromImage(img).scaled(l->size(), Qt::KeepAspectRatio,
                                                Qt::SmoothTransformation));
  }
  static void addSlider(const char *name, QSlider **out, QVBoxLayout *box) {
    box->addWidget(new QLabel(name));
    QSlider *s = new QSlider(Qt::Horizontal);
    s->setRange(-100, 100);
    *out = s;
    box->addWidget(s);
  }

  // UI widgets
  DragLabel *drop{};
  QLabel *origLabel{};
  QLabel *adjLabel{};
  QLabel *dithLabel{};
  QSlider *sExposure{}, *sContrast{}, *sHighlight{}, *sShadow{}, *sSaturation{},
      *sTemperature{}, *sHue{};
  QPushButton *btnSave{}, *btnSend{};

  // state
  QString srcPath;
  QImage srcImage, adjImage, dithImage, processedAdj, processedDith;
  bool processing = false, pending = false;
  QFutureWatcher<void> watcher;
};

#include "main.moc"

int main(int argc, char **argv) {
  QApplication app(argc, argv);
  ImageEditor w;
  w.resize(1200, 600);
  w.show();
  return app.exec();
}
