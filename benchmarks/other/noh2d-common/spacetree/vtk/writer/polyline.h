#pragma once

#include <lang/assert.h>
#include <io/writer/concept.h>
#include <vtk/writer/point.h>

template<typename FileWriter, typename fp = float> requires IoWriter<FileWriter> and std::is_floating_point_v<fp>
class VtkPolylineAsciiWriter : protected VtkPointAsciiWriter<FileWriter, fp> {

    void addPolyline() {
        if (this->pointCount < 2) return;

        char buf[48];
        auto bytes = sprintf(buf, "LINES %d %lu\n%lu ", 1, this->pointCount + 1, this->pointCount);
        assert(bytes < 48)

        this->writer->write(buf, bytes);

        for (auto i = 0UL; i < this->pointCount; i++) {
            bytes = sprintf(buf, "%lu ", i);
            assert(bytes < 48)
            this->writer->write(buf, bytes);
        }
    }

public:
    explicit VtkPolylineAsciiWriter(FileWriter *writer) : VtkPointAsciiWriter<FileWriter, fp>(writer) {}

    explicit VtkPolylineAsciiWriter() = default;

    void add(fp x, fp y, fp z) {
        VtkPointAsciiWriter<FileWriter, fp>::add(x, y, z);
    }

    void finish() {
        this->addPolyline();
        this->updatePointsCount();
        this->writer->sync();
    }
};

template<typename FileWriter, typename fp = float> requires IoWriter<FileWriter> and std::is_floating_point_v<fp>
class VtkPolylineBinaryWriter : protected VtkPointBinaryWriter<FileWriter, fp> {

    void addPolyline() {
        if (this->pointCount < 2) return;

        char buf[48];
        auto bytes = sprintf(buf, "LINES %d %lu\n", 1, this->pointCount + 1);
        assert(bytes < 48)

        this->writer->write(buf, bytes);

        this->writer->write(this->byteswapped((unsigned int) this->pointCount));

        for (auto i = 0UL; i < this->pointCount; i++) {
            this->writer->write(this->byteswapped((unsigned int) i));
        }
    }

public:
    explicit VtkPolylineBinaryWriter(FileWriter *writer) : VtkPointBinaryWriter<FileWriter, fp>(writer) {}

    explicit VtkPolylineBinaryWriter() = default;

    void add(fp x, fp y, fp z) {
        VtkPointBinaryWriter<FileWriter, fp>::add(x, y, z);
    }

    void finish() {
        this->addPolyline();
        this->updatePointsCount();
        this->writer->sync();
    }
};
