#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <ctime>

int portC = 3516;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    srand(time(0));

    timer = new QTimer(this);
    timer->start(1000/165);
    connect(timer, SIGNAL(timeout()), this, SLOT(onTimer()));

    //загрузка текстур
    background_texture.load(":/images/background.png");
    tank_texture.load(":/images/tank64.png");
    enemy_texture.load(":/images/enemy64.png");
    bullet_texture.load(":/images/bullet16.png");
    explosion_texture.load(":/images/explosion64.png");

    //сцена
    scene = new QGraphicsScene(0, 0, ui->graphicsView->width(), ui->graphicsView->height());
    ui->graphicsView->setScene(scene);

    //отрисовка
    background = scene->addPixmap(background_texture);
    background->setPos(0, 0);

    enemy = scene->addPixmap(enemy_texture);
    enemy->setOffset(-32, -32);
    enemy->setPos(132,268);

    tank = scene->addPixmap(tank_texture);
    tank->setOffset(-32, -32);
    tank->setPos(368,268);

    bullet = scene->addPixmap(bullet_texture);
    bullet->setOffset(-8, -8);
    bullet->setPos(-100, -100);

    bulletE = scene->addPixmap(bullet_texture);
    bulletE->setOffset(-8, -8);
    bulletE->setPos(-100, -100);

    explosion = scene->addPixmap(explosion_texture);
    explosion->setOffset(-32, -33);
    explosion->hide();

    explosionE = scene->addPixmap(explosion_texture);
    explosionE->setOffset(-32, -33);
    explosionE->hide();

    //список всех адресов
    for(QHostAddress address : QNetworkInterface::allAddresses())
    {
        if(address != QHostAddress::LocalHost && address.protocol() == QTcpSocket::IPv4Protocol) {
            qDebug() << address;
            ui->ip_2->addItem(address.toString());
        }
    }

    client = new QTcpSocket();
    connect(client, SIGNAL(readyRead()), this, SLOT(getMessage()));
    connect(client, SIGNAL(disconnected()), this, SLOT(onDisconnect()));
    if (client -> bind(portC)){
        qDebug() << "Client started" << portC;
    }
    else {
        qDebug() << "Client error";
    }
    server = new QTcpServer();
    connect(server, SIGNAL(newConnection()), this, SLOT(onConnect()));
}

MainWindow::~MainWindow()
{
    delete ui;
    client->disconnectFromHost();
    server->close();
    client->close();
}

void MainWindow::onConnect()
{
    while (server->hasPendingConnections()){
        QTcpSocket* user = server->nextPendingConnection();
        connect(user, SIGNAL(readyRead()), this, SLOT(getMessage()));
        connect(user, SIGNAL(disconnected()), this, SLOT(onDisconnect()));
        qDebug() << "New user connected"
                 << user->peerAddress().toString()
                 << user->peerPort();
    }
    schet = 0;
    schetE = 0;
    ui->schet->setText("Ваш счёт: " + QString::number(schet));
    ui->schetE->setText("Счёт противника: " + QString::number(schetE));
}

void MainWindow::onDisconnect()
{
    qDebug() << "User disconnected";
}

void MainWindow::getMessage()
{
    QTcpSocket* user = (QTcpSocket*)sender();
    QJsonObject obj;
    while (!user->atEnd()){
        QJsonDocument doc = QJsonDocument::fromJson(user->readAll());
        obj = doc.object();
    }

    if (!client->isOpen() and !obj.isEmpty()) {
        QString serverAddress;
        int serverPort;

        serverAddress = obj.value("serverAddress").toString();
        serverPort = obj.value("serverPort").toInt();

        ui->ip->setText(serverAddress);
        ui->port->setValue(serverPort);

        client->connectToHost(QHostAddress(serverAddress), serverPort);
    }

    qreal x, y, rotation, bx, by, brotation;

    if (!obj.isEmpty()) {
        x = obj.value("x").toVariant().toReal();
        y = obj.value("y").toVariant().toReal();
        rotation = obj.value("rotation").toVariant().toReal();
        bx = obj.value("bx").toVariant().toReal();
        by = obj.value("by").toVariant().toReal();
        brotation = obj.value("brotation").toVariant().toReal();

        enemy->setPos(x, y);
        enemy->setRotation(rotation);
        bulletE->setPos(bx, by);
        bulletE->setRotation(brotation);
        bulletE->show();
    }
    qDebug() << "Принято";
}

void MainWindow::moveOgr(QGraphicsPixmapItem *item, qreal vx, qreal vy)
{
    QRectF scene = ui->graphicsView->scene()->sceneRect();
    QRectF itemHit = item->sceneBoundingRect();
    QRectF otherHit;
    bool gox = false;
    bool goy = false;

    // if(item == tank) {
    //     otherHit = enemy->sceneBoundingRect();
    // }
    // else {
    //     otherHit = tank->sceneBoundingRect();
    // }

    QRectF itemHitX = itemHit.translated(vx, 0);
    if (scene.contains(itemHitX) and !itemHitX.intersects(otherHit)) {
        gox = true;
    }

    QRectF itemHitY = itemHit.translated(0, vy);
    if (scene.contains(itemHitY) and !itemHitY.intersects(otherHit)) {
        goy = true;
    }
    if (gox and goy){
        item->moveBy(vx, vy);
    }
}

void MainWindow::sendMessage()
{
    //отправка информации
    if (client->isOpen()) {
        QJsonDocument doc;
        QJsonObject obj;
        obj.insert("serverAddress", server->serverAddress().toString());
        obj.insert("serverPort", server->serverPort());
        obj.insert("x", tank->scenePos().x());
        obj.insert("y", tank->scenePos().y());
        obj.insert("rotation", tank->rotation());
        obj.insert("bx", bullet->scenePos().x());
        obj.insert("by", bullet->scenePos().y());
        obj.insert("brotation", bullet->rotation());
        doc.setObject(obj);
        client->write(doc.toJson());
        qDebug() << tank->scenePos().x() << tank->scenePos().y();
        qDebug() << "Отправлено";
    }
}

void MainWindow::onTimer()
{
    shootSchet++;
    sendSchet++;
    explosionSchet++;
    explosionESchet++;

    if (sendSchet >= 10) {
        sendMessage();
    }

    //колизия
    QRectF tankHit = tank->sceneBoundingRect();
    QRectF enemyHit = enemy->sceneBoundingRect();
    QRectF bulletHit = bullet->sceneBoundingRect();
    QRectF bulletEHit = bulletE->sceneBoundingRect();
    QRectF scene = ui->graphicsView->scene()->sceneRect();

    if (!scene.intersects(bulletHit)) {
        bullet->hide();
    }

    if (!scene.intersects(bulletEHit)) {
        bulletE->hide();
    }

    if (bulletHit.intersects(enemyHit)) {
        bullet->hide();
        bullet->setPos(-500,-1000);
        explosionE->setPos(enemy->pos());
        schet++;
        ui->schet->setText("Ваш счёт: " + QString::number(schet));
        explosionESchet = 0;
        explosionE->show();
    }

    if (bulletEHit.intersects(tankHit)) {
        sendMessage();
        bulletE->hide();
        bulletE->setPos(-500,-1000);
        explosion->setPos(tank->pos());
        tank->setPos(rand()%704+32, rand()%504+32);
        schetE++;
        ui->schetE->setText("Счёт противника: " + QString::number(schetE));
        explosionSchet = 0;
        explosion->show();
    }

    //взрывы
    if (explosionSchet>=100 and explosion->isVisible()) {
        explosion->hide();
    }

    if (explosionESchet>=100 and explosionE->isVisible()) {
        explosionE->hide();
    }

    //действия клавиш
    if (W) {
        if (A) {
            moveOgr(tank, -sqrt(2), -sqrt(2));
            tank->setRotation(-45);
        }
        else if (D) {
            moveOgr(tank, sqrt(2), -sqrt(2));
            tank->setRotation(45);
        }
        else if (!A and !D) {
            moveOgr(tank, 0, -2);
            tank->setRotation(0);
        }
    }
    else if (A) {
        if (W) {
            moveOgr(tank, -sqrt(2), -sqrt(2));
            tank->setRotation(-45);
        }
        else if (S) {
            moveOgr(tank, -sqrt(2), sqrt(2));
            tank->setRotation(-135);
        }
        else if (!W and !S) {
            moveOgr(tank, -2, 0);
            tank->setRotation(-90);
        }
    }
    else if (S) {
        if (A) {
            moveOgr(tank, -sqrt(2), sqrt(2));
            tank->setRotation(-135);
        }
        else if (D) {
            moveOgr(tank, sqrt(2), sqrt(2));
            tank->setRotation(135);
        }
        else if (!A and !D) {
            moveOgr(tank, 0, 2);
            tank->setRotation(180);
        }
    }
    else if (D) {
        if (W) {
            moveOgr(tank, sqrt(2), -sqrt(2));
            tank->setRotation(45);
        }
        else if (S) {
            moveOgr(tank, sqrt(2), sqrt(2));
            tank->setRotation(135);
        }
        else if (!W and !S) {
            moveOgr(tank, 2, 0);
            tank->setRotation(90);
        }
    }

    if (bullet->isVisible()) {
        if (bullet->rotation()==0) {
            bullet->moveBy(0, -3);
        }
        else if (bullet->rotation()==-90) {
            bullet->moveBy(-3, 0);
        }
        else if (bullet->rotation()==180) {
            bullet->moveBy(0, 3);
        }
        else if (bullet->rotation()==90) {
            bullet->moveBy(3, 0);
        }

        else if (bullet->rotation()==45) {
            bullet->moveBy(sqrt(3), -sqrt(3));
        }
        else if (bullet->rotation()==135) {
            bullet->moveBy(sqrt(3), sqrt(3));
        }
        else if (bullet->rotation()==-135) {
            bullet->moveBy(-sqrt(3), sqrt(3));
        }
        else if (bullet->rotation()==-45) {
            bullet->moveBy(-sqrt(3), -sqrt(3));
        }
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    qDebug() << "key:" << event->key();
    if(event->key() == Qt::Key_W or event->key() == Qt::Key(1062)) {
        W = true;
    }
    if (event->key() == Qt::Key_A or event->key() == Qt::Key(1060)) {
        A = true;
    }
    if (event->key() == Qt::Key_S or event->key() == Qt::Key(1067)) {
        S = true;
    }
    if (event->key() == Qt::Key_D or event->key() == Qt::Key(1042)) {
        D = true;
    }
    if (event->key() == Qt::Key_Space and !bullet->isVisible() and shootSchet > 200){
        bullet = scene->addPixmap(bullet_texture);
        bullet->setOffset(-8, -8);
        shoot = true;
        QPointF p = tank->pos();

        if (tank->rotation() == 0) {
            bullet->setPos(p.x(), p.y()-48);
            bullet->setRotation(tank->rotation());
        }
        else if (tank->rotation() == 90) {
            bullet->setPos(p.x()+48, p.y());
            bullet->setRotation(tank->rotation());
        }
        else if (tank->rotation() == 180) {
            bullet->setPos(p.x(), p.y()+48);
            bullet->setRotation(tank->rotation());
        }
        else if (tank->rotation() == -90) {
            bullet->setPos(p.x()-48, p.y());
            bullet->setRotation(tank->rotation());
        }

        if (tank->rotation() == 45) {
            bullet->setPos(p.x()+48, p.y()-48);
            bullet->setRotation(tank->rotation());
        }
        else if (tank->rotation() == 135) {
            bullet->setPos(p.x()+48, p.y()+48);
            bullet->setRotation(tank->rotation());
        }
        else if (tank->rotation() == -135) {
            bullet->setPos(p.x()-48, p.y()+48);
            bullet->setRotation(tank->rotation());
        }
        else if (tank->rotation() == -45) {
            bullet->setPos(p.x()-48, p.y()-48);
            bullet->setRotation(tank->rotation());
        }

        bullet->show();
        sendSchet = 10;
        shootSchet = 0;
    }
}

void MainWindow::keyReleaseEvent(QKeyEvent *event)
{
    if(event->key() == Qt::Key_W or event->key() == Qt::Key(1062)) {
        W = false;
    }
    if (event->key() == Qt::Key_A or event->key() == Qt::Key(1060)) {
        A = false;
    }
    if (event->key() == Qt::Key_S or event->key() == Qt::Key(1067)) {
        S = false;
    }
    if (event->key() == Qt::Key_D or event->key() == Qt::Key(1042)) {
        D = false;
    }
    if (event->key() == Qt::Key_Space){
        shoot = false;
    }
}

void MainWindow::on_connectButton_clicked()
{
    QString ip = ui -> ip -> text();
    int port = ui -> port -> value();

    if (client -> isOpen()) {
        client -> close();
    }
    client->connectToHost(QHostAddress(ip), port);
    if (!client -> isOpen()) {
        client -> disconnectFromHost();
    }
}


void MainWindow::on_createButton_clicked()
{
    QString address = ui->ip_2->currentText();
    int port = ui->port_2->value();
    if (server -> listen(QHostAddress(address), port)) {
        qDebug() << "Server started" << address << port;
    }
    else {
        qDebug() << "Server error";
    }
}

void MainWindow::on_ip_2_currentTextChanged(const QString &arg1)
{
    if (arg1 != "") {
        ui->ip->setText(arg1);
    }
}

