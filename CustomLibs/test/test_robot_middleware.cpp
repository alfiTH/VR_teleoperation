/**
 * @file test_robot_middleware.cpp
 * @brief Pruebas unitarias para RobotMiddleware (librería estática)
 *
 * Cubre:
 *   - Estructuras de datos: Pose, Controller, Haptic, ColorCloudData
 *   - Ciclo de vida sin servicios ICE (degradación elegante)
 *   - Comportamiento seguro de métodos sin conexión activa
 */

#include <RobotMiddleware.h>
#include <DataRecord.h>

#include <algorithm> // std::fill
#include <cmath>
#include <cstring>   // memcmp
#include <gtest/gtest.h>
#include <unistd.h>

// ═══════════════════════════════════════════════════════════════════════
//   Pose Tests
// ═══════════════════════════════════════════════════════════════════════

TEST(PoseTest, DefaultInitialization) {
  RobotMiddleware::Pose pose{};
  EXPECT_FLOAT_EQ(pose.x, 0.0f);
  EXPECT_FLOAT_EQ(pose.y, 0.0f);
  EXPECT_FLOAT_EQ(pose.z, 0.0f);
  EXPECT_FLOAT_EQ(pose.qrx, 0.0f);
  EXPECT_FLOAT_EQ(pose.qry, 0.0f);
  EXPECT_FLOAT_EQ(pose.qrz, 0.0f);
  EXPECT_FLOAT_EQ(pose.qrw, 0.0f);
}

TEST(PoseTest, AggregateAssignment) {
  RobotMiddleware::Pose pose{1.0f, 2.0f, 3.0f, 0.0f, 0.0f, 0.707f, 0.707f};
  EXPECT_FLOAT_EQ(pose.x, 1.0f);
  EXPECT_FLOAT_EQ(pose.y, 2.0f);
  EXPECT_FLOAT_EQ(pose.z, 3.0f);
  EXPECT_FLOAT_EQ(pose.qrx, 0.0f);
  EXPECT_FLOAT_EQ(pose.qry, 0.0f);
  EXPECT_FLOAT_EQ(pose.qrz, 0.707f);
  EXPECT_FLOAT_EQ(pose.qrw, 0.707f);
}

TEST(PoseTest, CopySemantics) {
  RobotMiddleware::Pose original{1.5f, -2.5f, 3.5f, 0.1f, 0.2f, 0.3f, 0.9f};
  RobotMiddleware::Pose copy = original;

  EXPECT_FLOAT_EQ(copy.x, original.x);
  EXPECT_FLOAT_EQ(copy.y, original.y);
  EXPECT_FLOAT_EQ(copy.z, original.z);
  EXPECT_FLOAT_EQ(copy.qrx, original.qrx);
  EXPECT_FLOAT_EQ(copy.qry, original.qry);
  EXPECT_FLOAT_EQ(copy.qrz, original.qrz);
  EXPECT_FLOAT_EQ(copy.qrw, original.qrw);

  // Modificar copia no afecta al original
  copy.x = 999.0f;
  EXPECT_FLOAT_EQ(original.x, 1.5f);
}

// ═══════════════════════════════════════════════════════════════════════
//   Controller Tests
// ═══════════════════════════════════════════════════════════════════════

TEST(ControllerTest, DefaultInitialization) {
  RobotMiddleware::Controller ctrl{};
  EXPECT_FLOAT_EQ(ctrl.trigger, 0.0f);
  EXPECT_FLOAT_EQ(ctrl.grab, 0.0f);
  EXPECT_FLOAT_EQ(ctrl.x, 0.0f);
  EXPECT_FLOAT_EQ(ctrl.y, 0.0f);
  EXPECT_FLOAT_EQ(ctrl.thumbstickCapTouch, 0.0f);
  EXPECT_FALSE(ctrl.aButton);
  EXPECT_FLOAT_EQ(ctrl.aButtonCapTouch, 0.0f);
  EXPECT_FALSE(ctrl.bButton);
  EXPECT_FLOAT_EQ(ctrl.bButtonCapTouch, 0.0f);
}

TEST(ControllerTest, FullAssignment) {
  RobotMiddleware::Controller ctrl{};
  ctrl.trigger = 0.8f;
  ctrl.grab = 1.0f;
  ctrl.x = -0.5f;
  ctrl.y = 0.3f;
  ctrl.thumbstickCapTouch = 0.9f;
  ctrl.aButton = true;
  ctrl.aButtonCapTouch = 0.7f;
  ctrl.bButton = false;
  ctrl.bButtonCapTouch = 0.0f;

  EXPECT_FLOAT_EQ(ctrl.trigger, 0.8f);
  EXPECT_FLOAT_EQ(ctrl.grab, 1.0f);
  EXPECT_FLOAT_EQ(ctrl.x, -0.5f);
  EXPECT_FLOAT_EQ(ctrl.y, 0.3f);
  EXPECT_TRUE(ctrl.aButton);
  EXPECT_FALSE(ctrl.bButton);
}

TEST(ControllerTest, CopySemantics) {
  RobotMiddleware::Controller original{};
  original.trigger = 0.5f;
  original.aButton = true;

  RobotMiddleware::Controller copy = original;
  EXPECT_FLOAT_EQ(copy.trigger, 0.5f);
  EXPECT_TRUE(copy.aButton);

  copy.trigger = 0.0f;
  EXPECT_FLOAT_EQ(original.trigger, 0.5f);
}

// ═══════════════════════════════════════════════════════════════════════
//   Haptic Tests
// ═══════════════════════════════════════════════════════════════════════

TEST(HapticTest, DefaultInitialization) {
  RobotMiddleware::Haptic haptic{};
  EXPECT_FLOAT_EQ(haptic.intensity, 0.0f);
  EXPECT_FLOAT_EQ(haptic.frequency, 0.0f);
}

TEST(HapticTest, Assignment) {
  RobotMiddleware::Haptic haptic{0.75f, 100.0f};
  EXPECT_FLOAT_EQ(haptic.intensity, 0.75f);
  EXPECT_FLOAT_EQ(haptic.frequency, 100.0f);
}

TEST(HapticTest, CopySemantics) {
  RobotMiddleware::Haptic original{0.5f, 50.0f};
  RobotMiddleware::Haptic copy = original;
  EXPECT_FLOAT_EQ(copy.intensity, original.intensity);
  EXPECT_FLOAT_EQ(copy.frequency, original.frequency);

  copy.intensity = 1.0f;
  EXPECT_FLOAT_EQ(original.intensity, 0.5f);
}

// ═══════════════════════════════════════════════════════════════════════
//   ColorCloudData Tests
// ═══════════════════════════════════════════════════════════════════════

TEST(ColorCloudDataTest, DefaultIsEmpty) {
  RobotMiddleware::ColorCloudData cloud{};
  EXPECT_TRUE(cloud.X.empty());
  EXPECT_TRUE(cloud.Y.empty());
  EXPECT_TRUE(cloud.Z.empty());
  EXPECT_TRUE(cloud.R.empty());
  EXPECT_TRUE(cloud.G.empty());
  EXPECT_TRUE(cloud.B.empty());
}

TEST(ColorCloudDataTest, PushDataConsistency) {
  RobotMiddleware::ColorCloudData cloud;
  const size_t N = 100;

  for (size_t i = 0; i < N; ++i) {
    cloud.X.push_back(static_cast<short>(i));
    cloud.Y.push_back(static_cast<short>(i * 2));
    cloud.Z.push_back(static_cast<short>(i * 3));
    cloud.R.push_back(static_cast<unsigned char>(i % 256));
    cloud.G.push_back(static_cast<unsigned char>((i + 85) % 256));
    cloud.B.push_back(static_cast<unsigned char>((i + 170) % 256));
  }

  EXPECT_EQ(cloud.X.size(), N);
  EXPECT_EQ(cloud.Y.size(), N);
  EXPECT_EQ(cloud.Z.size(), N);
  EXPECT_EQ(cloud.R.size(), N);
  EXPECT_EQ(cloud.G.size(), N);
  EXPECT_EQ(cloud.B.size(), N);

  // Verificar algunos valores
  EXPECT_EQ(cloud.X[0], 0);
  EXPECT_EQ(cloud.X[50], 50);
  EXPECT_EQ(cloud.Y[10], 20);
  EXPECT_EQ(cloud.R[255 % N], static_cast<unsigned char>(255 % N % 256));
}

TEST(ColorCloudDataTest, MoveSemantics) {
  RobotMiddleware::ColorCloudData source;
  source.X = {1, 2, 3, 4, 5};
  source.Y = {10, 20, 30, 40, 50};
  source.Z = {100, 200, 300, 400, 500};
  source.R = {255, 0, 128, 64, 32};
  source.G = {0, 255, 128, 64, 32};
  source.B = {128, 128, 128, 128, 128};

  RobotMiddleware::ColorCloudData dest = std::move(source);

  EXPECT_EQ(dest.X.size(), 5u);
  EXPECT_EQ(dest.X[0], 1);
  EXPECT_EQ(dest.X[4], 5);
  EXPECT_EQ(dest.R[0], 255);
  EXPECT_EQ(dest.B[2], 128);

  // source should be in a valid-but-unspecified (typically empty) state
  // We just verify it doesn't crash to access
  EXPECT_TRUE(source.X.empty() || source.X.size() <= 5);
}

TEST(ColorCloudDataTest, LargeCloudAllocation) {
  RobotMiddleware::ColorCloudData cloud;
  const size_t LARGE_N = 500'000;

  cloud.X.resize(LARGE_N, 42);
  cloud.Y.resize(LARGE_N, 43);
  cloud.Z.resize(LARGE_N, 44);
  cloud.R.resize(LARGE_N, 100);
  cloud.G.resize(LARGE_N, 150);
  cloud.B.resize(LARGE_N, 200);

  EXPECT_EQ(cloud.X.size(), LARGE_N);
  EXPECT_EQ(cloud.X[LARGE_N - 1], 42);
  EXPECT_EQ(cloud.B[0], 200);
}

// ═══════════════════════════════════════════════════════════════════════
//   Middleware Lifecycle Tests (sin servicios ICE)
// ═══════════════════════════════════════════════════════════════════════

class MiddlewareLifecycleTest : public ::testing::Test {
protected:
  // Ya no necesitamos el unique_ptr 'mw'
  // Accederemos directamente vía RobotMiddleware::getInstance()
  
  void SetUp() override {
    // Si necesitas resetear algo antes de cada test, 
    // tendrías que añadir un método "reset" a tu clase,
    // ya que el Singleton no se destruye hasta el final del programa.
  }

  void TearDown() override {
    // No podemos hacer mw.reset() porque no somos dueños de la instancia
  }
};

// 1. Prueba de construcción
TEST_F(MiddlewareLifecycleTest, ConstructAndDestruct) { 
  // La primera llamada construye el objeto.
  RobotMiddleware& instance = RobotMiddleware::getInstance();
  SUCCEED(); 
}

// 2. Verificar estado inicial
TEST_F(MiddlewareLifecycleTest, IsRunningStatus) {
  // Nota: Si el hardware no está conectado, esto será FALSE según tu lógica de Impl
  EXPECT_TRUE(RobotMiddleware::getInstance().isRunning());
}

// 3. Prueba de envío sin conexión
TEST_F(MiddlewareLifecycleTest, SendDataReturns) {
  RobotMiddleware::Pose head{}, left{}, right{};
  RobotMiddleware::Controller leftCtrl{}, rightCtrl{};

  // Usamos la instancia única
  bool result = RobotMiddleware::getInstance().sendData(head, left, leftCtrl, right, rightCtrl);
  
  // Como no hay conexión ICE, debería fallar graciosamente
  EXPECT_TRUE(result);
}

// 4. Prueba de nube de puntos vacía
TEST_F(MiddlewareLifecycleTest, GetColorCloudDataReturns) {
  sleep(1);
  const auto guard = RobotMiddleware::getInstance().lockColorCloudData();
  EXPECT_TRUE(guard.valid());
	const RobotMiddleware::ColorCloudData& cloud = guard.get();
  EXPECT_FALSE(cloud.X.empty());
  EXPECT_FALSE(cloud.Y.empty());
  EXPECT_FALSE(cloud.Z.empty());
  EXPECT_FALSE(cloud.R.empty());
  EXPECT_FALSE(cloud.G.empty());
  EXPECT_FALSE(cloud.B.empty());
}

// 5. Prueba de estado del robot
TEST_F(MiddlewareLifecycleTest, GetRobotStateReturns) {
  float left[8] = {INFINITY, INFINITY, INFINITY, INFINITY, INFINITY, INFINITY, INFINITY, INFINITY};
  float right[8] = {INFINITY, INFINITY, INFINITY, INFINITY, INFINITY, INFINITY, INFINITY, INFINITY};
sleep(1);
  bool result = RobotMiddleware::getInstance().getRobotState(left, right);
  EXPECT_TRUE(result);
  for (int i = 0; i < 8; ++i) {
    EXPECT_FALSE(std::isinf(left[i])) << "Error en el índice left[" << i << "]";
    EXPECT_FALSE(std::isinf(right[i])) << "Error en el índice right[" << i << "]";
  }
}

// 6. Prueba de hapticos
TEST_F(MiddlewareLifecycleTest, GetHapticsReturns) {
  sleep(1);
  RobotMiddleware::Haptic left = {INFINITY, INFINITY};
  RobotMiddleware::Haptic right = {INFINITY, INFINITY};
  bool result = RobotMiddleware::getInstance().receiveHaptics(left, right);
  EXPECT_TRUE(result);
  EXPECT_FALSE(std::isinf(left.intensity));
  EXPECT_FALSE(std::isinf(left.frequency));
  EXPECT_FALSE(std::isinf(right.intensity));
  EXPECT_FALSE(std::isinf(right.frequency));
}

TEST_F(MiddlewareLifecycleTest, GetOmniBaseStateReturns) {
  RobotMiddleware::Pose omniBaseState={INFINITY, INFINITY, INFINITY, INFINITY, INFINITY, INFINITY, INFINITY};
  sleep(1);
  bool result = RobotMiddleware::getInstance().getRobotPose(omniBaseState);
  EXPECT_TRUE(result);
  EXPECT_FALSE(std::isinf(omniBaseState.x));
  EXPECT_FALSE(std::isinf(omniBaseState.y));
  EXPECT_FALSE(std::isinf(omniBaseState.z));
  EXPECT_FALSE(std::isinf(omniBaseState.qrx));
  EXPECT_FALSE(std::isinf(omniBaseState.qry));
  EXPECT_FALSE(std::isinf(omniBaseState.qrz));
  EXPECT_FALSE(std::isinf(omniBaseState.qrw));
}
  

// ────────────────────────
//   DataRecord Serialization Tests
// ────────────────────────

class DataRecordTest : public ::testing::Test {
protected:
    // Opcional: Variables compartidas entre pruebas
    // MySingleton* instance = nullptr;

    void SetUp() override {
        // Código que se ejecuta antes de CADA prueba
        // Ejemplo: Reiniciar el singleton o limpiar datos
    }

    void TearDown() override {
        // Código que se ejecuta después de CADA prueba
      DataRecord& dr = DataRecord::getInstance();
      dr.clearData();

    }
};

TEST_F(DataRecordTest, TypesData) {
  DataRecord& dr = DataRecord::getInstance();

  VRData vr{{1.0f, 2.0f, 3.0f, 0.0f, 0.0f, 0.707f, 0.707f}, 
                      {1.0f, 2.0f, 3.0f, 0.0f, 0.0f, 0.707f, 0.707f},
                      {1.0f, 2.0f, 3.0f, 0.0f, 0.0f, 0.707f, 0.707f}};
  dr.addData(100u, 0u, vr);

  ControllerData controllers{{0.0f, 0.0f, 0.2f, 0.53f, 0.0f, 0.0f, 0.0f, false, false, false}, 
                            {0.0f, 0.0f, 0.1f, 1.0f, 0.0f, 0.0f, 0.0f, false, false, false}};
  dr.addData(200u, 0u, controllers);
    
  // Add a Haptic record
  HapticData haptic{{0.5f, 120.0f}, {0.5f, 120.0f}};
  dr.addData(2000u, 20u, haptic);

  JointData armJoins{ {1.30f, 2.40f, 3.05f, 0.40f, 02.0f, 0.7077f, 0.707f, 2.1},
                                  {-1.30f, -2.40f, -3.05f, -0.40f, -2.0f, -0.7077f, 5.707f, -2.1}};

  RobotMiddleware::Pose pose{1.0f, 2.0f, 3.0f, 0.0f, 0.0f, 0.707f, 0.707f};
  dr.addData(1000u, 10u, pose);

  DataRecord::printData(dr.getDeserializedData());

}



TEST_F(DataRecordTest, AddAndSaveLoadRoundTrip) {
    // Create a fresh instance and clear buffer via loadData from empty file
    const std::string tmpfile = "/tmp/datatest.bin";
    // Ensure no pre-existing data
    std::remove(tmpfile.c_str());

    DataRecord& dr = DataRecord::getInstance();
    // Add a Pose record (VRPose)
    RobotMiddleware::Pose pose{1.0f, 2.0f, 3.0f, 0.0f, 0.0f, 0.707f, 0.707f};
    dr.addData(1000u, 10u, pose);

    // Add a Haptic record
    HapticData haptic{{0.5f, 120.0f}, {0.5f, 120.0f}};
    dr.addData(2000u, 20u, haptic);

    // Save to disk
    ASSERT_TRUE(dr.saveData(tmpfile));

    // Load into a new instance (same singleton) – buffer should be replaced
    DataRecord& dr2 = DataRecord::getInstance();
    ASSERT_TRUE(dr2.clearData());
    ASSERT_TRUE(dr2.loadData(tmpfile));

    // Compare internal buffers size and content via serialization of same records
    // Re-serialize to temporary buffer for comparison
    std::vector<std::byte> expected;
    {
        // Manually construct expected buffer using same logic as DataRecord::append
        RecordHeader hdr1{1000u, 10u, RecordType::VRPose, static_cast<uint16_t>(sizeof(RobotMiddleware::Pose))};
        const std::byte* pHdr1 = reinterpret_cast<const std::byte*>(&hdr1);
        expected.insert(expected.end(), pHdr1, pHdr1 + sizeof(hdr1));
        const std::byte* pPos = reinterpret_cast<const std::byte*>(&pose);
        expected.insert(expected.end(), pPos, pPos + sizeof(pose));

        RecordHeader hdr2{2000u, 20u, RecordType::VRHaptic, static_cast<uint16_t>(sizeof(RobotMiddleware::Haptic))};
        const std::byte* pHdr2 = reinterpret_cast<const std::byte*>(&hdr2);
        expected.insert(expected.end(), pHdr2, pHdr2 + sizeof(hdr2));
        const std::byte* pHit = reinterpret_cast<const std::byte*>(&haptic);
        expected.insert(expected.end(), pHit, pHit + sizeof(haptic));
    }

    // Access internal buffer via reflection: we cannot directly. Instead, save again and compare files.
    const std::string tmpfile2 = "/tmp/datatest2.bin";
    ASSERT_TRUE(dr2.saveData(tmpfile2));

    // Read both files into vectors for comparison
    auto readFile=[&](const std::string &fn){
        std::ifstream ifs(fn, std::ios::binary);
        return std::vector<char>(std::istreambuf_iterator<char>(ifs), {});
    };

    auto buf1 = readFile(tmpfile);
    auto buf2 = readFile(tmpfile2);
    ASSERT_EQ(buf1.size(), buf2.size());
    EXPECT_TRUE(std::equal(buf1.begin(), buf1.end(), buf2.begin()));
    DataRecord::printData(dr2.getDeserializedData());

    RobotMiddleware::Pose pose2{1.45f, 2.45f, 3.0f, 0.0f, 0.0f, 0.707f, 0.707f};
    dr.addData(1100u, 10u, pose2);

    DataRecord::printData(dr2.getDeserializedData());
}
