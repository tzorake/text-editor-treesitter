#include "filereader.h"
#include <QDebug>
#include <QFile>
#include <QTextStream>

FileReader::FileReader(QObject *parent) : QObject(parent)
{

}

void FileReader::readFile(const QString &filename, QString &source)
{
    source = QString();
    QFile file(filename);

    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Unable to open file";
        return;
    }

    QTextStream in(&file);
    source = in.readAll();
}
