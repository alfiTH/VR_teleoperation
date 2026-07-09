// DataRecord.h
#ifndef DATA_RECORD_H
#define DATA_RECORD_H

#include <cstdint>
#include <vector>
#include <string>
#include <mutex>
#include <fstream>
#include <RobotMiddleware.h>  

#include <map>
#include <variant>


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
    ArmJoints       = 4,
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

#pragma pack(push,1)
struct VRData {
    RobotMiddleware::Pose hmd;
    RobotMiddleware::Pose left;
    RobotMiddleware::Pose right;
};
#pragma pack(pop)

#pragma pack(push,1)
struct HapticData {
    RobotMiddleware::Haptic left;
    RobotMiddleware::Haptic right;
};
#pragma pack(pop)

#pragma pack(push,1)
struct JointData {
    RobotMiddleware::ArmJoint left;
    RobotMiddleware::ArmJoint right;
};
#pragma pack(pop)

#pragma pack(push,1)
struct ControllerData {
    RobotMiddleware::Controller left;
    RobotMiddleware::Controller right;
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

    bool addData(const std::uint32_t timestamp,
                 const std::uint32_t delay,
                 const VRData& data);

    bool addData(const std::uint32_t timestamp,
                 const std::uint32_t delay,
                 const ControllerData& data);

    /// Añade un registro con Haptic
    bool addData(const std::uint32_t timestamp,
                 const std::uint32_t delay,
                 const HapticData& data);


    // /// Añade un registro con ColorCloudData
    bool addData(const std::uint32_t timestamp,
                 const std::uint32_t delay,
                 const RobotMiddleware::ColorCloudData& cloud);
                 
    bool addData(const std::uint32_t timestamp,
                 const std::uint32_t delay,
                 const JointData& data);
                 
    /// Añade un registro con Pose
    bool addData(const std::uint32_t timestamp,
                 const std::uint32_t delay,
                 const RobotMiddleware::Pose& data);



    

    /// Guarda todo el buffer en disco (binario)
    bool saveData(const std::string& filename) const;

    /// Carga un fichero binario y reemplaza el buffer interno
    bool loadData(const std::string& filename);

    /// Limpia el buffer interno
    bool clearData();

    /* Tipo para los datos deserializados */
    using DeserializedData = std::map<std::pair<std::uint32_t, RecordType>, 
                                            std::variant<   VRData, 
                                                            HapticData, 
                                                            ControllerData, 
                                                            RobotMiddleware::ColorCloudData,
                                                            JointData,
                                                            RobotMiddleware::Pose>>;

    /// Recupera los datos del buffer_ deserializados en el map
    DeserializedData getDeserializedData() const;

    /// Imprime el contenido del map en consola
    static void printData(const DeserializedData& data);
    
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
