#include "subscriptionwindow.h"
#include "ui_subscriptionwindow.h"

SubscriptionWindow::SubscriptionWindow(QMqttSubscription *sub, QWidget *parent) :
  QWidget(parent),
  ui(new Ui::SubscriptionWindow),
  m_sub(sub)
{
  ui->setupUi(this);

  ui->labelSub->setText(m_sub->topic().filter());
  ui->labelQoS->setText(QString::number(m_sub->qos()));
  updateStatus(m_sub->state());

  bool ok{false};

  ok = connect(m_sub, &QMqttSubscription::messageReceived, this, &SubscriptionWindow::updateMessage);
  Q_ASSERT(ok);

  ok = connect(m_sub, &QMqttSubscription::stateChanged, this, &SubscriptionWindow::updateStatus);
  Q_ASSERT(ok);

  ok = connect(ui->pushButton, &QAbstractButton::clicked, m_sub, &QMqttSubscription::unsubscribe);
  Q_ASSERT(ok);

  ok =connect(m_sub, &QMqttSubscription::qosChanged, [this](quint8 qos)
  {
    ui->labelQoS->setText(QString::number(qos));
  });
  Q_ASSERT(ok);
}

SubscriptionWindow::~SubscriptionWindow()
{
  m_sub->unsubscribe();
  delete ui;
}

void SubscriptionWindow::updateMessage(const QMqttMessage &msg)
{
  ui->listWidget->addItem(msg.payload());
}

void SubscriptionWindow::updateStatus(QMqttSubscription::SubscriptionState state)
{
  switch (state)
  {
    case QMqttSubscription::Unsubscribed:
      ui->labelStatus->setText(QLatin1String("Unsubscribed"));
      break;

    case QMqttSubscription::SubscriptionPending:
      ui->labelStatus->setText(QLatin1String("Pending"));
      break;

    case QMqttSubscription::Subscribed:
      ui->labelStatus->setText(QLatin1String("Subscribed"));
      break;

    case QMqttSubscription::Error:
      ui->labelStatus->setText(QLatin1String("Error"));
      break;

    default:
      ui->labelStatus->setText(QLatin1String("--Unknown--"));
      break;
  }
}
