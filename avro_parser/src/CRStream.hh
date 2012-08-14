#ifndef avro_CRStream_hh__
#define avro_CRStream_hh__

#include <avro/Stream.hh>
#include "CoroutineHelpers.h"
#include "Vertica.h"

using namespace avro;

class AVRO_DECL CRInputStream : public InputStream { 
    const size_t bufferSize_;
    uint8_t* const buffer_;
    ContinuousReader &cr_;
    size_t byteCount_;
    uint8_t* next_;
    size_t available_;


    bool next(const uint8_t** data, size_t* size);

    void backup(size_t len);

    void skip(size_t len);

    size_t byteCount() const;

    bool fill();
    
public:
    CRInputStream(ContinuousReader& cr, size_t bufferSize = 8*1024) :
        bufferSize_(bufferSize),
        buffer_(new uint8_t[bufferSize]),
        cr_(cr),
        byteCount_(0),
        next_(buffer_),
        available_(0) { }

    ~CRInputStream() {
        delete[] buffer_;
    }
    

};

#endif //CRStream_cc__
