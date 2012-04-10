#include "simple_render_policy.hpp"
#include "events.hpp"
#include "drawer_yg.hpp"
#include "window_handle.hpp"
#include "../yg/info_layer.hpp"
#include "../yg/internal/opengl.hpp"
#include "../yg/skin.hpp"

#include "../indexer/scales.hpp"
#include "../geometry/screenbase.hpp"

#include "../platform/platform.hpp"

SimpleRenderPolicy::SimpleRenderPolicy(VideoTimer * videoTimer,
                                       bool useDefaultFB,
                                       yg::ResourceManager::Params const & rmParams,
                                       shared_ptr<yg::gl::RenderContext> const & primaryRC)
  : RenderPolicy(primaryRC, false, 1)
{
  yg::ResourceManager::Params rmp = rmParams;

  rmp.checkDeviceCaps();

  rmp.m_primaryStoragesParams = yg::ResourceManager::StoragePoolParams(50000 * sizeof(yg::gl::Vertex),
                                                                       sizeof(yg::gl::Vertex),
                                                                       10000 * sizeof(unsigned short),
                                                                       sizeof(unsigned short),
                                                                       15,
                                                                       false,
                                                                       true,
                                                                       1,
                                                                       "primaryStorage",
                                                                       false,
                                                                       false);

  rmp.m_smallStoragesParams = yg::ResourceManager::StoragePoolParams(5000 * sizeof(yg::gl::Vertex),
                                                                     sizeof(yg::gl::Vertex),
                                                                     10000 * sizeof(unsigned short),
                                                                     sizeof(unsigned short),
                                                                     100,
                                                                     false,
                                                                     true,
                                                                     1,
                                                                     "smallStorage",
                                                                     false,
                                                                     false);

  rmp.m_blitStoragesParams = yg::ResourceManager::StoragePoolParams(10 * sizeof(yg::gl::Vertex),
                                                                    sizeof(yg::gl::Vertex),
                                                                    10 * sizeof(unsigned short),
                                                                    sizeof(unsigned short),
                                                                    50,
                                                                    true,
                                                                    true,
                                                                    1,
                                                                    "blitStorage",
                                                                    false,
                                                                    false);

  rmp.m_primaryTexturesParams = yg::ResourceManager::TexturePoolParams(512,
                                                                       256,
                                                                       10,
                                                                       rmp.m_texFormat,
                                                                       true,
                                                                       true,
                                                                       true,
                                                                       1,
                                                                       "primaryTexture",
                                                                       false,
                                                                       false);

  rmp.m_fontTexturesParams = yg::ResourceManager::TexturePoolParams(512,
                                                                    256,
                                                                    5,
                                                                    rmp.m_texFormat,
                                                                    true,
                                                                    true,
                                                                    true,
                                                                    1,
                                                                    "fontTexture",
                                                                    false,
                                                                    false);

  rmp.m_glyphCacheParams = yg::ResourceManager::GlyphCacheParams("unicode_blocks.txt",
                                                                 "fonts_whitelist.txt",
                                                                 "fonts_blacklist.txt",
                                                                 2 * 1024 * 1024,
                                                                 1,
                                                                 0);


  rmp.m_useSingleThreadedOGL = false;
  rmp.fitIntoLimits();

  m_resourceManager.reset(new yg::ResourceManager(rmp));

  Platform::FilesList fonts;
  GetPlatform().GetFontNames(fonts);
  m_resourceManager->addFonts(fonts);

  DrawerYG::params_t p;

  p.m_frameBuffer = make_shared_ptr(new yg::gl::FrameBuffer(useDefaultFB));
  p.m_resourceManager = m_resourceManager;
  p.m_dynamicPagesCount = 2;
  p.m_textPagesCount = 2;
  p.m_glyphCacheID = m_resourceManager->guiThreadGlyphCacheID();
  p.m_skinName = GetPlatform().SkinName();
  p.m_visualScale = GetPlatform().VisualScale();
  p.m_isSynchronized = true;

  m_drawer.reset(new DrawerYG(p));

  m_windowHandle.reset(new WindowHandle());

  m_windowHandle->setUpdatesEnabled(false);
  m_windowHandle->setRenderPolicy(this);
  m_windowHandle->setVideoTimer(videoTimer);
  m_windowHandle->setRenderContext(primaryRC);
}

void SimpleRenderPolicy::DrawFrame(shared_ptr<PaintEvent> const & e,
                               ScreenBase const & s)
{
  int scaleEtalonSize = GetPlatform().ScaleEtalonSize();

  m2::RectD glbRect;
  m2::PointD const pxCenter = s.PixelRect().Center();
  s.PtoG(m2::RectD(pxCenter - m2::PointD(scaleEtalonSize / 2, scaleEtalonSize / 2),
                   pxCenter + m2::PointD(scaleEtalonSize / 2, scaleEtalonSize / 2)),
         glbRect);

  shared_ptr<yg::InfoLayer> infoLayer(new yg::InfoLayer());

  DrawerYG * pDrawer = e->drawer();

  pDrawer->screen()->setInfoLayer(infoLayer);
  pDrawer->screen()->beginFrame();
  pDrawer->screen()->clear(m_bgColor);

  m_renderFn(e, s, s.ClipRect(), s.ClipRect(), scales::GetScaleLevel(glbRect), false);

  infoLayer->draw(pDrawer->screen().get(), math::Identity<double, 3>());
  pDrawer->screen()->resetInfoLayer();

  pDrawer->screen()->endFrame();
}
