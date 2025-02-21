#include <QDir>
#include <QCoreApplication>
#include "main.h"
#include "httpserver.h"

#include <QRegularExpression>

Client::Client(QTcpSocket* socket): QObject()
{
  this->socket = socket;
  connect(socket, &QTcpSocket::readyRead, this, &Client::newMessage);
  connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
  connect(socket, &QTcpSocket::disconnected, this, &Client::deleteLater);
  connect(this, &Client::hasMoreData, this, &Client::newMessage);
}
//-------------------------------------------------------------------
void Client::newMessage()
{
  QByteArray data = socket->read(COUNT_READ_MAX);
  if (!data.isNull()) {
    buffer.append(data);
    if (!socket->atEnd())
      emit hasMoreData();
    else
      parseBuffer();
  }
}
//-------------------------------------------------------------------
void Client::parseBuffer()
{
  if (headLength == 3) {
    headLength = buffer.indexOf("\r\n\r\n") + 4;
    if (headLength > 3) {
      QString head = QString::fromUtf8(buffer.constData(), headLength);
      QRegularExpression rxGet("^GET /([^ ]*) HTTP/1\\.1[\\s\\S]*?\r\n\r\n$");
      QRegularExpressionMatch matchGet = rxGet.match(head);
      if (matchGet.hasMatch()) {
        //обработка запроса GET
        QString getFileName = matchGet.captured(1);
        if (getFileName.length() > 0) {
          QString allowedDir = CURRENT_PATH + FILES_DIR + QDir::separator();
          QString fileCanonical = QFileInfo(allowedDir + getFileName).canonicalFilePath();
          if (!fileCanonical.isEmpty() && fileCanonical.startsWith(allowedDir) && allowedDir+getFileName == fileCanonical) {
            QFile file(fileCanonical);
            if (file.open(QIODevice::ReadOnly)) {
              socket->write("HTTP/1.1 200 OK\r\n");
              socket->write("Content-Type: application/octet-stream\r\n");
              socket->write("Content-Disposition: attachment; filename=\"");
              socket->write(getFileName.toUtf8());
              socket->write("\"\r\nContent-Length: ");
              socket->write(QString::number(file.size()).toUtf8());
              socket->write("\r\n\r\n");
              QByteArray fileBuffer(FILE_BUFFER_SIZE, Qt::Uninitialized);
              while (!file.atEnd()) {
                qint64 count = file.read(fileBuffer.data(), FILE_BUFFER_SIZE);
                if (count > 0) {
                  socket->write(fileBuffer.constData(), count);
                  socket->flush();
                }
              }
              file.close();
              socket->disconnectFromHost();
              return;
            }
          }
        }
        response();
        return;
      }
      //тут выход на проверку POST
    } else {
      if (buffer.size() > COUNT_HEAD_MAX)
        //превышен лимит чтения, завершаем сеанс
        response();
      //прилетело мало байтов, ждём ещё
      return;
    }
  }

  //--------------------------
  if (postHeadLength == 3) {
    postHeadLength = buffer.indexOf("\r\n\r\n", headLength) + 4;
    if (postHeadLength > 3) {
      //первая и вторая часть какогото заголовка (ожидаем только POST) получены
      QString postHead = QString::fromUtf8(buffer.constData(), postHeadLength);
      QRegularExpression rxPost("^POST /[^ ]* HTTP/1\\.1(?:[^\r\n]*?\r\n)*Content-Length: ([0-9]*)\r\n(?:[^\r\n]*?\r\n)*\r\n$");
      //(!) boundary не всегда находится в конце строки
      QRegularExpression rxBoundary("Content-Type:[^\r\n]*?boundary=([^\r\n]*?)\r\n");
      //(!) filename может быть без кавычек
      QRegularExpression rxFile("Content-Disposition:[^\r\n]*?filename=[^\"]*?\"([^\r\n]*?)\"[^\r\n]*?\r\n");
      QRegularExpressionMatch matchPost = rxPost.match(postHead);
      QRegularExpressionMatch matchBoundary = rxBoundary.match(postHead);
      QRegularExpressionMatch matchFile = rxFile.match(postHead);
      if (matchPost.hasMatch() && matchBoundary.hasMatch() && matchFile.hasMatch()) {
        int contentLength = matchPost.captured(1).toInt();
        int boundaryLength = matchBoundary.captured(1).length();
        QString postFileName = matchFile.captured(1);
        if (!postFileName.contains(QRegularExpression("^[a-zA-Z0-9._-]+$"))) {
          //проверка, не шлёт ли клиент какуюто дичь 
          response();
          return;
        }
        totalLength = contentLength + headLength - postHeadLength - boundaryLength - 8;
        buffer.remove(0, postHeadLength);
        postFileName = QCoreApplication::applicationDirPath() + QDir::separator() + "files" + QDir::separator() + postFileName;
        writeFile.setFileName(postFileName);
        if (!writeFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
          //проблема с открытием файла, завершаем сеанс
          response();
          return;
        }
      } else {
        //проблема с парсингом POST заголовка, завершаем сеанс
        response();
        return;
      }
    } else {
      if (buffer.size() > COUNT_HEAD_MAX)
        //превышен лимит чтения для первой + второй первой части заголовка POST, завершаем сеанс
        //всего ожидаем 2 части, каждая завершается последовательностью \r\n\r\n
        response();
      return; //прилетело мало байтов, ждём ещё
    }
  }
  qsizetype size = buffer.size();
  if (totalLength - writedLength < size) {
    size = totalLength - writedLength;
  }
  writedLength = writedLength + writeFile.write(buffer.constData(), size);
  emit percentChanged((writedLength * 100) / totalLength);
  buffer.clear();
  if (writedLength < totalLength) {
    //не весь файл записан, ждём ещё
    return;
  } else {
    //всё
    writeFile.close();
  }
  response();
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

  emit percentChanged(0);
}
//===================================================================
//===================================================================
//===================================================================
void Server::newClient()
{
  Client* client = new Client(this->nextPendingConnection());
  connect(client, &Client::percentChanged, this, &Server::clientPercentChanged);
}
