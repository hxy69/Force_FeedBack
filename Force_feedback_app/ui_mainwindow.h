/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 5.14.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSlider>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QWidget>
#include "axis3dwidget.h"

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QSlider *X_MOTOR;
    QComboBox *ENCODER_COM;
    QComboBox *BAUDRATE;
    QPushButton *CONNECT;
    QPushButton *RESET;
    QSlider *Y_MOTOR;
    QSlider *Z_MOTOR;
    QLineEdit *lineEdit_1;
    QLineEdit *lineEdit_2;
    QLineEdit *lineEdit_3;
    QLineEdit *lineEdit_4;
    QLineEdit *lineEdit_5;
    QLineEdit *lineEdit_6;
    QLabel *label;
    QLabel *label_2;
    QLabel *label_3;
    QLabel *label_4;
    QLabel *label_5;
    QLabel *label_6;
    QLabel *label_7;
    QLabel *label_8;
    QLabel *label_9;
    QPushButton *RESTART;
    QLineEdit *Force_X;
    QLineEdit *Force_Y;
    QLineEdit *Force_Z;
    QLabel *label_10;
    QLabel *label_12;
    Axis3DWidget *axisWidget;
    QLineEdit *lineEdit_7;
    QLabel *label_11;
    QLabel *label_13;
    QLabel *label_14;
    QLineEdit *lineEdit_8;
    QLineEdit *lineEdit_9;
    QMenuBar *menubar;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QString::fromUtf8("MainWindow"));
        MainWindow->resize(891, 618);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        X_MOTOR = new QSlider(centralwidget);
        X_MOTOR->setObjectName(QString::fromUtf8("X_MOTOR"));
        X_MOTOR->setGeometry(QRect(201, 323, 331, 20));
        X_MOTOR->setMinimum(-300);
        X_MOTOR->setMaximum(300);
        X_MOTOR->setOrientation(Qt::Horizontal);
        ENCODER_COM = new QComboBox(centralwidget);
        ENCODER_COM->setObjectName(QString::fromUtf8("ENCODER_COM"));
        ENCODER_COM->setGeometry(QRect(670, 367, 125, 25));
        BAUDRATE = new QComboBox(centralwidget);
        BAUDRATE->addItem(QString());
        BAUDRATE->addItem(QString());
        BAUDRATE->setObjectName(QString::fromUtf8("BAUDRATE"));
        BAUDRATE->setGeometry(QRect(670, 397, 125, 25));
        CONNECT = new QPushButton(centralwidget);
        CONNECT->setObjectName(QString::fromUtf8("CONNECT"));
        CONNECT->setGeometry(QRect(670, 427, 94, 26));
        RESET = new QPushButton(centralwidget);
        RESET->setObjectName(QString::fromUtf8("RESET"));
        RESET->setGeometry(QRect(770, 427, 26, 26));
        Y_MOTOR = new QSlider(centralwidget);
        Y_MOTOR->setObjectName(QString::fromUtf8("Y_MOTOR"));
        Y_MOTOR->setGeometry(QRect(201, 383, 331, 20));
        Y_MOTOR->setMinimum(-300);
        Y_MOTOR->setMaximum(300);
        Y_MOTOR->setOrientation(Qt::Horizontal);
        Z_MOTOR = new QSlider(centralwidget);
        Z_MOTOR->setObjectName(QString::fromUtf8("Z_MOTOR"));
        Z_MOTOR->setGeometry(QRect(201, 443, 331, 20));
        Z_MOTOR->setMinimum(-300);
        Z_MOTOR->setMaximum(300);
        Z_MOTOR->setOrientation(Qt::Horizontal);
        lineEdit_1 = new QLineEdit(centralwidget);
        lineEdit_1->setObjectName(QString::fromUtf8("lineEdit_1"));
        lineEdit_1->setGeometry(QRect(130, 107, 61, 27));
        lineEdit_1->setMinimumSize(QSize(0, 24));
        lineEdit_2 = new QLineEdit(centralwidget);
        lineEdit_2->setObjectName(QString::fromUtf8("lineEdit_2"));
        lineEdit_2->setGeometry(QRect(130, 152, 61, 27));
        lineEdit_2->setMinimumSize(QSize(0, 24));
        lineEdit_3 = new QLineEdit(centralwidget);
        lineEdit_3->setObjectName(QString::fromUtf8("lineEdit_3"));
        lineEdit_3->setGeometry(QRect(130, 195, 61, 27));
        lineEdit_3->setMinimumSize(QSize(0, 24));
        lineEdit_4 = new QLineEdit(centralwidget);
        lineEdit_4->setObjectName(QString::fromUtf8("lineEdit_4"));
        lineEdit_4->setGeometry(QRect(300, 107, 61, 27));
        lineEdit_4->setMinimumSize(QSize(0, 24));
        lineEdit_5 = new QLineEdit(centralwidget);
        lineEdit_5->setObjectName(QString::fromUtf8("lineEdit_5"));
        lineEdit_5->setGeometry(QRect(300, 152, 61, 27));
        lineEdit_5->setMinimumSize(QSize(0, 24));
        lineEdit_6 = new QLineEdit(centralwidget);
        lineEdit_6->setObjectName(QString::fromUtf8("lineEdit_6"));
        lineEdit_6->setGeometry(QRect(300, 195, 61, 27));
        lineEdit_6->setMinimumSize(QSize(0, 24));
        label = new QLabel(centralwidget);
        label->setObjectName(QString::fromUtf8("label"));
        label->setGeometry(QRect(60, 110, 64, 19));
        QFont font;
        font.setFamily(QString::fromUtf8("\345\276\256\350\275\257\351\233\205\351\273\221"));
        font.setPointSize(10);
        font.setBold(false);
        font.setWeight(50);
        label->setFont(font);
        label_2 = new QLabel(centralwidget);
        label_2->setObjectName(QString::fromUtf8("label_2"));
        label_2->setGeometry(QRect(60, 156, 64, 19));
        label_2->setFont(font);
        label_3 = new QLabel(centralwidget);
        label_3->setObjectName(QString::fromUtf8("label_3"));
        label_3->setGeometry(QRect(60, 198, 64, 19));
        label_3->setFont(font);
        label_4 = new QLabel(centralwidget);
        label_4->setObjectName(QString::fromUtf8("label_4"));
        label_4->setGeometry(QRect(230, 156, 64, 19));
        label_4->setFont(font);
        label_5 = new QLabel(centralwidget);
        label_5->setObjectName(QString::fromUtf8("label_5"));
        label_5->setGeometry(QRect(230, 198, 64, 19));
        label_5->setFont(font);
        label_6 = new QLabel(centralwidget);
        label_6->setObjectName(QString::fromUtf8("label_6"));
        label_6->setGeometry(QRect(230, 110, 64, 19));
        label_6->setFont(font);
        label_7 = new QLabel(centralwidget);
        label_7->setObjectName(QString::fromUtf8("label_7"));
        label_7->setGeometry(QRect(60, 321, 64, 19));
        label_7->setFont(font);
        label_8 = new QLabel(centralwidget);
        label_8->setObjectName(QString::fromUtf8("label_8"));
        label_8->setGeometry(QRect(60, 381, 64, 19));
        label_8->setFont(font);
        label_9 = new QLabel(centralwidget);
        label_9->setObjectName(QString::fromUtf8("label_9"));
        label_9->setGeometry(QRect(60, 441, 64, 19));
        label_9->setFont(font);
        RESTART = new QPushButton(centralwidget);
        RESTART->setObjectName(QString::fromUtf8("RESTART"));
        RESTART->setGeometry(QRect(305, 500, 125, 31));
        Force_X = new QLineEdit(centralwidget);
        Force_X->setObjectName(QString::fromUtf8("Force_X"));
        Force_X->setGeometry(QRect(124, 321, 51, 24));
        Force_X->setMinimumSize(QSize(0, 24));
        Force_Y = new QLineEdit(centralwidget);
        Force_Y->setObjectName(QString::fromUtf8("Force_Y"));
        Force_Y->setGeometry(QRect(124, 380, 51, 24));
        Force_Y->setMinimumSize(QSize(0, 24));
        Force_Z = new QLineEdit(centralwidget);
        Force_Z->setObjectName(QString::fromUtf8("Force_Z"));
        Force_Z->setGeometry(QRect(124, 440, 51, 24));
        Force_Z->setMinimumSize(QSize(0, 24));
        label_10 = new QLabel(centralwidget);
        label_10->setObjectName(QString::fromUtf8("label_10"));
        label_10->setGeometry(QRect(630, 370, 31, 19));
        label_10->setFont(font);
        label_12 = new QLabel(centralwidget);
        label_12->setObjectName(QString::fromUtf8("label_12"));
        label_12->setGeometry(QRect(620, 400, 41, 19));
        label_12->setFont(font);
        axisWidget = new Axis3DWidget(centralwidget);
        axisWidget->setObjectName(QString::fromUtf8("axisWidget"));
        axisWidget->setGeometry(QRect(590, 40, 250, 250));
        lineEdit_7 = new QLineEdit(centralwidget);
        lineEdit_7->setObjectName(QString::fromUtf8("lineEdit_7"));
        lineEdit_7->setGeometry(QRect(470, 105, 61, 27));
        lineEdit_7->setMinimumSize(QSize(0, 24));
        label_11 = new QLabel(centralwidget);
        label_11->setObjectName(QString::fromUtf8("label_11"));
        label_11->setGeometry(QRect(442, 110, 31, 19));
        label_11->setFont(font);
        label_13 = new QLabel(centralwidget);
        label_13->setObjectName(QString::fromUtf8("label_13"));
        label_13->setGeometry(QRect(434, 156, 41, 19));
        label_13->setFont(font);
        label_14 = new QLabel(centralwidget);
        label_14->setObjectName(QString::fromUtf8("label_14"));
        label_14->setGeometry(QRect(437, 198, 31, 19));
        label_14->setFont(font);
        lineEdit_8 = new QLineEdit(centralwidget);
        lineEdit_8->setObjectName(QString::fromUtf8("lineEdit_8"));
        lineEdit_8->setGeometry(QRect(470, 193, 61, 27));
        lineEdit_8->setMinimumSize(QSize(0, 24));
        lineEdit_9 = new QLineEdit(centralwidget);
        lineEdit_9->setObjectName(QString::fromUtf8("lineEdit_9"));
        lineEdit_9->setGeometry(QRect(470, 150, 61, 27));
        lineEdit_9->setMinimumSize(QSize(0, 24));
        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName(QString::fromUtf8("menubar"));
        menubar->setGeometry(QRect(0, 0, 891, 23));
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName(QString::fromUtf8("statusbar"));
        MainWindow->setStatusBar(statusbar);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "MainWindow", nullptr));
        BAUDRATE->setItemText(0, QCoreApplication::translate("MainWindow", "9600", nullptr));
        BAUDRATE->setItemText(1, QCoreApplication::translate("MainWindow", "115200", nullptr));

        CONNECT->setText(QCoreApplication::translate("MainWindow", "\346\211\223\345\274\200\344\270\262\345\217\243", nullptr));
        RESET->setText(QString());
        label->setText(QCoreApplication::translate("MainWindow", "Encoder_X", nullptr));
        label_2->setText(QCoreApplication::translate("MainWindow", "Encoder_Y", nullptr));
        label_3->setText(QCoreApplication::translate("MainWindow", "Encoder_Z", nullptr));
        label_4->setText(QCoreApplication::translate("MainWindow", "Pozition_Y", nullptr));
        label_5->setText(QCoreApplication::translate("MainWindow", "Position_Z", nullptr));
        label_6->setText(QCoreApplication::translate("MainWindow", "Pozition_X", nullptr));
        label_7->setText(QCoreApplication::translate("MainWindow", "Force_X", nullptr));
        label_8->setText(QCoreApplication::translate("MainWindow", "Force_Y", nullptr));
        label_9->setText(QCoreApplication::translate("MainWindow", "Force_Z", nullptr));
        RESTART->setText(QCoreApplication::translate("MainWindow", "\345\244\215\344\275\215", nullptr));
        label_10->setText(QCoreApplication::translate("MainWindow", "\344\270\262\345\217\243", nullptr));
        label_12->setText(QCoreApplication::translate("MainWindow", "\346\263\242\347\211\271\347\216\207", nullptr));
        label_11->setText(QCoreApplication::translate("MainWindow", "Roll", nullptr));
        label_13->setText(QCoreApplication::translate("MainWindow", "Pitch", nullptr));
        label_14->setText(QCoreApplication::translate("MainWindow", "Yaw", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
