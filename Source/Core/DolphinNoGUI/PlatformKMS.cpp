// Copyright 2018 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include "DolphinNoGUI/Platform.h"

#include "Common/MsgHandler.h"
#include "Core/ConfigManager.h"
#include "Core/Core.h"
#include "Core/State.h"

#include <array>
#include <memory>

#include <fmt/format.h>

#include <fcntl.h>
#include <gbm.h>
#include <stdio.h>
#include <unistd.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include "VideoCommon/RenderBase.h"

namespace
{


struct drm_fb {
	struct gbm_bo *bo;
	uint32_t fb_id;
};


struct ConnectorDeleter {
  void operator()(drmModeConnector *conn) const { drmModeFreeConnector(conn); }
};

using UniqueConnector = std::unique_ptr<drmModeConnector, ConnectorDeleter>;

static UniqueConnector makeConnector(int fd, uint32_t id) {
  return UniqueConnector(drmModeGetConnector(fd, id));
}

struct EncoderDeleter {
  void operator()(drmModeEncoder*conn) const { drmModeFreeEncoder(conn); }
};

using UniqueEncoder = std::unique_ptr<drmModeEncoder, EncoderDeleter>;

static UniqueEncoder makeEncoder(int fd, uint32_t id) {
  return UniqueEncoder(drmModeGetEncoder(fd, id));
}

struct Drm {
  Drm(char* device) {
    fd = open(device, O_RDWR);

    if (fd > 0) {
      resources = drmModeGetResources(fd);


      if (resources == nullptr)
        return;

      std::fprintf(stderr, "opened %s\n", device);

      // Find a connector with connected display
      findConnector();
      if (!connector) {
        std::fprintf(stderr, "no display connected");
      }

      chooseDefaultMode();

      crtc_id = findCrtc();

      fmt::print("Using connector {} and crtc {}\n", connector->connector_id, crtc_id);
    }
  }

  ~Drm() {
    if (resources != nullptr)
      drmModeFreeResources(resources);
  }

  bool is_ok() {
    return fd > 0 && resources != nullptr && connector && best_mode != -1 && crtc_id != -1;
  }

  drmModeModeInfo& getModeInfo() {
    return connector->modes[best_mode];
  }

  static std::unique_ptr<Drm> FindDrmDevice() {
    DrmDevices devices;

    for (auto device : devices) {
      printf("nodes %x\n", device->available_nodes);
      // Is this a primary device
      if (device->available_nodes & (1 << DRM_NODE_PRIMARY)) {
        // Check if it has KMS support
        auto drm = std::make_unique<Drm>(device->nodes[DRM_NODE_PRIMARY]);
        if (drm->is_ok())
          return drm;
      }
    }

    return {};
  }

  // Finds a connected connector
  void findConnector() {
    for (int i = 0; i < resources->count_connectors; i++) {
      connector = makeConnector(fd, resources->connectors[i]);

      if (connector&& connector->connection == DRM_MODE_CONNECTED) {
        return;
      }
    }

    connector.reset();
  }

  // Try to find the highest res display mode that is 60hz
  void chooseDefaultMode() {
    int current_score = 0;

    fmt::print("Avaliable display modes:\n");
    for (int i = 0; i < connector->count_modes; i++)
    {
      auto &mode = connector->modes[i];
      fmt::print("\t{} @ {}hz {:x}\n", mode.name, mode.vrefresh, mode.type);

      // Bias towards resolutions that are 60hz
      int score = mode.hdisplay * mode.vdisplay * (mode.vrefresh == 60 ? 5 : 1);
      if (mode.vdisplay == 480 && mode.hdisplay == 720 && mode.vrefresh == 60) // score > current_score)
      {
        current_score = score;
        best_mode = i;
      }
    }

    if (best_mode >= 0)
    {
      auto &mode = connector->modes[best_mode];
      fmt::print("Chosen {} @ {}hz\n", mode.name, mode.vrefresh);
    }
  }

  int findCrtc() {
    UniqueEncoder encoder;

    // Check the already connected encoder+crtc
    if (connector->encoder_id)
      encoder = makeEncoder(fd, connector->encoder_id);
    if (encoder && encoder->crtc_id) // Does it have a crtc
      return encoder->crtc_id;

    // Note: If we want to support multiple displays later, we need to
    //       make sure only one encoder+crtc is assigned per connector

    // Otherwise, we need to assign a new encoder.
    for (int i = 0; i < connector->count_encoders; i++)
    {
      encoder = makeEncoder(fd, connector->encoders[i]);
      if (!encoder) continue;

      // iterate over all crtcs
      for (int j = 0; j < resources->count_crtcs; j++)
      {
        int id = resources->crtcs[j];
        if (id >= 0 && encoder->possible_crtcs & (1 << j))
          return id;
      }
    }

    return -1;
  }


  int fd;
  int crtc_id = -1;
  UniqueConnector connector;
  int best_mode = -1;

private:
  // Wrapper for RAII
  struct DrmDevices : std::vector<drmDevicePtr> {
    static constexpr int MAX_DRM_DEVICES = 256;

    DrmDevices() {
      resize(MAX_DRM_DEVICES);
      int numDevices = drmGetDevices2(0, data(), MAX_DRM_DEVICES);
      resize(std::max(0, numDevices));
    }

    ~DrmDevices() {
      drmFreeDevices(data(), size());
    }
  };




  drmModeRes *resources = nullptr;

};

static void drm_fb_destroy_callback(struct gbm_bo *bo, void *data) {
  int drm_fd = gbm_device_get_fd(gbm_bo_get_device(bo));
	struct drm_fb *fb = (struct drm_fb *)data;

	if (fb->fb_id)
		drmModeRmFB(drm_fd, fb->fb_id);

	free(fb);
}


struct GbmDevice {
  GbmDevice(Drm &drm) {
    dev = gbm_create_device(drm.fd);

    if (!dev)
    {
      fmt::print(stderr, "Failed to create gbm device\n");
      return;
    }

    width = drm.getModeInfo().hdisplay;
    height = drm.getModeInfo().vdisplay;

    uint32_t format = GBM_FORMAT_XRGB8888;

    surface = gbm_surface_create(dev, width, height, format, GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);

    if (!surface)
    {
      fmt::print(stderr, "Failed to create gbm surface\n");
      return;
    }
  }

  ~GbmDevice() {
    if (dev)
      gbm_device_destroy(dev);
  }

  struct drm_fb* get_fb_from_bo(Drm& drm, struct gbm_bo* bo) {
    struct drm_fb* fb = reinterpret_cast<struct drm_fb*>(gbm_bo_get_user_data(bo));

    if (fb)
      return fb;

    fb = (struct drm_fb *)calloc(1, sizeof *fb);
    fb->bo = bo;

    uint32_t format = gbm_bo_get_format(bo);

    uint32_t /*strides[4] = {0}, handles[4] = {0}, offsets[4] = {0}, */flags = 0;
    std::array<uint32_t, 4> handles, strides, offsets;
    uint64_t modifiers[4] = {0};
    modifiers[0] = gbm_bo_get_modifier(bo);
    const int num_planes = gbm_bo_get_plane_count(bo);
    for (int i = 0; i < num_planes; i++)
    {
        strides[i] = gbm_bo_get_stride_for_plane(bo, i);
        handles[i] = gbm_bo_get_handle_for_plane(bo, i).u32;
        offsets[i] = gbm_bo_get_offset(bo, i);
        modifiers[i] = modifiers[0];
    }

    if (modifiers[0]) {
      flags = DRM_MODE_FB_MODIFIERS;
      fmt::print("Using modifier {:x}\n", modifiers[0]);
    }

    int ret = drmModeAddFB2WithModifiers(drm.fd, width, height, format, handles.data(), strides.data(), offsets.data(), modifiers, &fb->fb_id, flags);

    if (ret)
    {
      if (flags)
        fmt::print(stderr, "Modifiers failed\n");

      handles = {gbm_bo_get_handle(bo).u32,0,0,0};
      strides = {gbm_bo_get_stride(bo),0,0,0};
      offsets = {0, 0, 0, 0};
      ret = drmModeAddFB2(drm.fd, width, height, format,
          handles.data(), strides.data(), offsets.data(), &fb->fb_id, 0);
    }

    if (ret) {
      fmt::print("Failed to create fb: {}", errno);
      free(fb);
      return NULL;
    }

    gbm_bo_set_user_data(bo, fb, drm_fb_destroy_callback);

    return fb;
  }

  gbm_device *dev;
  gbm_surface *surface;
  int width;
  int height;

};


class PlatformKMS : public Platform
{
public:
  ~PlatformKMS() override;

  bool Init() override;
  void SetTitle(const std::string& string) override;
  void MainLoop() override;
  void Swap(int);

  WindowSystemInfo GetWindowSystemInfo() const override;

private:
  bool OpenFramebuffer();

  std::unique_ptr<Drm> drm;
  std::unique_ptr<GbmDevice> gbm;

  bool modeset = false;
  struct gbm_bo *prev_bo = nullptr;

  std::function<void(int)> swap_function;
};

PlatformKMS::~PlatformKMS()
{

}

bool PlatformKMS::Init()
{
  drm = Drm::FindDrmDevice();
  if (!drm || !drm->is_ok())
    return false;

  gbm = std::make_unique<GbmDevice>(*drm);
  if (!gbm)
    return false;

  //struct gbm_bo *bo = gbm_surface_lock_front_buffer(gbm->surface);
  //struct drm_fb *fb = gbm->get_fb_from_bo(*drm, bo);

  //if(drmModeSetCrtc(drm->fd, drm->crtc_id, fb->fb_id, 0, 0, &drm->connector->connector_id, 1, &drm->getModeInfo()))
  {
    //fmt::print(stderr, "Failed to set mode: {}", errno);
  }

  swap_function = [this](int interval) { this->Swap(interval); };

  return true;
}

void PlatformKMS::SetTitle(const std::string& string)
{
  std::fprintf(stdout, "%s\n", string.c_str());
}

void PlatformKMS::MainLoop()
{
  while (IsRunning())
  {
    UpdateRunningFlag();
    Core::HostDispatchJobs();

    // TODO: Is this sleep appropriate?
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}

void PlatformKMS::Swap(int interval) {
  struct gbm_bo *bo = gbm_surface_lock_front_buffer(gbm->surface);
  struct drm_fb *fb = gbm->get_fb_from_bo(*drm, bo);

  if (!fb)
  {
    fmt::print(stderr, "Failed to get new framebuffer BO");
    return;
  }

  if (!modeset)
  {
    if(drmModeSetCrtc(drm->fd, drm->crtc_id, fb->fb_id, 0, 0, &drm->connector->connector_id, 1, &drm->getModeInfo()))
    {
      fmt::print(stderr, "Failed to set mode: {}", errno);
    }
    modeset = true;
  }
  else
  {
    drmModePageFlip(drm->fd, drm->crtc_id, fb->fb_id, DRM_MODE_PAGE_FLIP_EVENT, nullptr);
  }

  if (prev_bo)
    gbm_surface_release_buffer(gbm->surface, prev_bo);
  prev_bo = bo;
}

WindowSystemInfo PlatformKMS::GetWindowSystemInfo() const
{
  fmt::print("GetWindowSystemInfo\n");
  WindowSystemInfo wsi;
  wsi.type = WindowSystemType::KMS;
  wsi.display_connection = gbm->dev;
  wsi.render_window = nullptr;
  wsi.render_surface = gbm->surface;
  wsi.swap_function = swap_function;
  return wsi;
}



}  // namespace

std::unique_ptr<Platform> Platform::CreateKMSPlatform()
{
  return std::make_unique<PlatformKMS>();
}
