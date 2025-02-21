//cmake -S . -B .
//make

#include <QApplication>
#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QFileInfo>
#include <QDir>
#include "main.h"
#include "httpserver.h"

class Window: public QWidget
{
public:
  Window(QHostAddress& address, int port): QWidget(nullptr)
  {
    QPushButton* exitButton = new QPushButton("exit");
    connect(exitButton, &QPushButton::clicked, this, &Window::close);
    QVBoxLayout* layout = new QVBoxLayout;
    layout->addWidget(exitButton);
    setLayout(layout);
    resize(300, 200);
    Server* server = new Server(this);
    if (!server->listen(address, port)) {
      QMessageBox::critical(this, "", "!tcpServer->listen()");
      return;
    }
    connect(server, &Server::newConnection, server, &Server::newClient);
    QLineEdit* addressLine = new QLineEdit("http://" + address.toString() + ":" + QString::number(port) + "/");
    QLabel* info = new QLabel();
    layout->insertWidget(0, info);
    layout->insertWidget(0, addressLine);
    connect(server, &Server::clientPercentChanged, [info](int newValue){
      if (newValue == 0)
        info->setText("");
      else
        info->setText(QString::number(newValue) + "%");
    });
  }
};

int main(int argc, char** argv)
{
  if (argc != 3)
    return 0;
  QByteArray arg(argv[1]);
  QString arg1(arg);
  QHostAddress address;
  if (!address.setAddress(arg1))
    return 0;
  arg.clear();
  arg.append(argv[2]);
  QString arg2(arg);
  int port = arg2.toInt();
  QApplication app(argc, argv);
  if (!QFileInfo::exists(CURRENT_PATH + FILES_DIR))
    QDir(CURRENT_PATH).mkdir(FILES_DIR);
  Window window(address, port);
  window.show();
  return app.exec();
}
