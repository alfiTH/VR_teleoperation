// DataRecord.h
#ifndef DATA_RECORD_H
#define DATA_RECORD_H

#include <cstdint>
#include <vector>
#include <string>
#include <mutex>
#include <fstream>
#include <atomic>
#include <chrono>
#include <RobotMiddleware.h>

#include <map>
#include <variant>


/*  Estructura de cada registro en el fichero binario:
 *  +----------------------+-----------------+--------+----------+
 *  |    timestamp (u32)   |  delay (u32)    | type(u8)| size(u32)|
 *  +----------------------+-----------------+--------+----------+
 *  |      payload bytes ...                               |
 *  +-------------------------------------------------------+
 *
 *  - `size` indica cuántos bytes ocupa el payload (hasta 4 GiB).
 *  - El tipo se expresa con el enum `RecordType`.
 */
enum class RecordType : std::uint8_t {
    VRPose          = 0,
    VRController    = 1,
    VRHaptic        = 2,
    ColorCloudData  = 3,
    ArmJoints       = 4,
    RobotPose       = 5,   // Pose de base RECIBIDA del robot (odometría)
    BaseSpeedCmd    = 6,   // Velocidad de base COMANDADA (setSpeedBase)
    BasePoseCmd     = 7    // Pose de base objetivo COMANDADA (setBasePose)
};

#pragma pack(push,1)   // Evita relleno de la estructura
struct RecordHeader {
    std::uint32_t timestamp;     // 32 bits
    std::uint32_t delay;         // 32 bits (puedes recortar si lo necesitas)
    RecordType   type;           // 8 bits
    std::uint32_t payloadSize;   // 32 bits (max. ~4.29 GB, permite payloads grandes como nubes de puntos)
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

#pragma pack(push,1)
struct BaseSpeedCmd {
    float x;
    float y;
    float yaw;
};
#pragma pack(pop)

#pragma pack(push,1)
struct BasePoseCmd : RobotMiddleware::Pose {};
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

    bool addData(const std::uint32_t timestamp,
                 const std::uint32_t delay,
                 const HapticData& data);

    bool addData(const std::uint32_t timestamp,
                 const std::uint32_t delay,
                 const RobotMiddleware::ColorCloudData& cloud);
                 
    bool addData(const std::uint32_t timestamp,
                 const std::uint32_t delay,
                 const JointData& data);

    bool addData(const std::uint32_t timestamp,
                 const std::uint32_t delay,
                 const RobotMiddleware::Pose& data);

    bool addData(const std::uint32_t timestamp,
                 const std::uint32_t delay,
                 const BaseSpeedCmd& data);

    bool addData(const std::uint32_t timestamp,
                 const std::uint32_t delay,
                 const BasePoseCmd& data);

    /* ---------- Control de sesión de grabación ---------- */

    /// Empieza una nueva sesión: limpia el buffer y reinicia el reloj de referencia.
    /// Mientras no se llame, addData() es un no-op (no consume memoria).
    bool startRecording();

    /// Detiene la sesión (no borra el buffer, para poder guardarlo después).
    bool stopRecording();

    bool isRecording() const;

    /// Milisegundos transcurridos desde el último startRecording() (0 si no hay sesión activa).
    /// Pensado para que todos los puntos de captura (envío/lectura) usen el mismo reloj
    /// y así los registros de distintos tipos sean comparables/sincronizables por timestamp.
    std::uint32_t elapsedMs() const;

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
                                                            RobotMiddleware::Pose,
                                                            BaseSpeedCmd,
                                                            BasePoseCmd>>;

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
    std::atomic<bool> recording_{false};
    std::chrono::steady_clock::time_point epoch_{};

    /*  Serializa una sola entrada y la añade al buffer  */
    template<typename T>
    bool append(const RecordType type, const T& payload,
                const std::uint32_t timestamp,
                const std::uint32_t delay);

    /*  Deserializa el fichero completo a `buffer_`  */
    bool deserializeFile(const std::string& filename);


};

#endif // DATA_RECORD_H
