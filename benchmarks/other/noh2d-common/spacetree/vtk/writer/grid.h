#pragma once

#include <typeinfo>
#include <lang/assert.h>
#include <io/writer/concept.h>

template<typename FileWriter>
class VtkGridWriterBase {
protected:
    FileWriter *writer = nullptr;

    unsigned long cellCount = 0;

    constexpr int particleCountPosition() const {
        return 68;
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
        this->writer->sync();
        this->writer->seek(particleCountPosition());

        char buf[24];
        auto bytes = sprintf(buf, "%020lu", this->cellCount * 8);
        assert(bytes == 20)

        this->writer->write(buf, bytes);
    }

    explicit VtkGridWriterBase(FileWriter *writer) : writer(writer) {}

    explicit VtkGridWriterBase() = default;
};

template<typename FileWriter, typename fp = float> requires IoWriter<FileWriter> and std::is_floating_point_v<fp>
class VtkGridAsciiWriter : protected VtkGridWriterBase<FileWriter> {
    void addPoint(fp x, fp y, fp z) {
        char buf[48];
        int bytes = sprintf(buf, "%.8e %.8e %.8e\n", x, y, z);
        this->writer->write(buf, bytes);
    }

    void addCell(unsigned long idx) {
        char buf[512];
        auto bytes = sprintf(buf, "8 %lu %lu %lu %lu %lu %lu %lu %lu\n",
                             idx * 8 + 0,
                             idx * 8 + 1,
                             idx * 8 + 2,
                             idx * 8 + 3,
                             idx * 8 + 4,
                             idx * 8 + 5,
                             idx * 8 + 6,
                             idx * 8 + 7);

        assert(bytes < 512)

        this->writer->write(buf, bytes);
    }

    void addCells() {
        char buf[48];
        auto bytes = sprintf(buf, "CELLS %lu %lu\n", this->cellCount, this->cellCount * 9);
        assert(bytes < 48)

        this->writer->write(buf, bytes);

        for (auto i = 0UL; i < this->cellCount; i++) {
            this->addCell(i);
        }
    }

    void addCellTypes() {
        char buf[48];
        auto bytes = sprintf(buf, "CELL_TYPES %lu\n", this->cellCount);
        assert(bytes < 48)

        this->writer->write(buf, bytes);

        for (auto i = 0UL; i < this->cellCount; i++) {
            const int TYPE = 11;
            bytes = sprintf(buf, "%d\n", TYPE);
            assert(bytes < 16)
            this->writer->write(buf, bytes);
        }
    }

public:
    explicit VtkGridAsciiWriter(FileWriter *writer) : VtkGridWriterBase<FileWriter>(writer) {
        this->writer->write("# vtk DataFile Version 3.0\n"
                            "\n"
                            "ASCII \n"
                            "DATASET UNSTRUCTURED_GRID\n"
                            "POINTS 00000000000000000000 ");
        if (typeid(fp) == typeid(float)) {
            this->writer->write(" float\n"); // the leading space keeps alignment intact (len matches 'double')
        } else {
            assert(typeid(fp) == typeid(double))
            this->writer->write("double\n");
        }
    }

    void add(fp xmin, fp ymin, fp zmin,
             fp xmax, fp ymax, fp zmax) {

        this->addPoint(xmin, ymin, zmin);
        this->addPoint(xmin, ymax, zmin);
        this->addPoint(xmin, ymin, zmax);
        this->addPoint(xmin, ymax, zmax);

        this->addPoint(xmax, ymin, zmin);
        this->addPoint(xmax, ymax, zmin);
        this->addPoint(xmax, ymin, zmax);
        this->addPoint(xmax, ymax, zmax);

        this->cellCount++;
    }

    void finish() {
        this->addCells();
        this->addCellTypes();
        this->updatePointsCount();
        this->writer->sync();
    }
};

template<typename FileWriter, typename fp = float> requires IoWriter<FileWriter> and std::is_floating_point_v<fp>
class VtkGridBinaryWriter : protected VtkGridWriterBase<FileWriter> {
    void addPoint(fp x, fp y, fp z) {
        this->writer->write(this->byteswapped(x));
        this->writer->write(this->byteswapped(y));
        this->writer->write(this->byteswapped(z));
    }

    void addCell(unsigned long idx) {
        this->writer->write(this->byteswapped(8));
        this->writer->write(this->byteswapped((int) idx * 8 + 0));
        this->writer->write(this->byteswapped((int) idx * 8 + 1));
        this->writer->write(this->byteswapped((int) idx * 8 + 2));
        this->writer->write(this->byteswapped((int) idx * 8 + 3));
        this->writer->write(this->byteswapped((int) idx * 8 + 4));
        this->writer->write(this->byteswapped((int) idx * 8 + 5));
        this->writer->write(this->byteswapped((int) idx * 8 + 6));
        this->writer->write(this->byteswapped((int) idx * 8 + 7));
    }

    void addCells() {
        char buf[48];
        auto bytes = sprintf(buf, "CELLS %lu %lu\n", this->cellCount, this->cellCount * 9);
        assert(bytes < 48)

        this->writer->write(buf, bytes);

        for (auto i = 0UL; i < this->cellCount; i++) {
            this->addCell(i);
        }
    }

    void addCellTypes() {
        char buf[48];
        auto bytes = sprintf(buf, "CELL_TYPES %lu\n", this->cellCount);
        assert(bytes < 48)

        this->writer->write(buf, bytes);

        for (auto i = 0UL; i < this->cellCount; i++) {
            const int TYPE = 11;
            this->writer->write(this->byteswapped(TYPE));
        }
    }

public:
    explicit VtkGridBinaryWriter() = default;

    explicit VtkGridBinaryWriter(FileWriter *writer) : VtkGridWriterBase<FileWriter>(writer) {
        this->writer->write("# vtk DataFile Version 3.0\n"
                            "\n"
                            "BINARY\n"
                            "DATASET UNSTRUCTURED_GRID\n"
                            "POINTS 00000000000000000000 ");
        if (typeid(fp) == typeid(float)) {
            this->writer->write(" float\n"); // the leading space keeps alignment intact (len matches 'double')
        } else {
            assert(typeid(fp) == typeid(double))
            this->writer->write("double\n");
        }
    }

    void add(fp xmin, fp ymin, fp zmin,
             fp xmax, fp ymax, fp zmax) {

        this->addPoint(xmin, ymin, zmin);
        this->addPoint(xmin, ymax, zmin);
        this->addPoint(xmin, ymin, zmax);
        this->addPoint(xmin, ymax, zmax);

        this->addPoint(xmax, ymin, zmin);
        this->addPoint(xmax, ymax, zmin);
        this->addPoint(xmax, ymin, zmax);
        this->addPoint(xmax, ymax, zmax);

        this->cellCount++;
    }

    void finish() {
        this->addCells();
        this->addCellTypes();
        this->updatePointsCount();
        this->writer->sync();
    }
};
