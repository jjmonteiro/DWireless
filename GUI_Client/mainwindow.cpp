#include "mainwindow.h"
#include "subscriptionwindow.h"
#include "ui_mainwindow.h"

#include <QtCore/QDateTime>
#include <QtMqtt/QMqttClient>
#include <QtWidgets/QMessageBox>

MainWindow::MainWindow(QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::MainWindow)
{
  ui->setupUi(this);

  m_client = new QMqttClient(this);
  m_client->setHostname(ui->lineEditHost->text());
  m_client->setPort(ui->spinBoxPort->value());
  m_client->setUsername("lgnnlude");
  m_client->setPassword("iWg6ghr1pMLO");
  m_client->setHostname("m20.cloudmqtt.com");
  m_client->setPort(17283);
  m_client->setClientId("Ruben_pc");

  bool ok{false};

  ok = connect(ui->buttonConnect, &QPushButton::pressed, this, &MainWindow::on_buttonConnect_clicked);
  Q_ASSERT(ok);

  ok = connect(m_client, &QMqttClient::stateChanged, this, &MainWindow::updateLogStateChange);
  Q_ASSERT(ok);

  ok = connect(m_client, &QMqttClient::disconnected, this, &MainWindow::brokerDisconnected);
  Q_ASSERT(ok);

  ok = connect(m_client, &QMqttClient::messageReceived, this, [this](const QByteArray &message, const QMqttTopicName &topic)
  {
    const QString content = QDateTime::currentDateTime().toString()
        + QLatin1String(" Received Topic: ")
        + topic.name()
        + QLatin1String(" Message: ")
        + message
        + QLatin1Char('\n');
    ui->editLog->insertPlainText(content);
  });
  Q_ASSERT(ok);


  connect(m_client, &QMqttClient::pingResponseReceived, this, [this]()
  {
    ui->buttonPing->setEnabled(true);
    const QString content = QDateTime::currentDateTime().toString()
        + QLatin1String(" PingResponse")
        + QLatin1Char('\n');
    ui->editLog->insertPlainText(content);
  });
  Q_ASSERT(ok);

  connect(ui->lineEditHost, &QLineEdit::textChanged, m_client, &QMqttClient::setHostname);
  Q_ASSERT(ok);

  connect(ui->spinBoxPort, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::setClientPort);
  Q_ASSERT(ok);

  updateLogStateChange();
}

MainWindow::~MainWindow()
{
  delete ui;
  qApp->quit();
}

void MainWindow::on_buttonConnect_clicked()
{
  if (m_client->state() == QMqttClient::Disconnected)
  {
    m_client->setUsername("lgnnlude");
    m_client->setPassword("iWg6ghr1pMLO");
    m_client->setHostname("m20.cloudmqtt.com");
    m_client->setPort(17283);
    m_client->setClientId("Ruben_pc");
    m_client->connectToHost();
  }
  else if(m_client->state() == QMqttClient::Connected)
  {
    m_client->disconnectFromHost();
  }
}

void MainWindow::on_buttonQuit_clicked()
{
  QApplication::quit();
}

void MainWindow::updateLogStateChange()
{
  QString state = "";
  switch(m_client->state())
  {
    case QMqttClient::Connected:
      ui->lineEditHost->setEnabled(false);
      ui->spinBoxPort->setEnabled(false);
      ui->buttonConnect->setEnabled(true);
      ui->buttonConnect->setText(tr("Disconnect"));
      state = "Connected";
      break;

    case QMqttClient::Disconnected:
      ui->lineEditHost->setEnabled(false);
      ui->spinBoxPort->setEnabled(true);
      ui->buttonConnect->setEnabled(true);
      ui->buttonConnect->setText(tr("Connect"));
      state = "Disconnected";
      break;

    case QMqttClient::Connecting:
      ui->lineEditHost->setEnabled(false);
      ui->spinBoxPort->setEnabled(true);
      ui->buttonConnect->setEnabled(false);
      state = "Connecting";
  }

  const QString content = QDateTime::currentDateTime().toString() + QLatin1String(": State Change: ") + state + QLatin1Char('\n');
  ui->editLog->insertPlainText(content);
}

void MainWindow::brokerDisconnected()
{
  ui->lineEditHost->setEnabled(true);
  ui->spinBoxPort->setEnabled(true);
  ui->buttonConnect->setText(tr("Connect"));
}

void MainWindow::setClientPort(int p)
{
  m_client->setPort(p);
}

void MainWindow::on_buttonPublish_clicked()
{
  if (m_client->publish(ui->lineEditTopic->text(),
                        ui->lineEditMessage->text().toUtf8(),
                        ui->spinQoS_2->text().toUInt(),
                        ui->checkBoxRetain->isChecked()) == -1)
    QMessageBox::critical(this, QLatin1String("Error"), QLatin1String("Could not publish message"));
}

void MainWindow::on_buttonSubscribe_clicked()
{
  auto subscription = m_client->subscribe(ui->lineEditTopic->text(), ui->spinQoS->text().toUInt());
  if (!subscription)
  {
    QMessageBox::critical(this, QLatin1String("Error"), QLatin1String("Could not subscribe. Is there a valid connection?"));
    return;
  }
  auto subWindow = new SubscriptionWindow(subscription);
  subWindow->setWindowTitle(subscription->topic().filter());
  subWindow->show();
}

void MainWindow::on_buttonPing_clicked()
{
  ui->buttonPing->setEnabled(false);
  m_client->requestPing();
}
