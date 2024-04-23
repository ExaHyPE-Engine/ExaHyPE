#pragma once

#include <typeinfo>

#include <lang/assert.h>
#include <io/writer/concept.h>

template<typename FileWriter>
class VtkPointWriterBase {
protected:
    FileWriter *writer = nullptr;
    unsigned long pointCount = 0;

    constexpr int particleCountPosition() const {
        return 59;
    }

    template<typename T>
    T byteswapped(T val) {
        T result;

        auto *bufIn = (char *) &val;
        auto *bufOut = (char *) &result;

        for (int i = 0; i < sizeof(T); i++) {
            bufOut[sizeof(T) - i - 1] = bufIn[i];
        }

        return result;
    }

    void updatePointsCount() {
        this->writer->seek(particleCountPosition());

        char buf[24];
        auto bytes = sprintf(buf, "%021lu", this->pointCount);
        assert(bytes == 21)

        this->writer->write(buf, bytes);
    }

    explicit VtkPointWriterBase(FileWriter *writer) : writer(writer) {}

    explicit VtkPointWriterBase() = default;
};

template<typename FileWriter, typename fp = float> requires IoWriter<FileWriter> and std::is_floating_point_v<fp>
class VtkPointAsciiWriter : protected VtkPointWriterBase<FileWriter> {
public:
    explicit VtkPointAsciiWriter(FileWriter *writer) : VtkPointWriterBase<FileWriter>(writer) {
        this->writer->write("# vtk DataFile Version 3.0\n"
                            "\n"
                            "ASCII \n"
                            "DATASET POLYDATA\nPOINTS 000000000000000000000 ");
        if (typeid(fp) == typeid(float)) {
            this->writer->write(" float\n");
        } else {
            assert(typeid(fp) == typeid(double))
            this->writer->write("double\n");
        }
    }

    explicit VtkPointAsciiWriter() = default;

    void add(fp x, fp y, fp z) {
        char buf[48];
        int bytes = sprintf(buf, "%.8e %.8e %.8e\n", x, y, z);
        this->writer->write(buf, bytes);
        this->pointCount++;
    }

    void addPointDataHeader() {
        char buf[48];
        auto bytes = sprintf(buf, "POINT_DATA %021lu\n", this->pointCount);
        assert(bytes <= 48)
        this->writer->write(buf, bytes);
    }

    void addScalarPointDataHeader(const char* name) {
        char buf[512];
        auto bytes = sprintf(buf, "SCALARS %s %s 1\nLOOKUP_TABLE default\n", name, typeid(fp) == typeid(float) ? " float" : "double");
        assert(bytes <= 512)
        this->writer->write(buf, bytes);
    }

    void addScalar(fp value) {
        char buf[48];
        int bytes = sprintf(buf, "%.8e\n", value);
        assert(bytes <= 48)
        this->writer->write(buf, bytes);
    }

    void finish() {
        this->updatePointsCount();
        this->writer->sync();
    }
};

template<typename FileWriter, typename fp = float> requires IoWriter<FileWriter> and std::is_floating_point_v<fp>
class VtkPointBinaryWriter : protected VtkPointWriterBase<FileWriter> {
public:
    explicit VtkPointBinaryWriter(FileWriter *writer) : VtkPointWriterBase<FileWriter>(writer) {
        this->writer->write("# vtk DataFile Version 3.0\n"
                            "\n"
                            "BINARY\n"
                            "DATASET POLYDATA\nPOINTS 000000000000000000000 ");
        if (typeid(fp) == typeid(float)) {
            this->writer->write(" float\n");
        } else {
            assert(typeid(fp) == typeid(double))
            this->writer->write("double\n");
        }
    }

    explicit VtkPointBinaryWriter() = default;

    void add(fp x, fp y, fp z) {
        if constexpr (IoBufferedWriter<FileWriter>) {
            auto *buf = (fp*) this->writer->buffer(3 * sizeof(fp));
            if (buf) {
                buf[0] = this->byteswapped(x);
                buf[1] = this->byteswapped(y);
                buf[2] = this->byteswapped(z);
                this->pointCount++;
                return;
            }
        }
        this->writer->write(this->byteswapped(x));
        this->writer->write(this->byteswapped(y));
        this->writer->write(this->byteswapped(z));

        this->pointCount++;
    }

    void addPointDataHeader() {
        char buf[48];
        auto bytes = sprintf(buf, "POINT_DATA %021lu\n", this->pointCount);
        assert(bytes <= 48)
        this->writer->write(buf, bytes);
    }

    void addScalarPointDataHeader(const char* name) {
        char buf[512];
        auto bytes = sprintf(buf, "SCALARS %s %s 1\nLOOKUP_TABLE default\n", name, typeid(fp) == typeid(float) ? " float" : "double");
        assert(bytes <= 512)
        this->writer->write(buf, bytes);
    }

    void addScalar(fp value) {
        this->writer->write(this->byteswapped(value));
    }

    void finish() {
        this->updatePointsCount();
        this->writer->sync();
    }
};
