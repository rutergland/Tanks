#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtCore>
#include <QtWidgets>
#include <QTcpSocket>
#include <QTcpServer>
#include <QtNetwork>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);
    void onTimer();
    void moveOgr(QGraphicsPixmapItem* item, qreal vx, qreal vy);
    void onConnect();
    void onDisconnect();
    void getMessage();
    void sendMessage();

    void on_connectButton_clicked();

    void on_createButton_clicked();

    void on_ip_2_currentTextChanged(const QString &arg1);

private:
    Ui::MainWindow *ui;

    QTcpSocket* client;
    QTcpServer* server;
    QTimer *timer;
    QGraphicsScene *scene;
    QGraphicsPixmapItem *background;
    QGraphicsPixmapItem *tank;
    QGraphicsPixmapItem *enemy;
    QGraphicsPixmapItem *bullet;
    QGraphicsPixmapItem *bulletE;
    QGraphicsPixmapItem *explosion;
    QGraphicsPixmapItem *explosionE;
    QPixmap background_texture;
    QPixmap tank_texture;
    QPixmap enemy_texture;
    QPixmap bullet_texture;
    QPixmap explosion_texture;

    bool isOnline = false;
    int shootSchet = 200, shootESchet = 200, schet = 0, schetE = 0, sendSchet = 0, explosionSchet = 1000, explosionESchet = 1000;
    bool W = false, A = false, S = false, D = false, shoot = false;
};
#endif // MAINWINDOW_H
