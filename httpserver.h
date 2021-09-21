#ifndef SERVER_H
#define SERVER_H

#include <QTcpServer>
#include <QTcpSocket>

class Client: public QObject
{
  QTcpSocket* socket;
  QByteArray buffer;
public:
  Client(QTcpSocket* socket);
  ~Client();
  void newMessage();
  void parseBuffer();
  void writeFile(QString fileName, int index, int length);
  void response();
};

class Server: public QTcpServer
{
public:
  Server(QObject* parent): QTcpServer(parent) {}
  void newClient();
};

#endif