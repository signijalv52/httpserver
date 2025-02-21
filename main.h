#ifndef MAIN_H
#define MAIN_H

#define CURRENT_PATH QCoreApplication::applicationDirPath() + QDir::separator()
#define FILES_DIR    "files"

#define COUNT_READ_MAX 2048 //порция чтения данных от клиента
#define COUNT_HEAD_MAX 2048 //лимит чтения для заголовка или части заголовка

#define FILE_BUFFER_SIZE 1024 //размер буфера при передачи файла

#endif