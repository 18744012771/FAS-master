#include <errno.h>
#include <sys/uio.h>
#include <sys/types.h>


#include <Buffer.h>
#include <Socket.h>


#include <boost/implicit_cast.hpp>

const char fas::Buffer::kCRLF[] = "\r\n";
const size_t fas::Buffer::kCheapPrepend;
//构造函数
fas::Buffer::Buffer(size_t initialSize) :
    initialSize_(initialSize),
    buffer_(kCheapPrepend + initialSize_),
    readerIndex_(kCheapPrepend),
    writerIndex_(kCheapPrepend) {
    assert(readableBytes() == 0);
    assert(writableBytes() == initialSize_);
    assert(prependableBytes() == kCheapPrepend);
}
//rhs.readerIndex_ 私有变量怎么访问，功能类似于拷贝
void fas::Buffer::swap(Buffer& rhs) {
    buffer_.swap(rhs.buffer_);
    std::swap(readerIndex_, rhs.readerIndex_);
    std::swap(writerIndex_, rhs.writerIndex_);
}

//返回buffer中可读的大小
size_t fas::Buffer::readableBytes() const {
    return writerIndex_ - readerIndex_;
}
//返回buffer中可写大小
size_t fas::Buffer::writableBytes() const {
    return buffer_.size() - writerIndex_;
}
//返回buffer中已读大小，这些空间可以用来写新内容
size_t fas::Buffer::prependableBytes() const {
    return readerIndex_;
}
//返回buffer中可以读新内容的位置
const char* fas::Buffer::peek() const {
    return begin() + readerIndex_;
}

//找到peek()开始第一个kcrtl
const char* fas::Buffer::findCRLF() const {
    const char* crlf = std::search(peek(), beginWrite(), kCRLF, kCRLF+2);
    return crlf == beginWrite() ? NULL : crlf;
}

const char* fas::Buffer::findCRLF(const char* start) const {
    assert(peek() <= start);
    assert(start <= beginWrite());
    const char* crlf = std::search(start, beginWrite(), kCRLF, kCRLF+2);
    return crlf == beginWrite() ? NULL : crlf;
}

const char* fas::Buffer::findChars(const char* chars, size_t chars_len) const {
    const char* pos = std::search(peek(), beginWrite(), chars, chars+chars_len);
    return pos == beginWrite() ? NULL : pos;
}
//恢复len空间，可读索引后移
void fas::Buffer::retrieve(size_t len) {
    assert(len <= readableBytes());
    if (len < readableBytes()) {
        readerIndex_ += len;
    }
    else {
        retrieveAll();
    }
}

void fas::Buffer::retrieveUntil(const char* end) {
    assert(peek() <= end);
    assert(end <= beginWrite());
    retrieve(end - peek());
}

void fas::Buffer::retrieveInt32() {
    retrieve(sizeof(int32_t));
}

void fas::Buffer::retrieveInt16() {
    retrieve(sizeof(int16_t));
}

void fas::Buffer::retrieveInt8() {
    retrieve(sizeof(int8_t));
}

//恢复所有空间
void fas::Buffer::retrieveAll() {
    readerIndex_ = kCheapPrepend;
    writerIndex_ = kCheapPrepend;
}
//恢复整个可读空间，并返回可读空间的string
string fas::Buffer::retrieveAllAsString() {
    return retrieveAsString(readableBytes());;
}
//恢复len并获取恢复空间的string
string fas::Buffer::retrieveAsString(size_t len) {
    assert(len <= readableBytes());
    string result(peek(), len);
    retrieve(len);
    return result;
}

//将data数据copy到可写位置
void fas::Buffer::append(const char* /*restrict*/ data, size_t len) {
    ensureWritableBytes(len);
    std::copy(data, data+len, beginWrite());
    hasWritten(len);
}

void fas::Buffer::append(const void* /*restrict*/ data, size_t len) {
    append(static_cast<const char*>(data), len);
}
//如果可写空间不足，开辟空间
void fas::Buffer::ensureWritableBytes(size_t len) {
    if (writableBytes() < len) {
        makeSpace(len);
    }
    assert(writableBytes() >= len);
}
//可写位置
char* fas::Buffer::beginWrite() {
    return begin() + writerIndex_;
}
//可写位置
const char* fas::Buffer::beginWrite() const {
    return begin() + writerIndex_;
}
//写完数据后可写下标后移
void fas::Buffer::hasWritten(size_t len) {
    writerIndex_ += len;
}

void fas::Buffer::appendInt32(int32_t x) {
    int32_t be32 = sockets::hostToNetwork32(x);
    append(&be32, sizeof be32);
}

void fas::Buffer::appendInt16(int16_t x) {
    int16_t be16 = sockets::hostToNetwork16(x);
    append(&be16, sizeof be16);
}

void fas::Buffer::appendInt8(int8_t x) {
    append(&x, sizeof x);
}
//读32大小的数据，读完之后恢复空间
int32_t fas::Buffer::readInt32() {
    int32_t result = peekInt32();
    retrieveInt32();
    return result;
}

int16_t fas::Buffer::readInt16() {
    int16_t result = peekInt16();
    retrieveInt16();
    return result;
}

int8_t fas::Buffer::readInt8() {
    int8_t result = peekInt8();
    retrieveInt8();
    return result;
}
//语法有问题  读32大小的数据
int32_t fas::Buffer::peekInt32() const {
    assert(readableBytes() >= sizeof(int32_t));
    int32_t be32 = 0;
    ::memcpy(&be32, peek(), sizeof be32);
    return sockets::networkToHost32(be32);
}

int16_t fas::Buffer::peekInt16() const {
    assert(readableBytes() >= sizeof(int16_t));
    int16_t be16 = 0;
    ::memcpy(&be16, peek(), sizeof be16);
    return sockets::networkToHost16(be16);
}

int8_t fas::Buffer::peekInt8() const {
    assert(readableBytes() >= sizeof(int8_t));
    int8_t x = *peek();
    return x;
}
//在可读下标前插入32
void fas::Buffer::prependInt32(int32_t x) {
    int32_t be32 = sockets::hostToNetwork32(x);
    prepend(&be32, sizeof be32);
}

void fas::Buffer::prependInt16(int16_t x) {
    int16_t be16 = sockets::hostToNetwork16(x);
    prepend(&be16, sizeof be16);
}

void fas::Buffer::prependInt8(int8_t x) {
    prepend(&x, sizeof x);
}
//在可读下标前插入
void fas::Buffer::prepend(const void* /*restrict*/ data, size_t len) {
    assert(len <= prependableBytes());
    readerIndex_ -= len;
    const char* d = static_cast<const char*>(data);
    std::copy(d, d+len, begin()+readerIndex_);
}

ssize_t fas::Buffer::readFd(int fd, int* savedErrno) {
    // saved an ioctl()/FIONREAD call to tell how much to read
    char extrabuf[65536];
    struct iovec vec[2];
    const size_t writable = writableBytes();
    vec[0].iov_base = begin()+writerIndex_;
    vec[0].iov_len = writable;
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof extrabuf;
    const ssize_t n = ::readv(fd, vec, 2);
    if (n < 0) {
        *savedErrno = errno;
		
    } else if (boost::implicit_cast<size_t>(n) <= writable)//boost::implicit_cast隐式转换更加安全
	{
        writerIndex_ += n;
    } else {
        writerIndex_ = buffer_.size();
        append(extrabuf, n - writable);
    }
    return n;
}

char* fas::Buffer::begin() {
    return &*buffer_.begin();
}

const char* fas::Buffer::begin() const {
    return &*buffer_.begin();
}

void fas::Buffer::makeSpace(size_t len) {    //
    if (writableBytes() + prependableBytes() < len + kCheapPrepend) {
        // FIXME: move readable data
        buffer_.resize(writerIndex_+len);
    } else {
        // move readable data to the front, make space inside buffer
        assert(kCheapPrepend < readerIndex_);
        size_t readable = readableBytes();
        std::copy(begin()+readerIndex_,
                begin()+writerIndex_,
                begin()+kCheapPrepend);
        readerIndex_ = kCheapPrepend;
        writerIndex_ = readerIndex_ + readable;
        assert(readable == readableBytes());
    }
}


