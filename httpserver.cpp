#include <QDir>
#include <QCoreApplication>
#include "main.h"
#include "httpserver.h"

Client::Client(QTcpSocket* socket): QObject()
{
  this->socket = socket;
  connect(this->socket, &QTcpSocket::readyRead, this, &Client::newMessage);
  connect(this->socket, &QTcpSocket::disconnected, this, &Client::deleteLater);
}
//-------------------------------------------------------------------
Client::~Client()
{
  this->socket->deleteLater();
}
//-------------------------------------------------------------------
void Client::newMessage()
{
  buffer.append(socket->readAll());
  parseBuffer();
}
//-------------------------------------------------------------------
void Client::parseBuffer()
{
  int headLength = buffer.indexOf("\r\n\r\n");
  if (headLength > -1) {
    headLength = headLength + 4;
    QString head = QString::fromUtf8(buffer.left(headLength));
    //--------------------------
    QRegExp rxGet("^GET /([^ ]*) HTTP/1.1.*\r\n\r\n$");
    rxGet.setMinimal(true);
    if (rxGet.indexIn(head)==0) {
      QString fileName = rxGet.cap(1);
      if (fileName.length() > 0) {
        QString path = CURRENT_PATH + FILES_DIR + QDir::separator();
        if (QFile::exists(path + fileName)) {
          QFile file(path + fileName);
          if (file.open(QIODevice::ReadOnly)) {
            socket->write("HTTP/1.1 200 OK\r\n");
            socket->write("Content-Type: application/octet-stream\r\n");
            socket->write("Content-Disposition: attachment; filename=\"");
            socket->write(fileName.toUtf8());
            socket->write("\"\r\nContent-Length: ");
            socket->write(QString::number(file.size()).toUtf8());
            socket->write("\r\n\r\n");
            socket->write(file.readAll());
            file.close();
          }
          socket->flush();
          socket->disconnectFromHost();
          buffer.clear();
          return;
        }
      }
      response();
      return;
    }
    //--------------------------
    int contentHeadLength = buffer.indexOf("\r\n\r\n", headLength);
    if (contentHeadLength > -1) {
      contentHeadLength = contentHeadLength - headLength + 4;
      QString head = QString::fromUtf8(buffer.left(headLength + contentHeadLength));
      QRegExp rxPost("^POST /[^ ]* HTTP/1.1(?:[^\n]*\r\n)*Content-Length: ([0-9]*)\r\n(?:[^\n]*\r\n)*\r\n"
                     "(?:[^\n]*\r\n)*Content-Disposition:[^\n]*filename=\"([^\n]*)\"[^\n]*(?:[^\n]*\r\n)*\r\n$");
      rxPost.setMinimal(true);
      if (rxPost.indexIn(head)==0) {
        int contentLength = rxPost.cap(1).toInt();
        if (buffer.length() == headLength+contentLength) {
          QRegExp rxBoundary("boundary=([^\n]*)\r\n");
          rxBoundary.setMinimal(true);
          if (rxBoundary.indexIn(head) >= 0) {
            QString fileName = rxPost.cap(2);
            QString boundary = rxBoundary.cap(1);
            contentLength = buffer.length() - headLength - contentHeadLength - boundary.length() - 8;
            writeFile(fileName, headLength + contentHeadLength, contentLength);
          }
          response();
          return;
        }
      }
    }
  }
}
//-------------------------------------------------------------------
void Client::writeFile(QString fileName, const int index, const int length)
{
  if (length > 0) {
    //интересно, что будет, если в fileName прилетит последовательность ../../file
    fileName = QCoreApplication::applicationDirPath() + QDir::separator() + "files" + QDir::separator() + fileName;
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly)) {
      file.write(buffer.constData() + index, length);
      file.close();
    }
  }
}
//-------------------------------------------------------------------
void Client::response()
{
  socket->write("HTTP/1.1 200 OK\r\n");
  socket->write("Content-Type: text/html; charset=UTF-8\r\n");
  socket->write("Connection: close\r\n");
  socket->write("\r\n");
  socket->write("<!DOCTYPE html>\r\n");
  socket->write("<html><head></head><body>");
  socket->write("<form action=\"/\" method=\"POST\" enctype=\"multipart/form-data\">");
  socket->write("<p><input type=\"file\" id=\"myfile\" name=\"myfile\"></p>");
  socket->write("<p><button type=\"submit\">upload new file</button></p>");
  socket->write("</form>");
  QDir dir(CURRENT_PATH + FILES_DIR);
  QStringList list = dir.entryList(QDir::Files, QDir::Name);
  for (QString file : list) {
    socket->write("<p><a href=\"/");
    socket->write(file.toUtf8());
    socket->write("\">");
    socket->write(file.toUtf8());
    socket->write("</a></p>");
  }
  socket->write("</body></html>");
  socket->flush();
  socket->disconnectFromHost();
  buffer.clear();
}
//===================================================================
//===================================================================
//===================================================================
void Server::newClient()
{
  new Client(this->nextPendingConnection());
}