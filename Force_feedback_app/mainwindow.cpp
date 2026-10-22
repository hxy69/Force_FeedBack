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

//定义串口指针
QSerialPort *COM = new QSerialPort();
QSerialPort *MOTOR = new QSerialPort();

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
        if (MOTOR->isOpen())
            MOTOR->write("RST;\r\n");
    });

    /* 统一发送三路目标电流：任意一个滑块变动，都读取三个滑块当前值拼成新协议 */
    auto sendMotorTarget = [this]() {
        if (MOTOR->isOpen()) {
            const QByteArray cmd = QString("X:%1,Y:%2,Z:%3;\r\n")
                .arg(ui->X_MOTOR->value())
                .arg(ui->Y_MOTOR->value())
                .arg(ui->Z_MOTOR->value())
                .toUtf8();
            MOTOR->write(cmd);
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

    QFont valueFont;
    valueFont.setPointSize(12);
    ui->lineEdit_1->setFont(valueFont);
    ui->lineEdit_2->setFont(valueFont);
    ui->lineEdit_3->setFont(valueFont);
    ui->lineEdit_4->setFont(valueFont);
    ui->lineEdit_5->setFont(valueFont);
    ui->lineEdit_6->setFont(valueFont);

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
    if (COM->isOpen() || MOTOR->isOpen())
    {
        COM->close();
        MOTOR->close();
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

    MOTOR->setPortName(ui->MOTOR_COM->currentText());
    MOTOR->setBaudRate(baudRate);
    MOTOR->setDataBits(dataBits);
    MOTOR->setStopBits(stopBits);
    MOTOR->setParity(checkBits);
    MOTOR->setFlowControl(QSerialPort::NoFlowControl);

    if (COM->open(QIODevice::ReadWrite))
    {
        if (!MOTOR->open(QIODevice::WriteOnly))
            QMessageBox::warning(this, "Motor Port",
                                 "MOTOR_COM open failed, sending disabled.");
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
    ui->MOTOR_COM->clear();
    foreach(const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        ui->ENCODER_COM->addItem(info.portName());
        ui->MOTOR_COM->addItem(info.portName());
    }
}

void MainWindow::readSerialData()
{
    const QByteArray data = COM->readAll();
    m_serialBuffer.append(data);
    int end = m_serialBuffer.indexOf(';');
    while (end != -1) {
        const QByteArray line = m_serialBuffer.left(end + 1);
        m_serialBuffer.remove(0, end + 1);
        handleCommand(QString::fromUtf8(line));
        end = m_serialBuffer.indexOf(';');
    }
}

void MainWindow::handleCommand(const QString &command)
{
    const QStringList parts = command.split(';', Qt::SkipEmptyParts);
    for (const QString &part : parts) {
        const int colon = part.indexOf(':');
        if (colon == -1)
            continue;
        const QString key = part.left(colon).trimmed();
        const QString valueText = part.mid(colon + 1).trimmed();
        const QStringList values = valueText.split(',', Qt::SkipEmptyParts);

        if (key.compare("Position", Qt::CaseInsensitive) == 0
            && values.size() >= 3) {
            ui->lineEdit_4->setText(values[0].trimmed());
            ui->lineEdit_5->setText(values[1].trimmed());
            ui->lineEdit_6->setText(values[2].trimmed());
            // 把 Position 的三个值喂给三维空间控件
            ui->axisWidget->setPosition(values[0].toDouble(),
                                        values[1].toDouble(),
                                        values[2].toDouble());
        } else if (key.compare("Test", Qt::CaseInsensitive) == 0
                   && values.size() >= 3) {
            ui->lineEdit_1->setText(values[0].trimmed());
            ui->lineEdit_2->setText(values[1].trimmed());
            ui->lineEdit_3->setText(values[2].trimmed());
        }
    }
}
