// DataRecord.h
#ifndef DATA_RECORD_H
#define DATA_RECORD_H

#include <cstdint>
#include <vector>
#include <string>
#include <mutex>
#include <fstream>
#include <RobotMiddleware.h>   // <-- tu cabecera con Pose, Haptic, …


/*  Estructura de cada registro en el fichero binario:
 *  +----------------------+-----------------+--------+----------+
 *  |    timestamp (u32)   |  delay (u32)    | type(u8)| size(u16)|
 *  +----------------------+-----------------+--------+----------+
 *  |      payload bytes ...                               |
 *  +-------------------------------------------------------+
 *
 *  - `size` indica cuántos bytes ocupa el payload.
 *  - El tipo se expresa con el enum `RecordType`.
 */
enum class RecordType : std::uint8_t {
    VRPose          = 0,
    VRController    = 1,
    VRHaptic        = 2,
    ColorCloudData  = 3,
    RobotState      = 4,
    RobotPose       = 5
};

#pragma pack(push,1)   // Evita relleno de la estructura
struct RecordHeader {
    std::uint32_t timestamp;     // 32 bits
    std::uint32_t delay;         // 32 bits (puedes recortar si lo necesitas)
    RecordType   type;           // 8 bits
    std::uint16_t payloadSize;   // 16 bits (máx. 65 535 bytes)
};
#pragma pack(pop)


class DataRecord {
public:
    /* ---------- Singleton ---------- */
    static DataRecord& getInstance() {
        static DataRecord instance;
        return instance;
    }

    /* ---------- API pública ---------- */

    /// Añade un registro con Pose
    bool addData(const std::uint32_t timestamp,
                 const std::uint32_t delay,
                 const RobotMiddleware::Pose& pose,
                 const bool VR = true);

    /// Añade un registro con Haptic
    bool addData(const std::uint32_t timestamp,
                 const std::uint32_t delay,
                 const RobotMiddleware::Haptic& haptic);

    /// Añade un registro con ColorCloudData
    bool addData(const std::uint32_t timestamp,
                 const std::uint32_t delay,
                 const RobotMiddleware::ColorCloudData& cloud);

    /// Guarda todo el buffer en disco (binario)
    bool saveData(const std::string& filename) const;

    /// Carga un fichero binario y reemplaza el buffer interno
    bool loadData(const std::string& filename);

private:
    /* ---------- Constructores privados ---------- */
    DataRecord() = default;
    ~DataRecord() = default;

    /* ---------- Deshabilitar copias/movimientos ---------- */
    DataRecord(const DataRecord&)            = delete;
    DataRecord& operator=(const DataRecord&) = delete;
    DataRecord(DataRecord&&)                 = delete;
    DataRecord& operator=(DataRecord&&)      = delete;

    /* ---------- Internas ---------- */
    mutable std::mutex mtx_;
    std::vector<std::byte> buffer_;          // Buffer en memoria

    /*  Serializa una sola entrada y la añade al buffer  */
    template<typename T>
    bool append(const RecordType type, const T& payload,
                const std::uint32_t timestamp,
                const std::uint32_t delay);

    /*  Deserializa el fichero completo a `buffer_`  */
    bool deserializeFile(const std::string& filename);
};

#endif // DATA_RECORD_H
