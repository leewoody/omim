#pragma once

#include "../base/thread.hpp"
#include "../base/condition.hpp"
#include "../base/commands_queue.hpp"
#include "../geometry/rect2d.hpp"
#include "../geometry/screenbase.hpp"
#include "../std/list.hpp"
#include "../std/function.hpp"
#include "../yg/color.hpp"
#include "../yg/tile_cache.hpp"
#include "../yg/tiler.hpp"
#include "render_policy.hpp"

class DrawerYG;

namespace threads
{
  class Condition;
}

class PaintEvent;
class WindowHandle;
class RenderQueue;

namespace yg
{
  class ResourceManager;


  namespace gl
  {
    class RenderContext;
    class FrameBuffer;
    class RenderBuffer;
    class BaseTexture;
    class RenderState;
    class RenderState;
    class Screen;
  }
}

class RenderQueueRoutine : public threads::IRoutine
{
public:

  typedef RenderPolicy::TRenderFn TRenderFn;

  /// Single tile rendering command
  struct Command
  {
    yg::Tiler::RectInfo m_rectInfo;
    shared_ptr<PaintEvent> m_paintEvent; //< paintEvent is set later after construction
    TRenderFn m_renderFn;
    size_t m_sequenceID;
    Command(yg::Tiler::RectInfo const & rectInfo,
            TRenderFn renderFn,
            size_t sequenceID);
  };

private:

  shared_ptr<yg::gl::RenderContext> m_renderContext;
  shared_ptr<yg::gl::FrameBuffer> m_frameBuffer;
  shared_ptr<DrawerYG> m_threadDrawer;

  threads::Mutex m_mutex;
  shared_ptr<Command> m_currentCommand;

  shared_ptr<yg::ResourceManager> m_resourceManager;

  /// A list of window handles to notify about ending rendering operations.
  list<shared_ptr<WindowHandle> > m_windowHandles;

  double m_visualScale;
  string m_skinName;
  bool m_isBenchmarking;
  unsigned m_scaleEtalonSize;
  yg::Color m_bgColor;

  size_t m_threadNum;

  RenderQueue * m_renderQueue;

  bool HasTile(yg::Tiler::RectInfo const & rectInfo);
  void AddTile(yg::Tiler::RectInfo const & rectInfo, yg::Tile const & tile);

public:

  RenderQueueRoutine(string const & skinName,
                     bool isBenchmarking,
                     unsigned scaleEtalonSize,
                     yg::Color const & bgColor,
                     size_t threadNum,
                     RenderQueue * renderQueue);
  /// initialize GL rendering
  /// this function is called just before the thread starts.
  void InitializeGL(shared_ptr<yg::gl::RenderContext> const & renderContext,
                    shared_ptr<yg::ResourceManager> const & resourceManager,
                    double visualScale);
  /// This function should always be called from the main thread.
  void Cancel();
  /// Thread procedure
  void Do();
  /// invalidate all connected window handles
  void Invalidate();
  /// add monitoring window
  void AddWindowHandle(shared_ptr<WindowHandle> window);
  /// add model rendering command to rendering queue
  void AddCommand(TRenderFn const & fn, yg::Tiler::RectInfo const & rectInfo, size_t seqNumber);
  /// free all available memory
  void MemoryWarning();
  /// free all easily recreatable opengl resources and make sure that no opengl call will be made.
  void EnterBackground();
  /// recreate all necessary opengl resources and prepare to run in foreground.
  void EnterForeground();
};
