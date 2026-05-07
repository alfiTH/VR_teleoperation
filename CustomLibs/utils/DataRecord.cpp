// DataRecord.cpp
#include "DataRecord.h"
#include <cstring>   // memcpy

/* ---------- Helper template ---------- */
template<typename T>
bool DataRecord::append(const RecordType type,
                        const T& payload,
                        const std::uint32_t timestamp,
                        const std::uint32_t delay)
{
    // 1. Serializar el payload a bytes
    constexpr std::size_t sz = sizeof(T);
    if (sz > UINT16_MAX) return false;   // sobrepasa el límite de la cabecera

    std::vector<std::byte> raw(sz);
    std::memcpy(raw.data(), &payload, sz);

    // 2. Construir la cabecera
    RecordHeader header{};
    header.timestamp   = timestamp;
    header.delay       = delay;
    header.type        = type;
    header.payloadSize = static_cast<std::uint16_t>(sz);

    // 3. Añadir a buffer_
    std::lock_guard lock(mtx_);
    const auto hdrBytes = reinterpret_cast<const std::byte*>(&header);
    buffer_.insert(buffer_.end(), hdrBytes, hdrBytes + sizeof(header));
    buffer_.insert(buffer_.end(), raw.begin(), raw.end());

    return true;
}

// void serialize(const ColorCloudData& cloud, std::ostream& os)
// {
//     // 1. Escribir la cantidad de puntos
//     uint32_t n = static_cast<uint32_t>(cloud.X.size());
//     os.write(reinterpret_cast<const char*>(&n), sizeof(n));

//     // 2. Escribir los arrays (asumimos que X,Y,Z,R,G,B tienen el mismo tamaño)
//     os.write(reinterpret_cast<const char*>(cloud.X.data()), n * sizeof(cloud.X[0]));
//     os.write(reinterpret_cast<const char*>(cloud.Y.data()), n * sizeof(cloud.Y[0]));
//     os.write(reinterpret_cast<const char*>(cloud.Z.data()), n * sizeof(cloud.Z[0]));
//     os.write(reinterpret_cast<const char*>(cloud.R.data()), n * sizeof(cloud.R[0]));
//     os.write(reinterpret_cast<const char*>(cloud.G.data()), n * sizeof(cloud.G[0]));
//     os.write(reinterpret_cast<const char*>(cloud.B.data()), n * sizeof(cloud.B[0]));
// }

/* ---------- overloads ---------- */
bool DataRecord::addData(const std::uint32_t timestamp,
                         const std::uint32_t delay,
                         const RobotMiddleware::Pose& pose,
                         const bool VR)
{
    // Si quieres usar el flag `VR` podrías añadirlo al payload.
    if (VR)
        return append<RobotMiddleware::Pose>(RecordType::VRPose, pose, timestamp, delay);
    else
        return append<RobotMiddleware::Pose>(RecordType::RobotPose, pose, timestamp, delay);
        ;
}

bool DataRecord::addData(const std::uint32_t timestamp,
                         const std::uint32_t delay,
                         const RobotMiddleware::Haptic& haptic)
{
    return append<RobotMiddleware::Haptic>(RecordType::VRHaptic, haptic, timestamp, delay);
}

bool DataRecord::addData(const std::uint32_t timestamp,
                         const std::uint32_t delay,
                         const RobotMiddleware::ColorCloudData& cloud)
{
    return append<RobotMiddleware::ColorCloudData>(RecordType::ColorCloudData, cloud, timestamp, delay);
}

/* ---------- Guardar en disco ---------- */
bool DataRecord::saveData(const std::string& filename) const
{
    std::ofstream ofs(filename, std::ios::binary | std::ios::trunc);
    if (!ofs.is_open()) return false;

    std::lock_guard lock(mtx_);
    ofs.write(reinterpret_cast<const char*>(buffer_.data()), buffer_.size());
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
