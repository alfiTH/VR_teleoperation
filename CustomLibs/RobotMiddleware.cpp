#include "RobotMiddleware.h"
#include <Ice/Ice.h>  // solo aquí dentro
#include <IceStorm/IceStorm.h>
#include <memory>
#include <VRControllerPub.h>
#include <vrcontrollerpubI.cpp>
#include <KinovaArm.h>
#include <Lidar3D.h>
#include <cmath>
#include <optional>
#include <mutex>
#include <future>

# pragma region Templates

template <typename SubInterfaceType>
bool subscribe( const Ice::CommunicatorPtr& communicator,
                const IceStorm::TopicManagerPrxPtr& topicManager,
                const std::string& endpointConfig,
                std::string name_topic,
                const std::string& topicBaseName,
                std::shared_ptr<SubInterfaceType> servant,
                std::shared_ptr<IceStorm::TopicPrx> &topic,
                Ice::ObjectPrxPtr& proxy)
{
    try   
    {  
        if (!name_topic.empty()) name_topic += "/";
        name_topic += topicBaseName;

        Ice::ObjectAdapterPtr adapter = communicator->createObjectAdapterWithEndpoints(name_topic, endpointConfig);
        proxy = adapter->addWithUUID(servant)->ice_oneway();

        std::shared_ptr<IceStorm::TopicPrx> topic;
        try {
            std::cout << "\033[32mINFO\033[0m " << "creating topic\n";
            topic = topicManager->create(name_topic);
        }
        catch (...) {
            try{
                std::cout <<  "\033[1;33mWARNING\033[0m subscribing the " 
                        << name_topic << " topic. It's possible that other component have created, retrieving\n";
                topic = topicManager->retrieve(name_topic);
            }
            catch(...)
            {
                std::cout << "\033[31mERROR\033[0m subscribing the " 
                        << name_topic << "unknown non-std::exception"<< std::endl;
                return false; 
                
            }
            IceStorm::QoS qos;
            topic->subscribeAndGetPublisher(qos, proxy);
        }
        adapter->activate();
    }
    catch(...)
    {
        std::cout << "\033[31mERROR\033[0m subscribing the " 
            << name_topic << "to created adapter"<< std::endl;
    }
    return true;
}



template <typename PubProxyType, typename PubProxyPointer>
std::optional<PubProxyPointer> publish(const IceStorm::TopicManagerPrxPtr& topicManager,
             std::string name_topic,
             const std::string& topicBaseName)
{
    if (!name_topic.empty()) name_topic += "/";
    name_topic += topicBaseName;
    std::shared_ptr<IceStorm::TopicPrx> topic;
    try
    {
        std::cout << "\033[32mINFO\033[0m " << "creating topic\n";
        topic = topicManager->create(name_topic);
    }
    catch (...)
    {
        try
        {
        std::cout <<  "\033[1;33mWARNING\033[0m publishing the " 
                    << name_topic << " topic. It's possible that other component have created, retrieving\n";
        topic = topicManager->retrieve(name_topic);
        }
        catch(...)
        {
            std::cout << "\033[31mERROR\033[0m publishing the " 
                    << name_topic << "unknown non-std::exception"<< std::endl;
            return std::nullopt; 
        }   
    }

    if (!topic) return std::nullopt;
    try {

        auto publisher = topic->getPublisher();
        if (!publisher) {
            std::cout << "\033[31mERROR\033[0m  Publisher is null for topic " << name_topic << "\n";
            return std::nullopt;
        }
        auto pubProxy = Ice::uncheckedCast<PubProxyType>(publisher->ice_oneway());
        if (!pubProxy) {
            std::cout << "\033[31mERROR\033[0m  uncheckedCast failed for topic " << name_topic << "\n";
            return std::nullopt;
        }
        std::cout << "\033[32mINFO\033[0m " << name_topic << " initialized Ok!\n";
        return pubProxy;

    } catch (const Ice::Exception& ex) {
        std::cout << "\033[31mERROR\033[0m Failed to create publisher: " << ex << std::endl;
        return std::nullopt;
    } catch (...) {
        std::cout << "\033[31mERROR\033[0m Failed to create publisher: unknown non-std::exception\n";
        return std::nullopt;
    }

};

template <typename ProxyType, typename ProxyPointer>
std::optional<ProxyPointer> require(const Ice::CommunicatorPtr& communicator,
             const std::string& proxyConfig, 
             const std::string& proxyName)
{
    try
    {
        ProxyPointer proxy = Ice::uncheckedCast<ProxyType>(communicator->stringToProxy(proxyConfig));
        std::cout << "\033[32mINFO\033[0m " << proxyName << " initialized Ok!\n";
        return proxy;
    }
    catch(const Ice::Exception& ex)
    {
        std::cout << "\033[31mERROR\033[0m Exception creating proxy " << proxyName << ": " << ex;
        return std::nullopt;        
    }
}
# pragma endregion Templates

constexpr double DEG2RAD = M_PI / 180.0;
# pragma region Parsers

inline RoboCompVRControllerPub::Pose toIcePose(const RobotMiddleware::Pose& p) {     
    return {
        p.x*10,
        p.y*10,
        p.z*10,
        0,
        0,
        0,
        p.qrx,
        p.qry,
        p.qrz,
        p.qrw
    };
}
inline RoboCompVRControllerPub::Controller toIceController(const RobotMiddleware::Controller& c) { 
    return {c.trigger, c.grab, c.x, c.y, c.thumbstickCapTouch, c.aButton, c.aButtonCapTouch, c.bButton, c.bButtonCapTouch}; 
}
//inline std::vector<std::array<float, 3>> iceToCloudPoints(const RoboCompLidar3D::TData &lidar){
//    std::vector<std::array<float, 3>> pointCloud;
//    pointCloud.reserve(lidar.points.size());
//    std::transform(lidar.points.begin(), lidar.points.end(), std::back_inserter(pointCloud),
//                   [](const auto &point) -> std::array<float, 3> {
//                       return {point.x, point.y, point.z};
//                   });
//    return pointCloud;
//}
inline std::vector<std::array<float, 3>> iceToCloudPoints(const RoboCompLidar3D::TData &lidar) {
    std::vector<std::array<float, 3>> pointCloud;
    pointCloud.reserve(lidar.points.size());
    for (const auto &point : lidar.points)
        pointCloud.emplace_back(std::array<float, 3>{point.x,point.y, point.z});
    return pointCloud;
}
inline RobotMiddleware::Haptic iceToHaptic(const RoboCompVRControllerPub::Haptic &haptic){
    return {
        haptic.intensity,
        haptic.frequency
    };
}

# pragma endregion Parsers

struct RobotMiddleware::Impl {
    std::atomic<bool> running{true};
    Ice::CommunicatorHolder communicator;
    IceStorm::TopicManagerPrxPtr topicManager;
    RoboCompVRControllerPub::VRControllerPubPrxPtr vrcontroller_proxy;

    // Lidar
    RoboCompLidar3D::Lidar3DPrxPtr lidar3d_proxy;
    std::mutex lidar_mutex;
    std::thread lidar_worker;
    std::vector<std::array<float, 3>> cloudPoints;

    // Arm
    std::thread arm_worker;
    RoboCompKinovaArm::KinovaArmPrxPtr arm_left_proxy, arm_right_proxy;
    std::mutex arm_mutex;
    float left_arm[8]{};
    float right_arm[8]{};



    //Haptic
    std::shared_ptr<IceStorm::TopicPrx> vrcontrollerpub_topic;
	Ice::ObjectPrxPtr vrcontrollerpub;
    Haptic left_haptic, right_haptic;
    std::atomic<bool> hapticChanged{false};

    std::mutex haptic_mutex;
    std::shared_ptr<VRControllerPubI> servant = std::make_shared<VRControllerPubI>(
        nullptr, 
        [this](const RoboCompVRControllerPub::Haptic& l, const RoboCompVRControllerPub::Haptic& r) {
            std::scoped_lock<std::mutex> lock(haptic_mutex);
            this->left_haptic  = iceToHaptic(l);
            this->right_haptic = iceToHaptic(r);
            hapticChanged = true;
        },
        nullptr);

    Impl() 
        : communicator(makeInitData())
    {
         try
        {
            auto ic = communicator.communicator();
            topicManager = Ice::checkedCast<IceStorm::TopicManagerPrx>(ic->stringToProxy("IceStorm/TopicManager:default -p 9999"));
            if (!topicManager)
            {
                std::cout << "\033[31mERROR\033[0m TopicManager.Proxy not defined in config file."<<std::endl;
                running = false;
            }
            std::cout << "\033[32mINFO\033[0m topicManager ptr: " << topicManager.get() << "\n";
            
            if (auto lidar3d_opt = require<RoboCompLidar3D::Lidar3DPrx, RoboCompLidar3D::Lidar3DPrxPtr>(ic,
                                "lidar3d:tcp -h localhost -p 11988", "Lidar3DProxy"))
                lidar3d_proxy = *lidar3d_opt;
            else{
                running = false;
                return;
            }

            if (auto arm_left_opt = require<RoboCompKinovaArm::KinovaArmPrx, RoboCompKinovaArm::KinovaArmPrxPtr>(ic,
                                "kinovaarm1:tcp -h localhost -p 10006", "KinovaArmProxy"))
                arm_left_proxy = *arm_left_opt;
            else{
                running = false;
                return;
            }
            
            if (auto arm_right_opt = require<RoboCompKinovaArm::KinovaArmPrx, RoboCompKinovaArm::KinovaArmPrxPtr>(ic,
                                "kinovaarm:tcp -h localhost -p 10005", "KinovaArmProxy"))
                arm_right_proxy = *arm_right_opt;
            else{
                running = false;
                return;
            }

            if (auto vrcontroller_opt = publish<RoboCompVRControllerPub::VRControllerPubPrx, RoboCompVRControllerPub::VRControllerPubPrxPtr>(topicManager,
                            "", "VRControllerPub"))
                vrcontroller_proxy = *vrcontroller_opt;
            else{
                running = false;
                return;
            }

            if (not subscribe<VRControllerPubI>(ic, topicManager, "tcp -p 0", "haptic", "VRControllerPub", servant,
                            vrcontrollerpub_topic, vrcontrollerpub)){
                running = false;
                return;
            }
        }
        catch (const Ice::Exception& ex) {
             std::cout << "\033[31mERROR\033[0m Exception: 'rcnode' not running: " << ex << std::endl;
            std::cerr << "Ice exception: " << ex.what() << "\n";
            running = false;
        }
        catch (const std::exception& ex)
        {
            std::cout << "\033[31mERROR\033[0m Impl failed: " << ex.what() << std::endl;
            running = false;
        }
        catch (...)
        {
            std::cout << "\033[31mERROR\033[0m Exception: 'rcnode' not running: " << std::endl;
            std::cout << "\033[31mERROR\033[0m Impl failed: unknown non-std::exception"<< std::endl;
            running = false;
        }

        // lidar_worker = std::thread(&RobotMiddleware::Impl::updateLidarData, this);
        arm_worker = std::thread(&RobotMiddleware::Impl::updateRobotState, this);
    }
    ~Impl(){
        running = false;

        if (vrcontrollerpub_topic){
		    vrcontrollerpub_topic->unsubscribe(vrcontrollerpub);
            vrcontrollerpub_topic = nullptr;
        }

        vrcontroller_proxy = nullptr;
        lidar3d_proxy = nullptr;
        topicManager = nullptr;
        arm_left_proxy = nullptr;
        arm_right_proxy = nullptr;

        auto ic = communicator.communicator();
        if (ic) {
            ic->shutdown();
            ic->waitForShutdown();
        }

        if (lidar_worker.joinable())
            lidar_worker.join();
         if (arm_worker.joinable())
            arm_worker.join();

    }

    static Ice::InitializationData makeInitData() {
        Ice::InitializationData initData;
        initData.properties = Ice::createProperties();
        initData.properties->setProperty("Ice.Warn.Connections", "0");
        initData.properties->setProperty("Ice.Trace.Network", "0");
        initData.properties->setProperty("Ice.Trace.Protocol", "0");
        initData.properties->setProperty("Ice.MessageSizeMax", "20004800");
        return initData;
    }
    void updateLidarData(){
         using namespace std::chrono;
        const auto period = 20ms;

        while (running)
        {
            auto start = steady_clock::now();
            try {
                auto ret = lidar3d_proxy->getLidarData("", 0.0f, 3.1416, 1);
                {
                    std::scoped_lock lock(lidar_mutex);
                    cloudPoints = iceToCloudPoints(ret);
                }
            }catch (...){
                std::cout <<  "\033[1;33mWARNING\033[0m Failed getting lidar data\n";
            }
             // Period
            auto elapsed = steady_clock::now() - start;
            if (elapsed < period)
                std::this_thread::sleep_for(period - elapsed);
        }
    }

    void updateRobotState(){
        using namespace std::chrono;
        const auto period = 20ms;
        const double rad_to_deg = 180.0 / M_PI;

        while (running)
        {
            auto start = steady_clock::now();
            try
            {
                // Llamadas asíncronas no bloqueantes
                auto left_future  = arm_left_proxy->getJointsStateAsync();
                auto right_future = arm_right_proxy->getJointsStateAsync();

                RoboCompKinovaArm::TJoints left_joints  = left_future.get();
                RoboCompKinovaArm::TJoints right_joints = right_future.get();

                {
                    std::scoped_lock lock(arm_mutex);
                    for (int i = 0; i < 8; ++i)
                    {
                        left_arm[i]  = left_joints.joints[i].angle * rad_to_deg;
                        right_arm[i] = right_joints.joints[i].angle * rad_to_deg;
                    }
                }
            }
            catch (...)
            {
                std::cout << "\033[1;33mWARNING\033[0m Failed to update robot state\n";
            }

            // Period
            auto elapsed = steady_clock::now() - start;
            if (elapsed < period)
                std::this_thread::sleep_for(period - elapsed);
        }
    }


};

RobotMiddleware::RobotMiddleware() {
    initIce();
}
RobotMiddleware::~RobotMiddleware() { 
}
bool RobotMiddleware::initIce()
{
    	this->pImpl = std::make_unique<Impl>();
        return isRunning();
}

bool RobotMiddleware::isRunning(){
    return this->pImpl->running;
}

bool RobotMiddleware::sendPoses(const RobotMiddleware::Pose& head, const RobotMiddleware::Pose& left, const RobotMiddleware::Pose& right) {
    try {
        pImpl->vrcontroller_proxy->sendPoses(
            toIcePose(head),
            toIcePose(left),
            toIcePose(right)
        );
        return true;
    } catch (...) {
        std::cout <<  "\033[1;33mWARNING\033[0m Failed sending pose\n";
        return false;
    }
}


bool RobotMiddleware::sendControllers(const RobotMiddleware::Controller& left, const RobotMiddleware::Controller& right) {
    try {
        pImpl->vrcontroller_proxy->sendControllers(
            toIceController(left),
            toIceController(right)
        );
        return true;
    } catch (...) {
        std::cout <<  "\033[1;33mWARNING\033[0m Failed sending controllers\n";
        return false;
    }
}

bool RobotMiddleware::getHaptics(RobotMiddleware::Haptic& left, RobotMiddleware::Haptic& right){
    if (!pImpl) return false;
    std::scoped_lock<std::mutex> lock(pImpl->haptic_mutex);
    pImpl->hapticChanged = false;
    left  = pImpl->left_haptic;
    right = pImpl->right_haptic;
    return pImpl->hapticChanged;
}

std::vector<std::array<float, 3>> RobotMiddleware::getLidarData() {
    if (!pImpl) return {};
    try {
        std::scoped_lock lock(pImpl->lidar_mutex);
        return pImpl->cloudPoints;
    } catch (...) {
        std::cout <<  "\033[1;33mWARNING\033[0m Failed to get robot state\n";
        return {};
    }
}

bool RobotMiddleware::getRobotState(float (&left)[8], float (&right)[8]) {
    if (!pImpl) return false;
    try {
        {
            std::scoped_lock lock(pImpl->arm_mutex);
            for (int i = 0; i < 8; ++i) {
                left[i]  = pImpl->left_arm[i];
                right[i] = pImpl->right_arm[i];
            }
        }
        return true;
    } catch (...) {
        std::cout <<  "\033[1;33mWARNING\033[0m Failed to get robot state\n";
        return false;
    }
}
