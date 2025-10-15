# Thunder Compositor

A high-performance, VSync-synchronized compositor built on the Thunder framework, providing tear-free graphics composition with efficient multi-client support.

## 🚀 Overview

The Thunder Compositor manages multiple graphics clients (applications) and composites their surfaces into a single, synchronized output stream. It leverages DRM/KMS for direct hardware access and implements sophisticated VSync synchronization to eliminate screen tearing while maintaining optimal performance.

## 🏗️ Architecture

### Core Components

```
┌─────────────────┐    ┌──────────────────┐    ┌─────────────────┐
│     Clients     │    │    Presenter     │    │   VSync/DRM     │
│                 │    │     Thread       │    │    Backend      │
│ ┌─────────────┐ │    │                  │    │                 │
│ │ App A       │ │───▶│  ┌─────────────┐ │───▶│ ┌─────────────┐ │
│ │ App B       │ │    │  │ Render Loop │ │    │ │ Display     │ │
│ │ App C       │ │    │  │ Compositor  │ │    │ │ Hardware    │ │
│ │ ...         │ │    │  └─────────────┘ │    │ └─────────────┘ │
│ └─────────────┘ │    │                  │    │                 │
└─────────────────┘    └──────────────────┘    └─────────────────┘
        │                        │                        │
        │                        │                        │
        └─── Render Requests ────┴─── VSync Events ───────┘
```

### Key Classes

- **CompositorImplementation**: Main compositor logic and client management
- **Presenter**: Dedicated rendering thread with VSync synchronization  
- **Client**: Individual application surface with texture management
- **Output**: DRM connector interface with double-buffered rendering
- **VSync Handler**: Display timing synchronization

## ⚡ VSync Synchronization Architecture

The compositor implements a sophisticated producer-consumer pattern for perfect frame timing:

### The Yin-Yang Balance

```
    Presenter Thread              VSync Handler
    (Producer)                    (Consumer)
         │                            │
         ▼                            ▼
    ┌─────────────┐              ┌─────────────┐
    │   Render    │              │  Complete   │
    │ All Clients │              │   Frame     │
    │   (~3ms)    │              │   Cycle     │
    └─────────────┘              └─────────────┘
         │                            │
         ▼                            ▼
    ┌─────────────┐              ┌─────────────┐
    │    Wait     │◄────────────▶│   Grant     │
    │ Permission  │              │ Permission  │
    │  (Sleep)    │              │  (Signal)   │
    └─────────────┘              └─────────────┘
         │                            │
         ▼                            ▼
    ┌─────────────┐              ┌─────────────┐
    │   Commit    │              │   Notify    │
    │   Frame     │              │   Clients   │
    │ (Display)   │              │ "Published" │
    └─────────────┘              └─────────────┘
```

### Synchronization Primitives

```cpp
std::mutex _commitMutex;                // Protects commit permission state
std::condition_variable _commitCV;      // VSync → Presenter communication
std::atomic<bool> _canCommit;          // Permission flag (true = can commit)
```

### Timeline Flow

1. **Client Request** → `_present.Run()` (thread-safe, coalesced)
2. **Presenter Renders** → All client surfaces to back buffer (~3000μs)
3. **Presenter Waits** → `_commitCV.wait_for(_canCommit == true)`
4. **[BLOCKED]** → Presenter sleeps until VSync signal
5. **VSync Arrives** → Display completes previous frame scan-out
6. **Clients Signaled** → `client->Completed()` (frame published)
7. **Permission Granted** → `_canCommit = true`, `_commitCV.notify_one()`
8. **Presenter Wakes** → `_canCommit = false`, `_output->Commit()`
9. **New Frame Displays** → Cycle repeats with perfect timing

### Benefits

✅ **No Screen Tearing** - Commits synchronized to display refresh  
✅ **Perfect Frame Pacing** - One render per VSync, never faster/slower  
✅ **Efficient CPU Usage** - Presenter sleeps 90%+ of time between frames  
✅ **Request Coalescing** - Multiple client updates = single render  
✅ **No Commit Errors** - Guaranteed `_output->Commit()` success  
✅ **Client Continuity** - Clear "Published" signals for new content  

## 🧵 Threading Model

### Thread Safety

| Component | Thread | Synchronization |
|-----------|--------|-----------------|
| **Client Requests** | Any | `Core::Thread` framework coalescing |
| **Presenter** | Dedicated | Mutex + condition variable blocking |
| **VSync Handler** | DRM Event | Atomic operations + client mutex |
| **Client Lifecycle** | Multiple | Reference counting + observer pattern |

### Request Flow

```
Client Thread A ──┐
Client Thread B ──┼─→ Core::Thread ──→ Presenter ──→ VSync ──→ Display
Client Thread C ──┘    Framework       Thread        Handler
                        (Coalesce)      (Render)     (Signal)
```

## 🛠️ Configuration

### JSON Configuration

```json
{
  "render": "/dev/dri/renderD128",
  "output": "HDMI-A-1", 
  "width": 1920,
  "height": 1080,
  "format": "0x34325258",
  "modifier": "0x0",
  "autoscale": true
}
```

### Parameters

- **render**: DRM render node path
- **output**: Display connector name  
- **width/height**: Display resolution
- **format**: Pixel format (DRM fourcc)
- **modifier**: Buffer layout modifier
- **autoscale**: Auto-scale client surfaces to display

## 📊 Performance Characteristics

### Timing (Raspberry Pi 4)

| Operation | Duration | Notes |
|-----------|----------|-------|
| **Full Render** | ~3000μs | Multiple clients, complex scenes |
| **Client Render** | ~500-900μs | Individual surface rendering |
| **VSync Overhead** | ~10μs | Signal clients + grant permission |
| **Context Switch** | ~1-5μs | Presenter wake-up latency |

### Memory Usage

- **Double Buffered Output**: 2× framebuffer memory
- **Client Textures**: Per-application GPU memory
- **Minimal Overhead**: ~1MB for compositor structures

### CPU Efficiency  

- **Active Rendering**: ~5% CPU (during 3ms render)
- **VSync Wait**: ~0% CPU (thread sleeps)
- **Overall Load**: <1% average on 60fps display

## 🔧 Build Instructions

### Prerequisites

```bash
# Install dependencies
sudo apt install libdrm-dev libgbm-dev libegl1-mesa-dev
```

### Build

```bash
# Clone Thunder framework
git clone https://github.com/rdkcentral/Thunder.git
cd Thunder

# Build with compositor
mkdir build && cd build
cmake -DPLUGIN_COMPOSITOR=ON ..
make -j$(nproc)
```

### Installation

```bash
sudo make install
sudo systemctl enable thunder
```

## 🚀 Usage

### Start Compositor

```bash
# Thunder configuration
{
  "plugins": {
    "Compositor": {
      "classname": "Compositor",
      "startmode": "Activated",
      "configuration": {
        "render": "/dev/dri/renderD128",
        "output": "HDMI-A-1",
        "width": 1920,
        "height": 1080
      }
    }
  }
}
```

### Client Integration

```cpp
// Get compositor interface
auto compositor = service->QueryInterface<IComposition>();

// Create client surface  
auto client = compositor->CreateClient("MyApp", 800, 600);

// Configure surface
client->Geometry({100, 100, 800, 600});
client->Opacity(255);
client->ZOrder(10);

// Render content and request display
client->Request();
```

## 🐛 Troubleshooting

### Debug Output

Enable debug tracing to see the synchronization flow:

```
-> RenderOutput: start
-> RenderOutput: Rendering complete, waiting for commit permission  
<- VSync: called
<- VSync: commit now allowed
-> RenderOutput: Commit permission granted
-> RenderOutput: Rendering process done. commit: 0
```

### Common Issues

**Black Screen**
- Check DRM permissions: `ls -la /dev/dri/`
- Verify connector name: `modetest -M <gpu> -c`
- Confirm display connection and power

**Stuttering/Tearing**  
- Verify VSync events: Check debug output timing
- Monitor CPU load: `top -p $(pidof Thunder)`
- Check for commit errors in logs

**High CPU Usage**
- Profile render times: Look for slow client operations  
- Check for VSync timeout: 1000ms safety timeout
- Verify efficient request coalescing

**Client Issues**
- Ensure proper `Published()` signal handling
- Check texture creation and binding
- Verify client lifecycle management

## 📈 Monitoring

### Performance Metrics

```bash
# Monitor frame timing
grep "RenderOutput.*done" /var/log/thunder.log | tail -10

# Check VSync regularity  
grep "VSync.*called" /var/log/thunder.log | tail -10

# CPU usage tracking
top -p $(pidof Thunder)
```
## DRM Debug Commands

### Enable Debug Logging

#### Check and Mount debugfs

```bash
# Check if debugfs is mounted
mount | grep debugfs

# Mount debugfs (if not already mounted)
sudo mount -t debugfs none /sys/kernel/debug

# Make it persistent across reboots (optional)
echo "debugfs /sys/kernel/debug debugfs defaults 0 0" | sudo tee -a /etc/fstab
```

#### Enable DRM Kernel Debug Output

```bash
# Enable DRM debug logging
# Debug levels:
# 0x01 - Core DRM
# 0x02 - Driver-specific
# 0x04 - KMS (mode setting)
# 0x08 - Prime (buffer sharing)
# 0x10 - Atomic
# 0x1f - All common categories
echo 0x1f | sudo tee /sys/module/drm/parameters/debug

# Watch kernel logs in real-time
sudo dmesg -wH

# Disable debug logging when done
echo 0 | sudo tee /sys/module/drm/parameters/debug
```

### DRM Information Commands

#### Basic DRM Information

```bash
# DRM connector info
modetest -M <gpu> -c

# Display timing info  
modetest -M <gpu> -w <connector>:<property>:<value>

# Show all planes and their capabilities (including format/modifier support)
modetest -M <gpu> -p

# List available DRM devices
ls /sys/kernel/debug/dri/
```

#### Detailed State Information

```bash
# Buffer info
cat /sys/kernel/debug/dri/0/framebuffer

# Full DRM state (connectors, CRTCs, planes, properties)
sudo cat /sys/kernel/debug/dri/0/state

# Check specific VC4 info (Broadcom/Raspberry Pi)
sudo cat /sys/kernel/debug/dri/0/vc4_*
```

### Example: Debug Framebuffer Creation Issues

```bash
# 1. Enable DRM debugging
echo 0x1f | sudo tee /sys/module/drm/parameters/debug

# 2. Run your application in another terminal

# 3. Monitor kernel logs for DRM-specific messages
sudo dmesg | grep -i drm

# 4. Look for specific errors
sudo dmesg | grep -i "modifier\|addfb\|framebuffer"

# 5. Disable debugging when done
echo 0 | sudo tee /sys/module/drm/parameters/debug
```

### Common Issues and Their Debug Output

#### Modifier-related Issues

When you see errors like:
```
non-zero modifier for unused plane
```

This indicates that modifiers are being set for unused planes. Only active planes should have non-zero modifiers.

#### Format Not Supported

```
unsupported pixel format
```

Use `modetest -M <gpu> -p` to check which formats and modifiers are supported by each plane.

#### Invalid Argument (EINVAL)

```
ret=-22
```

Common causes:
- Incorrect modifier for unused planes
- Format/modifier combination not supported by hardware
- Invalid buffer dimensions or stride


## 🤝 Contributing

1. Fork the repository
2. Create a feature branch
3. Follow the existing code style
4. Add tests for new functionality  
5. Ensure VSync timing isn't disrupted
6. Submit a pull request

### Development Notes

- Maintain thread safety in all client-facing APIs
- Preserve VSync synchronization timing
- Add performance measurements for new features
- Document threading model changes

### Reads
- [Linux low level graphics part 1](https://www.collabora.com/news-and-blog/blog/2018/03/20/a-new-era-for-linux-low-level-graphics-part-1)
- [Linux low level graphics part 2](https://www.collabora.com/news-and-blog/blog/2018/03/23/a-new-era-for-linux-low-level-graphics-part-2)
- [DRM modifiers and hardware](https://www.linux.com/training-tutorials/optimizing-graphics-memory-bandwidth-compression-and-tiling-notes-drm-format-modifiers)
- [How to use hardware planes and all its quirks]( https://emersion.fr/blog/2019/xdc2019-wrap-up/#libliftoff)

### Resources
- [DRM DB](https://drmdb.emersion.fr)

## 📄 License

Licensed under the Apache License, Version 2.0. See LICENSE file for details.

---

*Built with ❤️ on the Thunder framework*