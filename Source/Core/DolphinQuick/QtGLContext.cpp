
#include "DolphinQuick/PlatformQt.h"

#include "Common/GL/GLContext.h"

#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QSurface>

class QtGLContext : public GLContext
{
public:
  QtGLContext(std::shared_ptr<QSurface> surface, QOpenGLContext* shareContext, bool egl);
  ~QtGLContext() override;

  bool Initialize(const WindowSystemInfo& wsi, bool stereo, bool core) override;
  bool MakeCurrent() override;
  bool ClearCurrent() override;

  void Update() override;

  bool IsHeadless() const override { return m_headless; }

  void SwapInterval(int Interval) override;
  void Swap() override;

  void* GetFuncAddress(const std::string& name) override;
  std::unique_ptr<GLContext> CreateSharedContext() override;

private:
  std::shared_ptr<QSurface> m_surface;
  QOpenGLContext* m_share_context;
  bool m_egl;

  QOpenGLContext* m_context = nullptr;
  bool m_headless;
};

QtGLContext::QtGLContext(std::shared_ptr<QSurface> surface, QOpenGLContext* shareContext, bool egl)
    : m_surface(surface), m_share_context(shareContext), m_egl(egl)
{
  m_headless = surface->surfaceClass() == QSurface::Offscreen;
}

std::unique_ptr<GLContext> makeQtGLContext(std::shared_ptr<QSurface> surface,
                                           QOpenGLContext* shareContext, bool egl)
{
  return std::make_unique<QtGLContext>(surface, shareContext, egl);
}

bool QtGLContext::Initialize(const WindowSystemInfo& wsi, bool stereo, bool core)
{
  if (!m_context)
    m_context = new QOpenGLContext();

  if (m_share_context)
    m_context->setShareContext(m_share_context);

  auto format = m_surface->format();
  format.setStereo(stereo);

  if (core)
    format.setProfile(QSurfaceFormat::CoreProfile);

  format.setSwapBehavior(QSurfaceFormat::TripleBuffer);

  m_context->setFormat(format);
  m_backbuffer_height = m_surface->size().height();
  m_backbuffer_width = m_surface->size().width();

  m_context->create();
  MakeCurrent();

  glClearColor(0.2f, 0.0f, 0.8f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  Swap();

  printf("OpenGl version %i.%i, valid=%i\n", m_context->format().majorVersion(),
         m_context->format().minorVersion(), m_context->isValid());

  m_opengl_mode = m_context->isOpenGLES() ? Mode::OpenGLES : Mode::OpenGL;
  return m_context->isValid();
}

QtGLContext::~QtGLContext()
{
  delete m_context;
}

bool QtGLContext::MakeCurrent()
{
  return m_context->makeCurrent(m_surface.get());
}

bool QtGLContext::ClearCurrent()
{
  m_context->doneCurrent();
  return true;
}

void QtGLContext::Update()
{
  m_backbuffer_height = m_surface->size().height();
  m_backbuffer_width = m_surface->size().width();
}

void QtGLContext::SwapInterval(int Interval)
{
  // TODO: Implement this, somehow
}

void QtGLContext::Swap()
{
  m_context->swapBuffers(m_surface.get());
}

void* QtGLContext::GetFuncAddress(const std::string& name)
{
  auto addr = m_context->getProcAddress(name.c_str());
  return reinterpret_cast<void*>(addr);
}

std::unique_ptr<GLContext> QtGLContext::CreateSharedContext()
{
  auto surface = std::make_shared<QOffscreenSurface>();
  surface->setFormat(m_context->format());

  auto ctx = std::make_unique<QtGLContext>(std::move(surface), m_context, m_egl);

  bool stereo = m_context->format().stereo();
  bool core = m_context->format().profile() == QSurfaceFormat::CoreProfile;
  WindowSystemInfo wsi;

  if (ctx->Initialize(wsi, stereo, core))
  {
    return ctx;
  }
  return nullptr;
}
