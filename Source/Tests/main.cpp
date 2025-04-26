
#include "../MainHelper.h"
#include "core/core.h"
#include "renderer/renderer.h"
#include "interface/FullPipeline.h"
#include "interface/XmlSceneLoader.h"
#include "scene/BasicScene.h"

#include "../DebugCamera.h"

using namespace tim::core;
using namespace tim::resource;
using namespace tim::renderer;
using namespace tim::interface;
using namespace tim;

#ifdef __cplusplus
extern "C"
#endif
int main(int argc, char* argv[])
{
    tim::core::init();
    {
        const uivec2 resolution = { 800, 600 };
        initContextSDL(resolution.x(), resolution.y());
        tim::renderer::init();
        interface::ShaderPool::instance().add("gPass", "shader/gBufferPass.vert", "shader/gBufferPass.frag").value();
        interface::ShaderPool::instance().add("gPassAlphaTest", "shader/gBufferPass.vert", "shader/gBufferPass.frag", "", { "ALPHA_TEST" }).value();
        interface::ShaderPool::instance().add("fxaa", "shader/fxaa.vert", "shader/fxaa.frag").value();
        interface::ShaderPool::instance().add("combineScene", "shader/combineScene.vert", "shader/combineScene.frag").value();

        {
            SDLInputManager input;
            DebugCamera freeCamera(&input);

            interface::FullPipeline::Parameter pipelineParams;
            interface::FullPipeline renderingPipeline;
            renderingPipeline.create(uivec2(resolution.x(), resolution.y()), pipelineParams);

            interface::Scene scene;
            interface::View view;
            view.camera.pos = vec3(-3, -3, 2);
            view.camera.dir = vec3(1, 0, 0);
            view.camera.up = vec3(0, 0, 1);
            view.camera.fov = 70.f;
            view.camera.ratio = (float)resolution.x() / resolution.y();
            view.camera.clipDist = vec2(0.1f, 100.f);

            std::vector<XmlSceneLoader::ObjectLoaded> objects;
            XmlSceneLoader::loadScene("testScene.xml", scene, objects);
            renderingPipeline.setScene(scene, view, 0);

            float timeElapsed = 0, totalTime = 0;

            while (!input.keyState(SDLK_ESCAPE).pressed)
            {
                SDLTimer timer;
                input.getEvent();

                freeCamera.update(timeElapsed, view.camera);

                renderingPipeline.pipeline()->prepare();
                renderingPipeline.pipeline()->render();

                SDL_GL_SwapWindow(g_pWindow);

                timeElapsed = timer.elapsed() * 0.001;
                totalTime += timeElapsed;
            }

        }
        /** Close context **/

        tim::renderer::close();
        delContextSDL();
    }
    tim::core::quit();

    return 0;
}

