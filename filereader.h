#ifndef FILEREADER_H
#define FILEREADER_H

#include <QObject>

class FileReader : public QObject
{
    Q_OBJECT
public:
    static bool readFile(const QString &filename, QString &source);

private:
    explicit FileReader(QObject *parent = nullptr);
};

#endif // FILEREADER_H
