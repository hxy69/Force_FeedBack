#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QSerialPort"      //串口
#include "QSerialPortInfo"  //串口信息
#include <QMessageBox>
#include <QFont>
#include <QIntValidator>
#include <QSize>
#include <QStringList>
#include <QStyle>

//定义串口指针（单串口：编码器数据 + 电机指令共用一路）
QSerialPort *COM = new QSerialPort();

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(COM, &QSerialPort::readyRead, this, &MainWindow::readSerialData);
    connect(ui->CONNECT, &QPushButton::clicked, this, &MainWindow::CONNECT_ctrl);
    connect(ui->RESET, &QPushButton::clicked, this, &MainWindow::RESET_ctrl);
    connect(ui->RESTART, &QPushButton::clicked, this, [this]() {
        ui->X_MOTOR->setValue(0);
        ui->Y_MOTOR->setValue(0);
        ui->Z_MOTOR->setValue(0);
        if (COM->isOpen())
            COM->write("RST;\r\n");
    });

    /* 统一发送三路目标电流：任意一个滑块变动，都读取三个滑块当前值拼成新协议 */
    auto sendMotorTarget = [this]() {
        if (COM->isOpen()) {
            const QByteArray cmd = QString("X:%1,Y:%2,Z:%3;\r\n")
                .arg(ui->X_MOTOR->value())
                .arg(ui->Y_MOTOR->value())
                .arg(ui->Z_MOTOR->value())
                .toUtf8();
            COM->write(cmd);
        }
    };

    connect(ui->X_MOTOR, &QSlider::valueChanged, this, [this, sendMotorTarget](int value) {
        ui->Force_X->setText(QString::number(value));
        sendMotorTarget();
    });

    connect(ui->Y_MOTOR, &QSlider::valueChanged, this, [this, sendMotorTarget](int value) {
        ui->Force_Y->setText(QString::number(value));
        sendMotorTarget();
    });

    connect(ui->Z_MOTOR, &QSlider::valueChanged, this, [this, sendMotorTarget](int value) {
        ui->Force_Z->setText(QString::number(value));
        sendMotorTarget();
    });

    ui->Force_X->setText(QString::number(ui->X_MOTOR->value()));
    ui->Force_Y->setText(QString::number(ui->Y_MOTOR->value()));
    ui->Force_Z->setText(QString::number(ui->Z_MOTOR->value()));
    ui->Force_X->setAlignment(Qt::AlignCenter);
    ui->Force_Y->setAlignment(Qt::AlignCenter);
    ui->Force_Z->setAlignment(Qt::AlignCenter);

    QIntValidator *forceValidator = new QIntValidator(-300, 300, this);
    ui->Force_X->setValidator(forceValidator);
    ui->Force_Y->setValidator(forceValidator);
    ui->Force_Z->setValidator(forceValidator);

    auto bindForceEdit = [this](QLineEdit *edit, QSlider *slider) {
        connect(edit, &QLineEdit::editingFinished, this, [edit, slider]() {
            bool ok = false;
            int value = edit->text().toInt(&ok);
            if (!ok || value < slider->minimum() || value > slider->maximum())
                value = slider->value();
            slider->setValue(value);
            edit->setText(QString::number(value));
        });
    };
    bindForceEdit(ui->Force_X, ui->X_MOTOR);
    bindForceEdit(ui->Force_Y, ui->Y_MOTOR);
    bindForceEdit(ui->Force_Z, ui->Z_MOTOR);

    ui->lineEdit_1->setReadOnly(true);
    ui->lineEdit_2->setReadOnly(true);
    ui->lineEdit_3->setReadOnly(true);
    ui->lineEdit_4->setReadOnly(true);
    ui->lineEdit_5->setReadOnly(true);
    ui->lineEdit_6->setReadOnly(true);
    ui->lineEdit_1->setAlignment(Qt::AlignCenter);
    ui->lineEdit_2->setAlignment(Qt::AlignCenter);
    ui->lineEdit_3->setAlignment(Qt::AlignCenter);
    ui->lineEdit_4->setAlignment(Qt::AlignCenter);
    ui->lineEdit_5->setAlignment(Qt::AlignCenter);
    ui->lineEdit_6->setAlignment(Qt::AlignCenter);
    ui->lineEdit_7->setAlignment(Qt::AlignCenter);
    ui->lineEdit_8->setAlignment(Qt::AlignCenter);
    ui->lineEdit_9->setAlignment(Qt::AlignCenter);

    QFont valueFont;
    valueFont.setPointSize(12);
    ui->lineEdit_1->setFont(valueFont);
    ui->lineEdit_2->setFont(valueFont);
    ui->lineEdit_3->setFont(valueFont);
    ui->lineEdit_4->setFont(valueFont);
    ui->lineEdit_5->setFont(valueFont);
    ui->lineEdit_6->setFont(valueFont);
    ui->lineEdit_7->setFont(valueFont);
    ui->lineEdit_8->setFont(valueFont);
    ui->lineEdit_9->setFont(valueFont);

    ui->X_MOTOR->setTracking(true);
    ui->Y_MOTOR->setTracking(true);
    ui->Z_MOTOR->setTracking(true);

    RESET_ctrl();

    ui->RESET->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    ui->RESET->setText("");
    ui->RESET->setIconSize(QSize(16, 16));
}

MainWindow::~MainWindow()
{
    delete ui;
}

//连接串口按键
void MainWindow::CONNECT_ctrl()
{
    if (COM->isOpen())
    {
        COM->close();
        ui->CONNECT->setStyleSheet("");
        ui->CONNECT->setText("打开串口");
        return;
    }

    //串口初始化
    QSerialPort::BaudRate baudRate;
    QSerialPort::StopBits stopBits;
    QSerialPort::DataBits dataBits;
    QSerialPort::Parity   checkBits;

    baudRate = QSerialPort::Baud9600;
    if (ui->BAUDRATE->currentText() == "9600")
    {
        baudRate = QSerialPort::Baud9600;
    }
    else if (ui->BAUDRATE->currentText() == "115200")
    {
        baudRate = QSerialPort::Baud115200;
    }

    stopBits = QSerialPort::OneStop;
    dataBits = QSerialPort::Data8;
    checkBits = QSerialPort::NoParity;

    COM->setPortName(ui->ENCODER_COM->currentText());
    COM->setBaudRate(baudRate);
    COM->setDataBits(dataBits);
    COM->setStopBits(stopBits);
    COM->setParity(checkBits);
    COM->setFlowControl(QSerialPort::NoFlowControl);

    if (COM->open(QIODevice::ReadWrite))
    {
        ui->CONNECT->setStyleSheet("background-color: lightgreen;");
        ui->CONNECT->setText("关闭串口");
    }
    else
    {
        ui->CONNECT->setStyleSheet("");
        QMessageBox::warning(this, "连接失败",
                             "串口打开失败，请检查串口是否已连接、端口和波特率是否设置正确。");
    }
}

//刷新按键
void MainWindow::RESET_ctrl()
{
    ui->ENCODER_COM->clear();
    foreach(const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        ui->ENCODER_COM->addItem(info.portName());
    }
}

void MainWindow::readSerialData()
{
    const QByteArray data = COM->readAll();
    m_serialBuffer.append(data);
    int end = m_serialBuffer.indexOf('\n');
    while (end != -1) {
        const QByteArray line = m_serialBuffer.left(end + 1);
        m_serialBuffer.remove(0, end + 1);
        handleCommand(QString::fromUtf8(line));
        end = m_serialBuffer.indexOf('\n');
    }
}

// 从带标签的token里取出数值："Encoder:-1.3" -> "-1.3"；"Attitude-4.4" -> "-4.4"（兼容无冒号）
static QString tagValue(const QString &token, const QString &tag)
{
    const int colon = token.indexOf(':');
    if (colon != -1)
        return token.mid(colon + 1).trimmed();
    return token.mid(tag.size()).trimmed();
}

// 解析STM32新协议：
// %.1f,%.1f,%.1f,%.1f,%.1f,%.1f,Encoder:%.1f,%.1f,%.1f,Position:%.1f,%.1f,%.1f,Attitude:%.1f,%.1f,%.1f\r\n
void MainWindow::handleCommand(const QString &line)
{
    const QString l = line.trimmed();
    if (l.isEmpty())
        return;

    const QStringList tokens = l.split(',', Qt::SkipEmptyParts);

    // 定位三个标签（前缀匹配，兼容 "Encoder:" / "Encoder"）
    int encIdx = -1, posIdx = -1, attIdx = -1;
    for (int i = 0; i < tokens.size(); ++i) {
        if (encIdx < 0 && tokens.at(i).startsWith("Encoder", Qt::CaseInsensitive))
            encIdx = i;
        else if (posIdx < 0 && tokens.at(i).startsWith("Position", Qt::CaseInsensitive))
            posIdx = i;
        else if (attIdx < 0 && tokens.at(i).startsWith("Attitude", Qt::CaseInsensitive))
            attIdx = i;
    }

    // 标签前的纯数字是电流 6 值（本版本不显示，跳过）

    // Encoder 编码器角度 -> lineEdit_1/2/3
    if (encIdx >= 0 && encIdx + 2 < tokens.size()) {
        ui->lineEdit_1->setText(tagValue(tokens.at(encIdx), "Encoder"));
        ui->lineEdit_2->setText(tokens.at(encIdx + 1).trimmed());
        ui->lineEdit_3->setText(tokens.at(encIdx + 2).trimmed());
    }

    // Position 位置 -> lineEdit_4/5/6 + 3D控件
    if (posIdx >= 0 && posIdx + 2 < tokens.size()) {
        const QString x = tagValue(tokens.at(posIdx), "Position");
        const QString y = tokens.at(posIdx + 1).trimmed();
        const QString z = tokens.at(posIdx + 2).trimmed();
        ui->lineEdit_4->setText(x);
        ui->lineEdit_5->setText(y);
        ui->lineEdit_6->setText(z);
        ui->axisWidget->setPosition(x.toDouble(), y.toDouble(), z.toDouble());
    }

    // Attitude 欧拉角 -> 显示到 Roll/Pitch/Yaw 数字框（lineEdit_7/9/8），并暂存成员
    if (attIdx >= 0 && attIdx + 2 < tokens.size()) {
        m_attX = tagValue(tokens.at(attIdx), "Attitude").toDouble();
        m_attY = tokens.at(attIdx + 1).trimmed().toDouble();
        m_attZ = tokens.at(attIdx + 2).trimmed().toDouble();
        ui->lineEdit_7->setText(QString::number(m_attX, 'f', 1));   // Roll
        ui->lineEdit_9->setText(QString::number(m_attY, 'f', 1));   // Pitch
        ui->lineEdit_8->setText(QString::number(m_attZ, 'f', 1));   // Yaw
    }
}
