#ifndef SERVER_H
#define SERVER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QFile>

class Client: public QObject
{
Q_OBJECT
  QTcpSocket* socket;
  QByteArray buffer;

  qsizetype headLength = 3;
  qsizetype postHeadLength = 3;

  qint64 totalLength = 0;
  qint64 writedLength = 0;
  QFile writeFile;

public:
  Client(QTcpSocket* socket);
  void newMessage();
  void parseBuffer();
  void response();
signals:
  void hasMoreData();
  void percentChanged(int);
};

class Server: public QTcpServer
{
Q_OBJECT
public:
  Server(QObject* parent): QTcpServer(parent) {}
  void newClient();
signals:
  void clientPercentChanged(int);
};

#endif