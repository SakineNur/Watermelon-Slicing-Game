#include <QApplication>
#include <QWidget>
#include <QFile>
#include <QTextStream>
#include <QLabel>
#include <QPixmap>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QTime>
#include <QDebug>
#include <QTimer>
#include <QList>
#include <QGraphicsPixmapItem>
#include <random>
#include <QGraphicsSceneMouseEvent>
#include <QElapsedTimer>
#include <QVBoxLayout>
#include <QMainWindow>
#include <QMessageBox>
#include <QGroupBox>

class ClickablePixmapItem : public QGraphicsPixmapItem {
public:
    ClickablePixmapItem(const QPixmap &pixmap, QLabel *cutLabel)
        : QGraphicsPixmapItem(pixmap), cutLabel(cutLabel) {}

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override {
        if (pixmap().isNull()) {
            event->ignore();
            return;
        }

        // Karpuz kesildi
        QPixmap kesikKarpuzPixmap(":/icons/icons/2.png");
        int yeniGenislik = kesikKarpuzPixmap.width() / 7;
        int yeniYukseklik = kesikKarpuzPixmap.height() / 7;
        kesikKarpuzPixmap = kesikKarpuzPixmap.scaled(yeniGenislik, yeniYukseklik, Qt::KeepAspectRatio);
        this->setPixmap(kesikKarpuzPixmap);

        QElapsedTimer timer;
        timer.start();
        while (timer.elapsed() < 1000) {
            QCoreApplication::processEvents();
        }
        this->hide();

        if (cutLabel != nullptr) {
            int cutCount = cutLabel->text().split(" ").last().toInt();
            cutCount++;
            cutLabel->setText(QString("Kesilen Karpuzlar: %1").arg(cutCount));
        }

        event->accept();
    }

private:
    QLabel *cutLabel;
};

void writeHighestScoreToFile(const QString &fileName, int highestScore) {
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        stream << highestScore;
        file.close();
    } else {
        qDebug() << "Dosya yazma hatası!";
    }
}

int readHighestScoreFromFile(const QString &fileName) {
    int highestScore = 0;
    QFile file(fileName);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        QString line = in.readLine();
        if (!line.isEmpty()) {
            highestScore = line.toInt();
        }
        file.close();
    } else {
        qDebug() << "Dosya okuma hatası!";
    }
    return highestScore;
}

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    int remainingTime = 30;

    QMainWindow mainWindow;
    mainWindow.setWindowTitle("Karpuzlar");

    QWidget *centralWidget = new QWidget(&mainWindow);
    mainWindow.setCentralWidget(centralWidget);

    QVBoxLayout mainLayout(centralWidget);
    QVBoxLayout topLayout;

    QLabel cutLabel("Kesilen Karpuzlar: 0");
    QLabel missedLabel("Kaçırılan Karpuzlar: 0");
    QLabel timeLabel("Süre: 30");

    QFont font = cutLabel.font();
    font.setPointSize(16);
    cutLabel.setFont(font);
    missedLabel.setFont(font);
    timeLabel.setFont(font);

    topLayout.addWidget(&timeLabel);
    topLayout.addWidget(&missedLabel, 0, Qt::AlignRight);
    topLayout.addWidget(&cutLabel, 0, Qt::AlignRight);

    mainLayout.addLayout(&topLayout);

    QGraphicsView view;
    view.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QGraphicsScene scene(0, 0, 1350, 750);
    view.setScene(&scene);

    QPixmap backgroundPixmap(":/background/background/back.jpg");
    backgroundPixmap = backgroundPixmap.scaled(scene.width(), scene.height());
    QGraphicsPixmapItem *backgroundItem = scene.addPixmap(backgroundPixmap);

    mainLayout.addWidget(&view);

    QFile file(":/konumlar/konumlar/konumlar.txt");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Dosya açılamadı!";
        return -1;
    }

    QTextStream in(&file);
    QList<QPoint> konumlar;
    while (!in.atEnd()) {
        QString line = in.readLine();
        QStringList parts = line.split(" ");
        if (parts.size() != 2) continue;
        konumlar.append(QPoint(parts[0].toInt(), parts[1].toInt()));
    }
    file.close();

    QList<ClickablePixmapItem*> karpuzlar;

    QTime time = QTime::currentTime();
    std::default_random_engine generator(time.msec());
    std::uniform_int_distribution<int> distribution(0, konumlar.size() - 1);

    QTimer karpuzEkleTimer;
    QObject::connect(&karpuzEkleTimer, &QTimer::timeout, [&]() {
        if (remainingTime > 0) {
            QPixmap karpuzPixmap(":/icons/icons/1.png");
            karpuzPixmap = karpuzPixmap.scaled(karpuzPixmap.width() / 7, karpuzPixmap.height() / 7, Qt::KeepAspectRatio);

            ClickablePixmapItem *karpuzItem = new ClickablePixmapItem(karpuzPixmap, &cutLabel);
            scene.addItem(karpuzItem);
            karpuzlar.append(karpuzItem);

            int randomIndex = distribution(generator);
            QPoint randomKonum = konumlar[randomIndex];
            karpuzItem->setPos(randomKonum);

            QTimer *timer = new QTimer(&mainWindow);
            int hiz = 5;

            QObject::connect(timer, &QTimer::timeout, [=, &scene, &karpuzlar, &missedLabel]() mutable {
                if (!karpuzItem) return;
                karpuzItem->setPos(karpuzItem->x(), karpuzItem->y() + hiz);

                if (karpuzItem->y() > scene.height()) {
                    int missedCount = missedLabel.text().split(" ").last().toInt();
                    missedCount++;
                    missedLabel.setText(QString("Kaçırılan Karpuzlar: %1").arg(missedCount));
                    scene.removeItem(karpuzItem);
                    karpuzlar.removeOne(karpuzItem);
                    delete karpuzItem;
                    karpuzItem = nullptr;
                    timer->stop();
                    timer->deleteLater();
                }
            });
            timer->start(100);
        }
    });
    karpuzEkleTimer.start(1000);

    QTimer remainingTimeTimer;
    QObject::connect(&remainingTimeTimer, &QTimer::timeout, [&]() {
        if (remainingTime > 0) {
            remainingTime--;
            timeLabel.setText(QString("Kalan Süre: %1").arg(remainingTime));
        }
    });
    remainingTimeTimer.start(1000);

    QTimer::singleShot(30000, [&]() {
        int cutCount = cutLabel.text().split(" ").last().toInt();
        int missedCount = missedLabel.text().split(" ").last().toInt();
        int highestScore = readHighestScoreFromFile("highest_score.txt");

        if (cutCount > highestScore) {
            highestScore = cutCount;
            writeHighestScoreToFile("highest_score.txt", highestScore);
            QMessageBox::information(&mainWindow, "Tebrikler!", "Yeni bir rekor kırdınız!\nYeni en yüksek skor: " + QString::number(highestScore));
        }

        for (ClickablePixmapItem *karpuzItem : karpuzlar) {
            scene.removeItem(karpuzItem);
            delete karpuzItem;
        }
        karpuzlar.clear();

        QMessageBox msgBox;
        msgBox.setWindowTitle("Oyun Bitti");
        msgBox.setText(QString("Kesilen Karpuzlar: %1\nKaçırılan Karpuzlar: %2\nEn Yüksek Skor: %3").arg(cutCount).arg(missedCount).arg(highestScore));
        msgBox.setStandardButtons(QMessageBox::Close);
        msgBox.setDefaultButton(QMessageBox::Close);
        QObject::connect(&msgBox, &QMessageBox::finished, [&mainWindow](int result) {
            if (result == QMessageBox::Close)
                mainWindow.close();
        });
        msgBox.exec();
    });

    mainWindow.showMaximized();
    return a.exec();
}

