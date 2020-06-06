#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTableView>
#include <QItemDelegate>
#include <QStandardItemModel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QTimer>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    int selectedPortIndex;
    int getDataIndex;
    Ui::MainWindow *ui;
    QPushButton *airFilterButton;
    QPushButton *oilFilterButton;
    QPushButton *separatorButton;
    QPushButton *oilButton;
    QPushButton *oilAnaylsisButton;
    QSerialPort *serial = nullptr;
    QStandardItemModel *model;
    QString unitNumber;
    QString responseBuffer;
    QStringList rowTitles;
    QStringList portNames;
    QStringList putCommands;
    QStringList getCommands;
    QStringList controllerData;
    void updatePorts();
    void updateHours();
    void sendReset(int index);
    void getControllerData(int index);
    void sendCommand(const QString &command);
    void showStatusMessage(const QString &message);
    void initActionsConnections();
    void parseControllerData(const QString &data);
    QString checksum(const QString &message);

private slots:
    void on_scanButton_clicked();
    void on_actionReset_triggered();
    void on_comboBox_currentIndexChanged(int index);
    void updateID();
    void resetAirFilter();
    void resetOilFilter();
    void resetSeparator();
    void resetOil();
    void resetOilSample();
    void resetAll();
    void openSerialPort();
    void closeSerialPort();
    //void about();
    void writeData(const QByteArray &data);
    void readData();
    void readControllerData();
    void handleError(QSerialPort::SerialPortError error);

};
#endif // MAINWINDOW_H
