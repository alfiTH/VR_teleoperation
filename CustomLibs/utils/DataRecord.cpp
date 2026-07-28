// DataRecord.cpp
#include "DataRecord.h"
#include <cstring>   // memcpy
#include <print>
#include <iostream>

/* ---------- Helper template ---------- */
template<typename T>
bool DataRecord::append(const RecordType type,
                        const T& payload,
                        const std::uint32_t timestamp,
                        const std::uint32_t delay)
{
    if (!recording_) return true;   // no hay sesión activa: no-op, no se acumula memoria

    // 1. Serializar el payload a bytes
    constexpr std::size_t sz = sizeof(T);
    if (sz > UINT32_MAX) return false;   // sobrepasa el límite de la cabecera

    std::vector<std::byte> raw(sz);
    std::memcpy(raw.data(), &payload, sz);

    // 2. Construir la cabecera
    RecordHeader header{};
    header.timestamp   = timestamp;
    header.delay       = delay;
    header.type        = type;
    header.payloadSize = static_cast<std::uint32_t>(sz);

    // 3. Añadir a buffer_
    std::lock_guard lock(mtx_);
    const auto hdrBytes = reinterpret_cast<const std::byte*>(&header);
    buffer_.insert(buffer_.end(), hdrBytes, hdrBytes + sizeof(header));
    buffer_.insert(buffer_.end(), raw.begin(), raw.end());

    return true;
}

void serialize(const RobotMiddleware::ColorCloudData& cloud, std::ostream& os)
{
    // 1. Escribir la cantidad de puntos
    uint32_t n = static_cast<uint32_t>(cloud.X.size());
    os.write(reinterpret_cast<const char*>(&n), sizeof(n));

    // 2. Escribir los arrays (asumimos que X,Y,Z,R,G,B tienen el mismo tamaño)
    os.write(reinterpret_cast<const char*>(cloud.X.data()), n * sizeof(cloud.X[0]));
    os.write(reinterpret_cast<const char*>(cloud.Y.data()), n * sizeof(cloud.Y[0]));
    os.write(reinterpret_cast<const char*>(cloud.Z.data()), n * sizeof(cloud.Z[0]));
    os.write(reinterpret_cast<const char*>(cloud.R.data()), n * sizeof(cloud.R[0]));
    os.write(reinterpret_cast<const char*>(cloud.G.data()), n * sizeof(cloud.G[0]));
    os.write(reinterpret_cast<const char*>(cloud.B.data()), n * sizeof(cloud.B[0]));
}

/* ---------- overloads ---------- */
bool DataRecord::addData(const std::uint32_t timestamp,
                         const std::uint32_t delay,
                         const VRData& data)
{
    return append<VRData>(RecordType::VRPose, data, timestamp, delay);
}

bool DataRecord::addData(const std::uint32_t timestamp,
                         const std::uint32_t delay,
                         const ControllerData& data)
{
    return append<ControllerData>(RecordType::VRController, data, timestamp, delay);
}

bool DataRecord::addData(const std::uint32_t timestamp,
                         const std::uint32_t delay,
                         const HapticData& data)
{
    return append<HapticData>(RecordType::VRHaptic, data, timestamp, delay);
}

bool DataRecord::addData(const std::uint32_t timestamp,
                         const std::uint32_t delay,
                         const RobotMiddleware::ColorCloudData& cloud)
{
    return append<RobotMiddleware::ColorCloudData>(RecordType::ColorCloudData, {cloud}, timestamp, delay);
}

bool DataRecord::addData(const std::uint32_t timestamp,
                         const std::uint32_t delay,
                         const JointData& data)
{
    return append<JointData>(RecordType::ArmJoints, data, timestamp, delay);
}

bool DataRecord::addData(const std::uint32_t timestamp,
                         const std::uint32_t delay,
                         const RobotMiddleware::Pose& data)
{
    return append<RobotMiddleware::Pose>(RecordType::RobotPose, data, timestamp, delay);

}

bool DataRecord::addData(const std::uint32_t timestamp,
                         const std::uint32_t delay,
                         const BaseSpeedCmd& data)
{
    return append<BaseSpeedCmd>(RecordType::BaseSpeedCmd, data, timestamp, delay);
}

bool DataRecord::addData(const std::uint32_t timestamp,
                         const std::uint32_t delay,
                         const BasePoseCmd& data)
{
    return append<BasePoseCmd>(RecordType::BasePoseCmd, data, timestamp, delay);
}

/* ---------- Control de sesión de grabación ---------- */
bool DataRecord::startRecording()
{
    std::lock_guard lock(mtx_);
    buffer_.clear();
    epoch_ = std::chrono::steady_clock::now();
    recording_ = true;
    return true;
}

bool DataRecord::stopRecording()
{
    recording_ = false;
    return true;
}

bool DataRecord::isRecording() const
{
    return recording_;
}

std::uint32_t DataRecord::elapsedMs() const
{
    if (!recording_) return 0;
    std::chrono::steady_clock::time_point epoch;
    {
        std::lock_guard lock(mtx_);
        epoch = epoch_;
    }
    const auto now = std::chrono::steady_clock::now();
    return static_cast<std::uint32_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now - epoch).count());
}

/* ---------- Guardar en disco ---------- */
bool DataRecord::saveData(const std::string& filename) const
{
    std::print("\033[32mSave Data: {} in {}\033[0m\n", buffer_.size(), filename);
    std::ofstream ofs(filename, std::ios::binary | std::ios::trunc);
    if (!ofs.is_open()) return false;

    std::lock_guard lock(mtx_);
    ofs.write(reinterpret_cast<const char*>(buffer_.data()), buffer_.size());
    ofs.close();
    std::print("\033[32mData saved successfully\033[0m\n");
    return ofs.good();
}

/* ---------- Cargar desde disco ---------- */
bool DataRecord::loadData(const std::string& filename)
{
    return deserializeFile(filename);
}

bool DataRecord::deserializeFile(const std::string& filename)
{
    std::ifstream ifs(filename, std::ios::binary | std::ios::in);
    if (!ifs.is_open()) return false;

    // Leer todo el fichero en memoria
    std::vector<unsigned char> tmp((std::istreambuf_iterator<char>(ifs)),
                               std::istreambuf_iterator<char>());
    if (tmp.empty() && !ifs.eof()) return false;  // error de lectura

    std::lock_guard lock(mtx_);
    buffer_.reserve(tmp.size());
    for (unsigned char c : tmp) {
        buffer_.emplace_back(static_cast<std::byte>(c));
    }   // reemplaza el contenido actual

    return true;
}

bool DataRecord::clearData() {
    try {
        std::lock_guard lock(mtx_);
        buffer_.clear();
        return true;
    } catch (const std::exception &ex) {
        std::cerr << "\033[1;33mWARNING\033[0m clearData std::exception: "
                    << ex.what() << "\n";
        return false;
    } catch (...) {
        std::cerr << "\033[1;33mWARNING\033[0m Failed to clear data\n";
        return false;
    }
}

DataRecord::DeserializedData DataRecord::getDeserializedData() const {
    DeserializedData result;
    std::lock_guard lock(mtx_);
    
    std::size_t offset = 0;
    while (offset + sizeof(RecordHeader) <= buffer_.size()) {
        RecordHeader header{};
        // Safe copy from buffer to header
        std::memcpy(&header, buffer_.data() + offset, sizeof(RecordHeader));
        
        std::size_t recordSize = sizeof(RecordHeader) + header.payloadSize;
        if (offset + recordSize > buffer_.size()) break;
        
        auto key = std::make_pair(header.timestamp, header.type);
        
        // Check payload size to decide which struct to use
        if (header.type == RecordType::RobotPose && header.payloadSize == sizeof(RobotMiddleware::Pose)) {
            RobotMiddleware::Pose pose{};
            std::memcpy(&pose, buffer_.data() + offset + sizeof(RecordHeader), sizeof(RobotMiddleware::Pose));
            result[key] = pose;
        } else if (header.type == RecordType::VRHaptic && header.payloadSize == sizeof(HapticData)) {
            HapticData haptic{};
            std::memcpy(&haptic, buffer_.data() + offset + sizeof(RecordHeader), sizeof(HapticData));
            result[key] = haptic;
        } else if (header.type == RecordType::VRController && header.payloadSize == sizeof(ControllerData)) {
            ControllerData controller{};
            std::memcpy(&controller, buffer_.data() + offset + sizeof(RecordHeader), sizeof(ControllerData));
            result[key] = controller;
        } else if (header.type == RecordType::VRPose && header.payloadSize == sizeof(VRData)) {
            VRData vrData{};
            std::memcpy(&vrData, buffer_.data() + offset + sizeof(RecordHeader), sizeof(VRData));
            result[key] = vrData;
        } else if (header.type == RecordType::ArmJoints && header.payloadSize == sizeof(JointData)) {
            JointData jointData{};
            std::memcpy(&jointData, buffer_.data() + offset + sizeof(RecordHeader), sizeof(JointData));
            result[key] = jointData;
        } else if (header.type == RecordType::BaseSpeedCmd && header.payloadSize == sizeof(BaseSpeedCmd)) {
            BaseSpeedCmd speedCmd{};
            std::memcpy(&speedCmd, buffer_.data() + offset + sizeof(RecordHeader), sizeof(BaseSpeedCmd));
            result[key] = speedCmd;
        } else if (header.type == RecordType::BasePoseCmd && header.payloadSize == sizeof(BasePoseCmd)) {
            BasePoseCmd poseCmd{};
            std::memcpy(&poseCmd, buffer_.data() + offset + sizeof(RecordHeader), sizeof(BasePoseCmd));
            result[key] = poseCmd;
        } else {
            // Unknown payload size, skip
            std::print("\033[33mSkipping record with unknown payload size: {}\033[0m\n", header.payloadSize);
        }
        
        offset += recordSize;
    }
    return result;
}

void DataRecord::printData(const DeserializedData& data) {
    std::print("\n--- Deserialized Data ({}) ---\n", data.size());
    
    for (const auto& [key, variant] : data) {
        std::uint32_t time = key.first;
        RecordType type = key.second;
        
        // std::print("Time: {}, Type: ", time);
        switch (type) {
            case RecordType::VRPose: std::print("VRPose"); break;
            case RecordType::RobotPose: std::print("RobotPose"); break;
            case RecordType::VRHaptic: std::print("VRHaptic"); break;
            case RecordType::ColorCloudData: std::print("ColorCloudData"); break;
            case RecordType::ArmJoints: std::print("ArmJoints"); break;
            case RecordType::VRController: std::print("VRController"); break;
            case RecordType::BaseSpeedCmd: std::print("BaseSpeedCmd"); break;
            case RecordType::BasePoseCmd: std::print("BasePoseCmd"); break;
            default: std::print("Unknown"); break;
        }
        std::print(": ");
        
        std::visit([](const auto& val) {
            using T = std::decay_t<decltype(val)>;
            if constexpr (std::is_same_v<T, RobotMiddleware::Pose>) {
                std::print("Pose(x={}, y={}, z={}, qrx={}, qry={}, qrz={}, qrw={})", 
                           val.x, val.y, val.z, val.qrx, val.qry, val.qrz, val.qrw);
            } else if constexpr (std::is_same_v<T, HapticData>) {
                std::print("Haptic(left(intensity={}, frequency={}), right(intensity={}, frequency={}))", 
                val.left.intensity, val.left.frequency, val.right.intensity, val.right.frequency);
            } else if constexpr (std::is_same_v<T, ControllerData>) {
                std::print("Controller(left(trigger={}, grab={}, x={}, y={}, aButton={}, bButton={}, "
                           "aCapTouch={}, bCapTouch={}, thumbstickCapTouch={}, thumbstickButton={}), "
                           "right(trigger={}, grab={}, x={}, y={}, aButton={}, bButton={}, "
                           "aCapTouch={}, bCapTouch={}, thumbstickCapTouch={}, thumbstickButton={})"
                           ")",
                           // Argumentos izquierda
                            val.left.trigger, val.left.grab, val.left.x, val.left.y,
                            val.left.aButton, val.left.bButton, val.left.aButtonCapTouch,
                            val.left.bButtonCapTouch, val.left.thumbstickCapTouch, val.left.thumbstickButton,
                            // Argumentos derecha
                            val.right.trigger, val.right.grab, val.right.x, val.right.y,
                            val.right.aButton, val.right.bButton, val.right.aButtonCapTouch,
                            val.right.bButtonCapTouch, val.right.thumbstickCapTouch, val.right.thumbstickButton
                        );
            } else if constexpr (std::is_same_v<T, VRData>) {
                std::print("VRData(hmd(x={}, y={}, z={}, qrx={}, qry={}, qrz={}, qrw={}),"
                            "left(x={}, y={}, z={}, qrx={}, qry={}, qrz={}, qrw={}), "
                            "right(x={}, y={}, z={}, qrx={}, qry={}, qrz={}, qrw={}))", 
                           val.hmd.x, val.hmd.y, val.hmd.z, val.hmd.qrx, val.hmd.qry, val.hmd.qrz, val.hmd.qrw, 
                           val.left.x, val.left.y, val.left.z, val.left.qrx, val.left.qry, val.left.qrz, val.left.qrw, 
                           val.right.x, val.right.y, val.right.z, val.right.qrx, val.right.qry, val.right.qrz, val.right.qrw);
            } else if constexpr (std::is_same_v<T, JointData>) {
                std::print("JointData(left(j1={}, j2={}, j3={}, j4={}, j5={}, j6={}, j7={}, gripper={}), "
                           "right(j1={}, j2={}, j3={}, j4={}, j5={}, j6={}, j7={}, gripper={}))", 
                           val.left[0], val.left[1], val.left[2], val.left[3], val.left[4], val.left[5], val.left[6], val.left[7],
                           val.right[0], val.right[1], val.right[2], val.right[3], val.right[4], val.right[5], val.right[6], val.right[7]);
            } else if constexpr (std::is_same_v<T, BaseSpeedCmd>) {
                std::print("BaseSpeedCmd(x={}, y={}, yaw={})", val.x, val.y, val.yaw);
            } else if constexpr (std::is_same_v<T, BasePoseCmd>) {
                std::print("BasePoseCmd(x={}, y={}, z={}, qrx={}, qry={}, qrz={}, qrw={})",
                           val.x, val.y, val.z, val.qrx, val.qry, val.qrz, val.qrw);
            }
        }, variant);
        std::print("\n");
    }
    std::print("--- End Data ---\n");
}