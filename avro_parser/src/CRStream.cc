
#include "CRStream.hh"

bool CRInputStream::next(const uint8_t** data, size_t* size) {
    if (available_ == 0 && ! fill()) {
        return false;
    }
    *data = next_;
    *size = available_;
    next_ += available_;
    byteCount_ += available_;
    available_ = 0;
    return true;
}

void CRInputStream::backup(size_t len) {
    next_ -= len;
    available_ += len;
    byteCount_ -= len;
}

void CRInputStream::skip(size_t len) {
    while (len > 0) {
        if (available_ == 0) {
            cr_.seek(len);
            byteCount_ += len;
            return;
        }
        size_t n = std::min(available_, len);
        available_ -= n;
        next_ += n;
        len -= n;
        byteCount_ += n;
    }
}

size_t CRInputStream::byteCount() const { return byteCount_; }

bool CRInputStream::fill() {
    size_t n = 0;
    if ((n = cr_.read((void*)buffer_, bufferSize_))) {
        next_ = buffer_;
        available_ = n;
        return true;
    }
    return false;
}
