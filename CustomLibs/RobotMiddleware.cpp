#include "RobotMiddleware.h"
#include <Ice/Ice.h>  // solo aquí dentro
#include <IceStorm/IceStorm.h>
#include <memory>
#include <VRController.h>
#include <Lidar3D.h>


template <typename PubProxyType, typename PubProxyPointer>
void publish(const IceStorm::TopicManagerPrxPtr& topicManager,
             std::string name_topic,
             const std::string& topicBaseName,
             PubProxyPointer& pubProxy)
{
    if (!name_topic.empty()) name_topic += "/";
    name_topic += topicBaseName;

    std::cout << "\033[32mINFO\033[0m Topic: " 
              << name_topic << " will be used for publication. \033[0m\n";

     std::shared_ptr<IceStorm::TopicPrx> topic;
    while (!topic)
    {
        try
        {
            topic = topicManager->retrieve(name_topic);
        }
        catch (const IceStorm::NoSuchTopic&)
        {
            std::cout << "\033[1;33mWARNING\033[0m " 
                      << name_topic << " topic did not create. \033[32mCreating...\033[0m\n\n";
            try
            {
                topic = topicManager->create(name_topic);
            }
            catch (const IceStorm::TopicExists&)
            {
                std::cout << "\033[1;33mWARNING\033[0m publishing the " 
                          << name_topic << " topic. It's possible that other component have created\n";
            }
        }
        catch(const IceUtil::NullHandleException&)
        {
            std::cout << "\033[31mERROR\033[0m TopicManager is Null.\n";
            throw;
        }
    }
    auto publisher = topic->getPublisher()->ice_oneway();
    pubProxy = Ice::uncheckedCast<PubProxyType>(publisher);
};

template <typename ProxyType, typename ProxyPointer>
void require(const Ice::CommunicatorPtr& communicator,
             const std::string& proxyConfig, 
             const std::string& proxyName,
             ProxyPointer& proxy)
{
    try
    {
        proxy = Ice::uncheckedCast<ProxyType>(communicator->stringToProxy(proxyConfig));
        std::cout << proxyName << " initialized Ok!\n";
    }
    catch(const Ice::Exception& ex)
    {
        std::cout << "[" << PROGRAM_NAME << "]: Exception creating proxy " << proxyName << ": " << ex;
        throw;
    }
}

inline RoboCompVRController::Pose toIcePose(const RobotMiddleware::Pose& p) { return {p.x, p.y, p.z, p.rx, p.ry, p.rz}; }
inline RoboCompVRController::Controller toIceController(const RobotMiddleware::Controller& c) { 
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

struct RobotMiddleware::Impl {
    Ice::CommunicatorHolder communicator;
    IceStorm::TopicManagerPrxPtr topicManager;
    RoboCompVRController::VRControllerPrxPtr vrcontroller_proxy;
    RoboCompLidar3D::Lidar3DPrxPtr lidar3d_proxy;


    Impl() 
        : communicator(makeInitData())
    {
        try
        {
            auto ic = communicator.communicator();
            topicManager = Ice::checkedCast<IceStorm::TopicManagerPrx>(ic->stringToProxy("IceStorm/TopicManager:default -p 9999"));
            if (!topicManager)
            {
                std::cout << "TopicManager.Proxy not defined in config file."<<std::endl;
                std::cout << "	 Config line example: TopicManager.Proxy=IceStorm/TopicManager:default -p 9999"<<std::endl;
                throw std::runtime_error("TopicManager.Proxy not defined");
            }
        }
        catch (const Ice::Exception &ex)
        {
            std::cout << "Exception: 'rcnode' not running: " << ex << std::endl;
            throw std::runtime_error("TopicManager.Proxy not defined");
        }

        //Require code
        require<RoboCompLidar3D::Lidar3DPrx, RoboCompLidar3D::Lidar3DPrxPtr>(ic,
                            "lidar3d:tcp -h localhost -p 11988", "Lidar3DProxy", lidar3d_proxy);

        //Publish code
        publish<RoboCompVRController::VRControllerPrx, RoboCompVRController::VRControllerPrxPtr>(topicManager,
                            "",
                            "VRController", vrcontroller_proxy);



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

RobotMiddleware::RobotMiddleware() {}
RobotMiddleware::~RobotMiddleware() { delete pImpl; }

bool RobotMiddleware::initIce()
{
    try
    {
        this->pImpl = new Impl();
        return true;
    }
    catch(...)
    {
        return false;
    }
}

bool RobotMiddleware::sendPose(const RobotMiddleware::Pose& head, const RobotMiddleware::Pose& left, const RobotMiddleware::Pose& right) {
    try {
        pImpl->vrcontroller_proxy->sendPose(
            toIcePose(head),
            toIcePose(left),
            toIcePose(right)
        );
        return true;
    } catch (const Ice::Exception& ex) {
        std::cerr << "[RobotMiddleware] Error sending pose: " << ex << std::endl;
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
    } catch (const Ice::Exception& ex) {
        std::cerr << "[RobotMiddleware] Error sending controllers: " << ex << std::endl;
        return false;
    }
}

std::vector<std::array<float, 3>> RobotMiddleware::getLidarData() {
    try {
        std::vector<std::array<float, 3>> cloudPoints = iceToCloudPoints(pImpl->lidar3d_proxy->getLidarData("", 0.0f, 180.0f, 1));
        return cloudPoints;
    }catch (const Ice::Exception& ex){
        std::cerr << "[RobotMiddleware] Error getting lidar data: " << ex << std::endl;
        return {};
    }
}
