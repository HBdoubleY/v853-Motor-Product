#ifndef LINEREADERBUFFER_H
#define LINEREADERBUFFER_H

#include "readerbuffer.h"

class LineReaderBuffer : private ReaderBuffer
{
public:
    LineReaderBuffer(int bufferSize = 4096);

    ~LineReaderBuffer();

    bool canReadLine();

    const char * readLine();

    void reset();

    void appendDatas(void *datas, int len);
private:
    int findLineEndPos(int * rightTrim = 0);
};

#endif // BUFFERREADLINE_H
