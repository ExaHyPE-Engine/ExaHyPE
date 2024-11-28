#pragma once

#include <aio.h>
#include <fcntl.h>
#include <unistd.h>

#include <lang/assert.h>
#include <lang/type.h>

constexpr i32 DMA_FLAGS = O_RDWR | O_CREAT | O_NONBLOCK | O_TRUNC | O_DIRECT;
// O_DIRECT works only when all of the above are true:
//   - buffer is BLOCK-aligned (typically 512 bytes) this happens automatically for ~1kiB+ allocations
//   - length to be written is divisible by BLOCK-size
//   - offset is divisible by BLOCK-size

template<u64 T_BUFF_SIZE = 1024 * 1024 * 1, u8 WIDTH = 4, i32 BLOCK = 512>
class DmaWriter {
    static constexpr u64 BUFF_SIZE() {
        return T_BUFF_SIZE / WIDTH;
    }

    // for fast path the buffer size has to be a multiple of BLOCK size
    static_assert(T_BUFF_SIZE % BLOCK == 0);

    static_assert(BUFF_SIZE() >= BLOCK);

    aiocb *cbs = nullptr;

    u64 len = 0;
    u64 offset = 0;
    u64 max_len = 0;
    u32 prefill = 0;
    i32 fd = -1;
    u8 idx = 0;

    aiocb *getCb() {
        return this->cbs + this->idx;
    }

    aiocb *pickNextCb() {
        auto localIdx = (this->idx + 1) % WIDTH;

        while (true) {
            auto *cbCandidate = this->cbs + localIdx;

            if (cbCandidate->aio_fildes == -1) [[unlikely]] {
                this->idx = localIdx;

                return cbCandidate;
            }

            auto error = aio_error(cbCandidate);
            if (error != EINPROGRESS) [[unlikely]] {
                assert(error == 0)

                auto bytes = aio_return(cbCandidate);
                assert(bytes == cbCandidate->aio_nbytes)

                this->idx = localIdx;
                cbCandidate->aio_fildes = -1;

                return cbCandidate;
            }

            localIdx = (localIdx + 1) % WIDTH;
        }
    }

    void syncCb(aiocb *cb) {
        if (cb->aio_fildes < 0) [[unlikely]] return;
        auto error = aio_error(cb);

        while (error == EINPROGRESS) {
            error = aio_error(cb);
        }

        assert(error == 0)

        auto bytes = aio_return(cb);
        assert(bytes == cb->aio_nbytes)
    }

    /*
     * We have to adhere to the BLOCK boundaries
     */
    void flushCb(aiocb *cb) {
        cb->aio_fildes = this->fd;
        cb->aio_offset = this->offset;

        this->max_len = std::max(this->max_len, this->offset + this->len);

        auto postfill = (BLOCK - this->len % BLOCK) % BLOCK;

        if (this->prefill == 0 & postfill == 0) [[likely]] {  // fast path, we're updating whole block(s)
            cb->aio_nbytes = this->len;

            auto code = aio_write(cb);
            assert(code == 0)

            this->offset += this->len;
            this->len = 0;

            return;
        }

        alignas(BLOCK) char buf[BLOCK] = {};

        if (this->prefill > 0) {
            auto bytes = pread(this->fd, buf, BLOCK, this->offset);
            assert(bytes >= 0) // usually bytes == this->prefill unless seek() is called before any writes

            memcpy((char *) cb->aio_buf, buf, this->prefill);
        }

        if (postfill == 0) {
            cb->aio_nbytes = this->len;

            auto code = aio_write(cb);
            assert(code == 0)

            this->offset += this->len;
            this->len = 0;

            return;
        }

        if (this->len + postfill > BLOCK | this->prefill == 0) {
            // write spans multiple buffers (or prefill is zero), cannot reuse buf
            auto bytes = pread(this->fd, buf, BLOCK, this->offset);
            assert(bytes >= 0)
        }

        memcpy((char *) cb->aio_buf + this->len, buf + BLOCK - postfill, postfill);

        cb->aio_nbytes = this->len + postfill;

        auto code = aio_write(cb);
        assert(code == 0)

        this->offset += this->len + postfill - BLOCK;
        this->prefill = BLOCK - postfill;
        this->len = this->prefill;
    }

    void flushCbAsync() {
        if (this->len == 0 | this->len == this->prefill) [[unlikely]] return;

        this->flushCb(this->getCb());

        this->pickNextCb();
    }

    char *getBuf() {
        return ((char *) this->getCb()->aio_buf) + this->len;
    }

    struct alignas(BLOCK) BUF {
        char b[BUFF_SIZE()];
    };

public:
    explicit DmaWriter(i32 fd) noexcept: fd(fd), cbs(nullptr) {
        this->cbs = new aiocb[WIDTH];
        std::memset(this->cbs, 0, sizeof(aiocb) * WIDTH);
        for (i32 i = 0; i < WIDTH; i++) {
            this->cbs[i].aio_fildes = -1;
            this->cbs[i].aio_buf = new BUF;
            assert(((u64) this->cbs[i].aio_buf) % BLOCK == 0) // buffers must be BLOCK-aligned
        }
    }

    explicit DmaWriter() = default;

    DmaWriter(const DmaWriter &) = delete;

    DmaWriter& operator=(DmaWriter&& other) noexcept {
        if (this->cbs) {
            for (i32 i = 0; i < WIDTH; i++) delete (BUF *) this->cbs[i].aio_buf;
            delete[] this->cbs;
        }
        this->cbs = other.cbs;

        this->len = other.len;
        this->offset = other.offset;
        this->max_len = other.max_len;
        this->prefill = other.prefill;
        this->fd = other.fd;
        this->idx = other.idx;

        other.cbs = nullptr;
        other.fd = -1;

        return *this;
    }

    template<typename T>
    requires (!std::is_pointer_v<T>)
    void write(T val) {
        static_assert(sizeof(T) <= BUFF_SIZE());

        if (this->len + sizeof(T) <= BUFF_SIZE()) [[likely]] {
            *((T *) this->getBuf()) = val;
            this->len += sizeof(T);
            return;
        }

        this->write(&val, sizeof(T));
    }

    void write(std::string_view s) {
        this->write(s.data(), s.length());
    }

    void write(const void *ptr, u64 n) {
        auto *data = (char *) ptr;
        auto freeCap = BUFF_SIZE() - this->len;

        do {
            auto bytesToWrite = std::min(n, freeCap);
            memcpy(this->getBuf(), data, bytesToWrite);

            if (this->len == BUFF_SIZE()) this->flushCbAsync();

            data += bytesToWrite;
            n -= bytesToWrite;
            this->len += bytesToWrite;
            freeCap = BUFF_SIZE() - this->len;
        } while (n > 0);
    }

    void seek(u64 pos) {
        this->sync();

        this->prefill = pos % BLOCK;
        this->offset = pos - this->prefill;
        assert(this->offset % BLOCK == 0)
        this->len = this->prefill;
    }

    void *buffer(u64 n) {
        if (this->len + n > BUFF_SIZE()) [[unlikely]] return nullptr;
        auto *buf = this->getBuf();
        this->len += n;
        return buf;
    }

    void sync() {
        this->flushCbAsync();
        i32 localIdx = this->idx;
        for (i32 i = 1; i < WIDTH; i++) {
            this->syncCb(this->cbs + (localIdx + i) % WIDTH);
        }
        auto code = ftruncate(this->fd, this->max_len);
        assert(code == 0)
        code = syncfs(this->fd);
        assert(code == 0)
    }

    u64 getPos() {
        auto pos = this->offset + this->len;
        return pos;
    }

    ~DmaWriter() {
        if (this->fd == -1) return;
        for (i32 i = 0; i < WIDTH; i++) delete (BUF *) this->cbs[i].aio_buf;
        delete[] this->cbs;
    }
};

using DefaultDmaWriter = DmaWriter<1024 * 1024 * 1, 4, 512>;
