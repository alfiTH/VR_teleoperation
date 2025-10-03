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
                std::shared_ptr<IceStorm::TopicPrx> topic,
                Ice::ObjectPrxPtr& proxy)
{
    try   
    {  
        if (!name_topic.empty()) name_topic += "/";
        name_topic += topicBaseName;

        Ice::ObjectAdapterPtr adapter = communicator->createObjectAdapterWithEndpoints(name_topic, endpointConfig);
        auto proxy = adapter->addWithUUID(servant)->ice_oneway();

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
                IceStorm::QoS qos;
                topic->subscribeAndGetPublisher(qos, proxy);
            }
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
        haptic.frequency,
        0
    };
}

# pragma endregion Parsers

struct RobotMiddleware::Impl {
    bool isOk = true;
    Ice::CommunicatorHolder communicator;
    IceStorm::TopicManagerPrxPtr topicManager;
    RoboCompVRControllerPub::VRControllerPubPrxPtr vrcontroller_proxy;
    RoboCompLidar3D::Lidar3DPrxPtr lidar3d_proxy;
    RoboCompKinovaArm::KinovaArmPrxPtr arm_left_proxy, arm_right_proxy;

    //Haptic
    std::shared_ptr<IceStorm::TopicPrx> vrcontrollerpub_topic;
	Ice::ObjectPrxPtr vrcontrollerpub;
    Haptic left, right;
    std::mutex mtx;
    std::shared_ptr<VRControllerPubI> servant = std::make_shared<VRControllerPubI>(
        nullptr, 
        [this](const RoboCompVRControllerPub::Haptic& l, const RoboCompVRControllerPub::Haptic& r) {
            std::lock_guard<std::mutex> lock(mtx);
            this->left  = iceToHaptic(l);
            this->right = iceToHaptic(r);
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
                isOk = false;
            }
            std::cout << "\033[32mINFO\033[0m topicManager ptr: " << topicManager.get() << "\n";
            
            if (auto lidar3d_opt = require<RoboCompLidar3D::Lidar3DPrx, RoboCompLidar3D::Lidar3DPrxPtr>(ic,
                                "lidar3d:tcp -h localhost -p 11988", "Lidar3DProxy"))
                lidar3d_proxy = *lidar3d_opt;
            else{
                isOk = false;
                return;
            }

            if (auto arm_left_opt = require<RoboCompKinovaArm::KinovaArmPrx, RoboCompKinovaArm::KinovaArmPrxPtr>(ic,
                                "kinovaarm:tcp -h localhost -p 10006", "KinovaArmProxy"))
                arm_left_proxy = *arm_left_opt;
            else{
                isOk = false;
                return;
            }
            
            if (auto arm_right_opt = require<RoboCompKinovaArm::KinovaArmPrx, RoboCompKinovaArm::KinovaArmPrxPtr>(ic,
                                "kinovaarm:tcp -h localhost -p 10007", "KinovaArmProxy"))
                arm_right_proxy = *arm_right_opt;
            else{
                isOk = false;
                return;
            }

            if (auto vrcontroller_opt = publish<RoboCompVRControllerPub::VRControllerPubPrx, RoboCompVRControllerPub::VRControllerPubPrxPtr>(topicManager,
                            "", "VRControllerPub"))
                vrcontroller_proxy = *vrcontroller_opt;
            else{
                isOk = false;
                return;
            }

            if (not subscribe<VRControllerPubI>(ic, topicManager, "tcp -p 0", "haptic", "VRControllerPub", servant,
                            vrcontrollerpub_topic, vrcontrollerpub))
                isOk = false;
                return;
  
        }
        catch (const Ice::Exception& ex) {
             std::cout << "\033[31mERROR\033[0m Exception: 'rcnode' not running: " << ex << std::endl;
            std::cerr << "Ice exception: " << ex.what() << "\n";
            isOk = false;
        }
        catch (const std::exception& ex)
        {
            std::cout << "\033[31mERROR\033[0m Impl failed: " << ex.what() << std::endl;
            isOk = false;
        }
        catch (...)
        {
            std::cout << "\033[31mERROR\033[0m Exception: 'rcnode' not running: " << std::endl;
            std::cout << "\033[31mERROR\033[0m Impl failed: unknown non-std::exception"<< std::endl;
            isOk = false;
        }
    }
    ~Impl(){
		vrcontrollerpub_topic->unsubscribe(vrcontrollerpub);

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


};

RobotMiddleware::RobotMiddleware() {
    running = initIce();
}
RobotMiddleware::~RobotMiddleware() { 
}
bool RobotMiddleware::initIce()
{
    	this->pImpl = std::make_unique<Impl>();
        return this->pImpl->isOk;
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
    std::lock_guard<std::mutex> lock(pImpl->mtx);
    left  = pImpl->left;
    right = pImpl->right;
    return true;
}

std::vector<std::array<float, 3>> RobotMiddleware::getLidarData() {
    try {
        std::vector<std::array<float, 3>> cloudPoints = iceToCloudPoints(pImpl->lidar3d_proxy->getLidarData("", 0.0f, 180.0f, 1));
        return cloudPoints;
    }catch (...){
        std::cout <<  "\033[1;33mWARNING\033[0m Failed getting lidar data\n";
        return {};
    }
}

bool RobotMiddleware::getRobotState(float (&left)[8], float (&right)[8]) {
    try {
        // llamar a getJointsStateAsync, devuelve std::future<TJoints>
        std::future<RoboCompKinovaArm::TJoints> left_future  = pImpl->arm_left_proxy->getJointsStateAsync();
        std::future<RoboCompKinovaArm::TJoints> right_future = pImpl->arm_right_proxy->getJointsStateAsync();

        // opcional: bloquear hasta que estén listos
        RoboCompKinovaArm::TJoints left_joints  = left_future.get();
        RoboCompKinovaArm::TJoints right_joints = right_future.get();

        for (int i = 0; i < 8; ++i) {
            left[i]  = left_joints.joints[i].angle;
            right[i] = right_joints.joints[i].angle;
        }
        return true;
    } catch (...) {
        std::cout <<  "\033[1;33mWARNING\033[0m Failed to get robot state\n";
        return false;
    }
}

