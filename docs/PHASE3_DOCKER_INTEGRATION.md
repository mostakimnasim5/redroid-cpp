# VirtualPhonePro - Phase 3: Docker Integration

## Overview

Phase 3 implements the Docker Engine integration layer that manages ReDroid container lifecycle.

## Docker Architecture

### Communication Methods

```
┌─────────────────┐     ┌─────────────────┐     ┌─────────────────┐
│ Named Pipe      │     │ TCP REST API    │     │ Docker SDK      │
│ \\.\pipe\docker │     │ localhost:2375  │     │ libdocker       │
└────────┬────────┘     └────────┬────────┘     └────────┬────────┘
         │                       │                       │
         └───────────────────────┼───────────────────────┘
                                 │
                    ┌────────────┴────────────┐
                    │     Docker Engine        │
                    │   (WSL2 Backend)        │
                    └────────────┬────────────┘
                                 │
                    ┌────────────┴────────────┐
                    │   ReDroid Containers     │
                    └─────────────────────────┘
```

## Components

### 3.1 DockerEngine

```cpp
// src/docker/DockerEngine.h
#pragma once

#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>

namespace VirtualPhonePro {

using json = nlohmann::json;

struct ContainerInfo {
    std::string id;
    std::string name;
    std::string image;
    std::string status;
    std::string state;
    std::string ipAddress;
    int portsCount;
    time_t created;
};

struct ContainerStats {
    uint64_t memoryUsage;
    uint64_t memoryLimit;
    double cpuPercent;
    uint64_t networkRx;
    uint64_t networkTx;
};

class DockerEngine {
public:
    enum class ConnectionType {
        NamedPipe,
        TCPREST,
        DockerSDK
    };
    
    static DockerEngine& getInstance();
    
    // Connection
    bool connect(ConnectionType type = ConnectionType::TCPREST);
    bool isConnected() const;
    std::string getVersion();
    
    // Container Operations
    std::vector<ContainerInfo> listContainers(bool all = false);
    std::optional<ContainerInfo> getContainer(const std::string& idOrName);
    
    std::string createContainer(const json& config);
    bool startContainer(const std::string& idOrName);
    bool stopContainer(const std::string& idOrName, int timeout = 10);
    bool restartContainer(const std::string& idOrName, int timeout = 10);
    bool removeContainer(const std::string& idOrName, bool force = false);
    
    // Container Info
    json inspectContainer(const std::string& idOrName);
    ContainerStats getContainerStats(const std::string& idOrName);
    std::string getContainerLogs(const std::string& idOrName, int tail = 100);
    
    // Image Operations
    std::vector<std::string> listImages();
    bool pullImage(const std::string& imageName, 
                   std::function<void(float)> progress = nullptr);
    
    // Network
    std::string createNetwork(const std::string& name, const std::string& subnet);
    bool removeNetwork(const std::string& networkId);
    
    // System
    bool ping();
    json getSystemInfo();
    
private:
    DockerEngine() = default;
    
    bool sendRequest(const std::string& method,
                     const std::string& endpoint,
                     const json& body = {},
                     std::string& response);
    
    ConnectionType m_connectionType;
    std::string m_baseUrl;
    bool m_connected = false;
};

} // namespace VirtualPhonePro
```

### 3.2 ContainerConfig

```cpp
// src/docker/ContainerConfig.h
#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace VirtualPhonePro {

using json = nlohmann::json;

struct PortMapping {
    int hostPort;
    int containerPort;
    std::string protocol = "tcp";
    
    json toJson() const {
        return {
            {"hostPort", hostPort},
            {"containerPort", containerPort},
            {"protocol", protocol}
        };
    }
};

struct DeviceMapping {
    std::string hostPath;
    std::string containerPath;
    std::string permissions = "rwm";
    
    json toJson() const {
        return {
            {"PathOnHost", hostPath},
            {"PathInContainer", containerPath},
            {"CgroupPermissions", permissions}
        };
    }
};

struct VolumeMapping {
    std::string hostPath;
    std::string containerPath;
    bool readOnly = false;
    
    json toJson() const {
        return {
            {"Source", hostPath},
            {"Destination", containerPath},
            {"RW", !readOnly}
        };
    }
};

struct EnvironmentVariable {
    std::string key;
    std::string value;
};

class ContainerConfigBuilder {
public:
    ContainerConfigBuilder& setName(const std::string& name);
    ContainerConfigBuilder& setImage(const std::string& image);
    
    // Device identity environment variables
    ContainerConfigBuilder& setManufacturer(const std::string& value);
    ContainerConfigBuilder& setModel(const std::string& value);
    ContainerConfigBuilder& setAndroidVersion(const std::string& value);
    ContainerConfigBuilder& setDeviceProfileId(const std::string& value);
    
    // GPU mode: host, swiftshader, software
    ContainerConfigBuilder& setGPUMode(const std::string& mode);
    
    // Resource limits
    ContainerConfigBuilder& setMemoryLimit(const std::string& limit); // e.g., "512m"
    ContainerConfigBuilder& setCPUCount(int cores);
    
    // Networking
    ContainerConfigBuilder& addPortMapping(const PortMapping& mapping);
    ContainerConfigBuilder& addPortMappings(const std::vector<PortMapping>& mappings);
    
    // Devices
    ContainerConfigBuilder& addDevice(const DeviceMapping& device);
    ContainerConfigBuilder& addDevices(const std::vector<DeviceMapping>& devices);
    
    // Volumes
    ContainerConfigBuilder& addVolume(const VolumeMapping& volume);
    ContainerConfigBuilder& addVolumes(const std::vector<VolumeMapping>& volumes);
    
    // Environment
    ContainerConfigBuilder& addEnv(const std::string& key, const std::string& value);
    ContainerConfigBuilder& addEnvs(const std::vector<EnvironmentVariable>& envs);
    
    // Privileged mode
    ContainerConfigBuilder& setPrivileged(bool privileged);
    
    // Health check
    ContainerConfigBuilder& setHealthCheck(const std::string& test, int interval = 30);
    
    // Labels
    ContainerConfigBuilder& addLabel(const std::string& key, const std::string& value);
    
    json build() const;
    
private:
    std::string m_name;
    std::string m_image;
    std::vector<PortMapping> m_ports;
    std::vector<DeviceMapping> m_devices;
    std::vector<VolumeMapping> m_volumes;
    std::vector<EnvironmentVariable> m_envs;
    std::map<std::string, std::string> m_labels;
    
    std::string m_gpuMode = "swiftshader";
    std::string m_memoryLimit = "768m";
    int m_cpuCount = 2;
    bool m_privileged = true;
    
    std::string m_manufacturer = "Samsung";
    std::string m_model = "Galaxy S24 Ultra";
    std::string m_androidVersion = "14";
    std::string m_profileId;
};

} // namespace VirtualPhonePro
```

### 3.3 Default ReDroid Configuration

```cpp
// Standard ReDroid container configuration
json getDefaultReDroidConfig(int instanceNumber) {
    return ContainerConfigBuilder()
        .setName("redroid-" + std::to_string(instanceNumber))
        .setImage("ghcr.io/redroid/redroid:14.0.0_google_64only")
        .setGPUMode("swiftshader")  // Use swiftshader for compatibility
        .setMemoryLimit("768m")
        .setCPUCount(2)
        .setManufacturer("Samsung")
        .setModel("Galaxy S24 Ultra")
        .setAndroidVersion("14")
        .setPrivileged(true)
        .addPortMapping({5555 + (instanceNumber * 2), 5555})  // ADB
        .addPortMapping({5900 + (instanceNumber * 2), 5900})  // VNC
        .addDevice({"/dev/binderfs/binder", "/dev/binderfs/binder", "rwm"})
        .addDevice({"/dev/binder", "/dev/binder", "rwm"})
        .addDevice({"/dev/hwbinder", "/dev/hwbinder", "rwm"})
        .addDevice({"/dev/vndbinder", "/dev/vndbinder", "rwm"})
        .addDevice({"/dev/kvm", "/dev/kvm", "rwm"})
        .addEnv("ROG_BOOTANIMATION", "false")
        .addEnv("ROG_DISABLE_FPS_LIMIT", "true")
        .addLabel("virtualphonepro.instance", std::to_string(instanceNumber))
        .addLabel("virtualphonepro.version", "1.0.0")
        .setHealthCheck("getprop sys.boot_completed", 30)
        .build();
}
```

## Docker Compose Template

```yaml
# docker/docker-compose.yml
version: '3.8'

services:
  redroid-base:
    image: ghcr.io/redroid/redroid:14.0.0_google_64only
    privileged: true
    shm_size: 256mb
    stop_grace_period: 60s

  redroid-instance-1:
    build:
      context: ..
      dockerfile: docker/Dockerfile.redroid
    extends:
      service: redroid-base
    container_name: redroid-1
    environment:
      - DEVICE_MANUFACTURER=${DEVICE_MANUFACTURER:-Samsung}
      - DEVICE_MODEL=${DEVICE_MODEL:-Galaxy S24 Ultra}
      - REDROID_GPU_MODE=${GPU_MODE:-swiftshader}
    ports:
      - "5555:5555"  # ADB
      - "5900:5900"  # VNC
    volumes:
      - ./data/redroid-1:/data
      - ./profiles:/opt/profiles:ro
    networks:
      redroid-network:
        ipv4_address: 172.28.0.2

networks:
  redroid-network:
    driver: bridge
    ipam:
      config:
        - subnet: 172.28.0.0/16
```

## Implementation Tasks

### Task 3.1: Docker Engine Client
- [ ] Implement TCP REST API client
- [ ] Implement container creation
- [ ] Implement container lifecycle
- [ ] Implement container inspection
- [ ] Implement stats monitoring

### Task 3.2: Container Configuration
- [ ] Implement builder pattern
- [ ] Implement default configurations
- [ ] Implement validation

### Task 3.3: Resource Management
- [ ] Implement memory limiting
- [ ] Implement CPU allocation
- [ ] Implement network isolation
- [ ] Implement volume management

---

## Next Phase

[Phase 4: ADB Bridge](./PHASE4_ADB_BRIDGE.md)

---

*VirtualPhonePro - Phase 3*
