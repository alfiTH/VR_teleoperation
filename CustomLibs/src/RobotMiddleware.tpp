#include <Ice/Communicator.h>
#include <IceStorm/IceStorm.h>
#include <optional>

template <typename SubInterfaceType>
bool subscribe(const Ice::CommunicatorPtr &communicator,
               const IceStorm::TopicManagerPrxPtr &topicManager,
               const std::string &endpointConfig, std::string name_topic,
               const std::string &topicBaseName,
               std::shared_ptr<SubInterfaceType> servant,
               std::shared_ptr<IceStorm::TopicPrx> &topic,
               Ice::ObjectPrxPtr &proxy) {
  bool ok = true;
  try {
    if (!name_topic.empty())
      name_topic += "/";
    name_topic += topicBaseName;

    Ice::ObjectAdapterPtr adapter =
        communicator->createObjectAdapterWithEndpoints(name_topic,
                                                       endpointConfig);
    proxy = adapter->addWithUUID(servant)->ice_oneway();

    try {
      std::cout << "\033[32mINFO\033[0m " << "creating topic\n";
      topic = topicManager->create(name_topic);
    } catch (...) {
      try {
        std::cout << "\033[1;33mWARNING\033[0m subscribing the " << name_topic
                  << " topic. It's possible that other component have created, "
                     "retrieving\n";
        topic = topicManager->retrieve(name_topic);
      } catch (const IceStorm::NoSuchTopic &ex) {
        std::cout << "\033[31mERROR\033[0m Topic " << name_topic
                  << " does not exist: " << ex << std::endl;
        ok = false;
      } catch (const Ice::Exception &ex) {
        std::cout << "\033[31mERROR\033[0m Ice exception retrieving topic "
                  << name_topic << ": " << ex.what() << std::endl;
        ok = false;
      } catch (const std::exception &ex) {
        std::cout << "\033[31mERROR\033[0m std::exception retrieving topic "
                  << name_topic << ": " << ex.what() << std::endl;
        ok = false;
      } catch (...) {
        std::cout << "\033[31mERROR\033[0m subscribing the " << name_topic
                  << " unknown non-std::exception" << std::endl;
        ok = false;
      }
      if (topic) {
        IceStorm::QoS qos;
        topic->subscribeAndGetPublisher(qos, proxy);
      }
    adapter->activate();
    }
  } catch (...) {
    std::cout << "\033[31mERROR\033[0m subscribing the " << name_topic
              << " to created adapter, endpoint: " << endpointConfig
              << std::endl;
    ok = false;
  }
  return ok;
}

template <typename PubProxyType, typename PubProxyPointer>
std::optional<PubProxyPointer>
publish(const IceStorm::TopicManagerPrxPtr &topicManager,
        std::string name_topic, const std::string &topicBaseName) {
  if (!name_topic.empty())
    name_topic += "/";
  name_topic += topicBaseName;
  std::shared_ptr<IceStorm::TopicPrx> topic;
  try {
    std::cout << "\033[32mINFO\033[0m " << "creating topic\n";
    topic = topicManager->retrieve(name_topic);
  } catch (...) {
    try {
      std::cout << "\033[1;33mWARNING\033[0m publishing the " << name_topic
                << " topic. It's possible that other component have created, "
                   "retrieving\n";
      topic = topicManager->create(name_topic);
    } catch (const Ice::Exception &ex) {
      std::cout << "\033[31mERROR\033[0m Ice exception creating topic "
                << name_topic << ": " << ex.what() << std::endl;
      return std::nullopt;
    } catch (const std::exception &ex) {
      std::cout << "\033[31mERROR\033[0m std::exception creating topic "
                << name_topic << ": " << ex.what() << std::endl;
      return std::nullopt;
    } catch (...) {
      std::cout << "\033[31mERROR\033[0m publishing the " << name_topic
                << " unknown non-std::exception" << std::endl;
      return std::nullopt;
    }
  }

  if (!topic)
    return std::nullopt;

  try {

    auto publisher = topic->getPublisher();
    if (!publisher) {
      std::cout << "\033[31mERROR\033[0m  Publisher is null for topic "
                << name_topic << "\n";
      return std::nullopt;
    }
    auto pubProxy = Ice::uncheckedCast<PubProxyType>(publisher->ice_oneway());
    if (!pubProxy) {
      std::cout << "\033[31mERROR\033[0m  uncheckedCast failed for topic "
                << name_topic << "\n";
      return std::nullopt;
    }
    std::cout << "\033[32mINFO\033[0m " << name_topic << " initialized Ok!\n";
    return pubProxy;

  } catch (const Ice::Exception &ex) {
    std::cout << "\033[31mERROR\033[0m Failed to create publisher: " << ex
              << std::endl;
    return std::nullopt;
  } catch (...) {
    std::cout << "\033[31mERROR\033[0m Failed to create publisher: unknown "
                 "non-std::exception\n";
    return std::nullopt;
  }
};

template <typename ProxyType, typename ProxyPointer>
std::optional<ProxyPointer> require(const Ice::CommunicatorPtr &communicator,
                                    const std::string &proxyConfig,
                                    const std::string &proxyName) {
  try {
    ProxyPointer proxy =
        Ice::uncheckedCast<ProxyType>(communicator->stringToProxy(proxyConfig));
    std::cout << "\033[32mINFO\033[0m " << proxyName << " initialized Ok!\n";
    return proxy;
  } catch (const Ice::EndpointParseException &ex) {
    std::cout << "\033[31mERROR\033[0m Invalid endpoint for proxy " << proxyName
              << " (" << proxyConfig << "): " << ex.what() << std::endl;
    return std::nullopt;
  } catch (const Ice::ProxyParseException &ex) {
    std::cout << "\033[31mERROR\033[0m Invalid proxy string for " << proxyName
              << " (" << proxyConfig << "): " << ex.what() << std::endl;
    return std::nullopt;
  } catch (const Ice::Exception &ex) {
    std::cout << "\033[31mERROR\033[0m Ice exception creating proxy "
              << proxyName << ": " << ex.what() << std::endl;
    return std::nullopt;
  } catch (const std::exception &ex) {
    std::cout << "\033[31mERROR\033[0m std::exception creating proxy "
              << proxyName << ": " << ex.what() << std::endl;
    return std::nullopt;
  } catch (...) {
    std::cout << "\033[31mERROR\033[0m Unknown exception creating proxy "
              << proxyName << std::endl;
    return std::nullopt;
  }
}