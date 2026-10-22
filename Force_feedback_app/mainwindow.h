#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QByteArray>
#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

//私有槽函数
private slots:
    void CONNECT_ctrl();
    void RESET_ctrl();
    void readSerialData();

private:
    Ui::MainWindow *ui;
    QByteArray m_serialBuffer;
    void handleCommand(const QString &command);
};
#endif // MAINWINDOW_H
