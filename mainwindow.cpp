#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QMessageBox>

enum Parameters {
    AIR_FILTER = 0,
    OIL_FILTER,
    SEPARATOR,
    OIL,
    OIL_SAMPLE,
    RUN_HOURS,
    LOAD_HOURS,
    PASSWORD
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    // Create a new model before ui setup to avoid slot crash
    model = new QStandardItemModel(8,3,this);

    serial = new QSerialPort(this);
    connect(serial, &QSerialPort::readyRead, this, &MainWindow::readData);

    ui->setupUi(this);

    updatePorts();

    selectedPortIndex = 0;
    getDataIndex = 0;

    // Unit number hardcoded, will add option to change
    unitNumber = "01";

    // Get hours commands
    getCommands << "G57," << "G55," << "G56," << "G58,"
                << "G59," << "G98," << "G99," << "G45,";

    // Reset commands
    putCommands << "P57,1000," << "P55,1000," << "P56,8000,"
                << "P58,8000," << "P59,1000,";

    rowTitles << tr("Air Filter") << tr("Oil Filter")
              << tr("Separator") << tr("Oil")
              << tr("Oil Sample") << tr("Run")
              << tr("Load") << tr("Password");

    controllerData << "0" << "0" << "0" << "0"
                   << "0" << "0" << "0" << "0";

    // Reset Buttons
    airFilterButton = new QPushButton (tr("Reset"), ui->tableView);
    oilFilterButton = new QPushButton (tr("Reset"), ui->tableView);
    separatorButton = new QPushButton (tr("Reset"), ui->tableView);
    oilButton = new QPushButton (tr("Reset"), ui->tableView);
    oilAnaylsisButton = new QPushButton (tr("Reset"), ui->tableView);

    //Connect Buttons
    connect(airFilterButton, SIGNAL (released()), this, SLOT (resetAirFilter()));
    connect(oilFilterButton, SIGNAL (released()), this, SLOT (resetOilFilter()));
    connect(separatorButton, SIGNAL (released()), this, SLOT (resetSeparator()));
    connect(oilButton, SIGNAL (released()), this, SLOT (resetOil()));
    connect(oilAnaylsisButton, SIGNAL (released()), this, SLOT (resetOilSample()));

    // Setup Table View
    ui->tableView->setModel(model);
    ui->tableView->verticalHeader()->setVisible(false);
    ui->tableView->horizontalHeader()->setVisible(false);
    ui->tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableView->setSelectionMode(QAbstractItemView::NoSelection);

    // Initialize Table View row data
    for(int row = 0; row < 8; row++)
    {
        for(int col = 0; col < 2; col++)
        {
            QString labelText;

            if(col == 1)
            {
                if(row == 7)
                    labelText = QString("%1").arg(controllerData[row]);
                else
                    labelText = QString("%1 Hours").arg(controllerData[row]);
            }
            else
            {
                labelText = rowTitles[row];
            }
            QModelIndex index = model->index(row,col,QModelIndex());
            model->setData(index, labelText);
        }
    }

    // Add reset buttons to Table View
    ui->tableView->setIndexWidget (model->index (0, 2, QModelIndex () ), airFilterButton);
    ui->tableView->setIndexWidget (model->index (1, 2, QModelIndex () ), oilFilterButton);
    ui->tableView->setIndexWidget (model->index (2, 2, QModelIndex () ), separatorButton);
    ui->tableView->setIndexWidget (model->index (3, 2, QModelIndex () ), oilButton);
    ui->tableView->setIndexWidget (model->index (4, 2, QModelIndex () ), oilAnaylsisButton);

    // Setup polling timer
    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(readControllerData()));
    timer->start(2000);
    
    // Unit ID change
    connect(ui->unitID, SIGNAL(textChanged(const QString &)), this, SLOT(updateID()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_scanButton_clicked()
{
    updatePorts();
}

void MainWindow::on_actionReset_triggered()
{
    resetAll();
}

void MainWindow::updateID()
{
    unitNumber = ui->unitID->text();
}

void MainWindow::updatePorts()
{
    ui->comboBox->clear();
    portNames.clear();
    const auto infos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : infos)
    {
        portNames.append(info.portName());
    }
    ui->comboBox->addItems(portNames);
}

void MainWindow::on_comboBox_currentIndexChanged(int index)
{
    if(index < 0)
        return;
    selectedPortIndex = index;
    closeSerialPort();
    openSerialPort();
}

void MainWindow::updateHours()
{
    for(int row = 0; row < 8; row++)
    {
        QString labelText;

        if(row == 7)
            labelText = QString("%1").arg(controllerData[row]);
        else
            labelText = QString("%1 Hours").arg(controllerData[row]);

        QModelIndex index = model->index(row,1,QModelIndex());
        model->setData(index, labelText);
    }
}

void MainWindow::sendReset(int index)
{
    sendCommand(putCommands[index]);

    if(selectedPortIndex < 0)
        return;
}

void MainWindow::getControllerData(int index)
{
    sendCommand(getCommands[index]);

    if(selectedPortIndex < 0)
        return;
}

void MainWindow::resetAirFilter()
{
    sendReset(AIR_FILTER);
}

void MainWindow::resetOilFilter()
{
    sendReset(OIL_FILTER);
}

void MainWindow::resetSeparator()
{
    sendReset(SEPARATOR);
}

void MainWindow::resetOil()
{
    sendReset(OIL);
}

void MainWindow::resetOilSample()
{
    sendReset(OIL_SAMPLE);
}

void MainWindow::resetAll()
{
    for(int i = 0; i <= 4; i++){
    sendReset(i);
    }
    
    showStatusMessage(tr("All Reset"));
}

void MainWindow::openSerialPort()
{
    const QString serialPortName = portNames[selectedPortIndex];
    serial->setPortName(serialPortName);

    const int serialPortBaudRate = QSerialPort::Baud9600;
    serial->setBaudRate(serialPortBaudRate);

    if (serial->open(QIODevice::ReadWrite))
    {
        showStatusMessage(tr("Connected"));
    }
    else
    {
        QMessageBox::critical(this, tr("Error"), serial->errorString());

        showStatusMessage(tr("Open error"));
    }
}

void MainWindow::closeSerialPort()
{
    if (serial->isOpen())
        serial->close();
    showStatusMessage(tr("Disconnected"));
}

void MainWindow::writeData(const QByteArray &data)
{
    serial->write(data);
}

void MainWindow::readData()
{
    const QByteArray data = serial->readAll();

    // Data appears to come in chunks so fill a buffer until we find the end
    QString part(data);
    responseBuffer.append(part);

    // End of a response comes as a new line, parse and clear the buffer
    if(responseBuffer.contains("\r\n", Qt::CaseInsensitive)){
        QString response = responseBuffer;
        qDebug().noquote() << "Received " << response;
        response.replace("\x02", "");
        response.replace("\r\n", "");
        response = response.left(response.length() - 2);
        parseControllerData(response);
        responseBuffer = "";
    }
}

void MainWindow::handleError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::ResourceError)
    {
        QMessageBox::critical(this, tr("Critical Error"), serial->errorString());
        closeSerialPort();
    }
}

void MainWindow::showStatusMessage(const QString &message)
{
    ui->statusbar->showMessage(message);
}

void MainWindow::readControllerData()
{
    getControllerData(0);
}

void MainWindow::sendCommand(const QString &command)
{
    // Commands start with 0x02 and end with a new line
    // Checksum is on everything except 0x02
    
    QString fullMessage;
    fullMessage += unitNumber;
    fullMessage += command;
    fullMessage += checksum(fullMessage);
    fullMessage += QString("\r\n");
    fullMessage.prepend(QString("\x02"));
    writeData(fullMessage.toUtf8());
}

QString MainWindow::checksum(const QString &message)
{
    QByteArray ba = message.toLocal8Bit();
    char *c_str = ba.data();

    uint8_t sum = 0;
    char ch;
    while ((ch = *c_str++) != 0)
    {
        sum -= ch;
    }

    QString hexvalue = QString("%1").arg(sum, 2, 16, QLatin1Char( '0' ));
    return hexvalue.toUpper();
}


void MainWindow::parseControllerData(const QString &data)
{
    QStringList values = data.split(",");

    QString command = values[0];
    if(command.contains("g", Qt::CaseSensitive) && values.count() > 1)
    {
        QString hours = values[1];
        hours.replace(",", "");

        if(command.contains("g57", Qt::CaseInsensitive))
            controllerData[AIR_FILTER] = hours;

        if(command.contains("g55", Qt::CaseInsensitive))
            controllerData[OIL_FILTER] = hours;

        if(command.contains("g56", Qt::CaseInsensitive))
            controllerData[SEPARATOR] = hours;

        if(command.contains("g58", Qt::CaseInsensitive))
            controllerData[OIL] = hours;

        if(command.contains("g59", Qt::CaseInsensitive))
            controllerData[OIL_SAMPLE] = hours;

        if(command.contains("g98", Qt::CaseInsensitive))
            controllerData[RUN_HOURS] = hours;

        if(command.contains("g99", Qt::CaseInsensitive))
            controllerData[LOAD_HOURS] = hours;

        if(command.contains("g45", Qt::CaseInsensitive))
            controllerData[PASSWORD] = hours;

        // This probably needs to be out of here, move through all hours in sequence
        updateHours();

        getDataIndex++;

        if(getDataIndex <= PASSWORD)
        {
            getControllerData(getDataIndex);
        }
        else
        {
            getDataIndex = 0;
        }
    }
}
