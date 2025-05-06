#include "MainHelper.h"
#include "cxxopts.h"

#include "OpenVR/OnHmdRenderer.h"
#include "OpenVR/HmdSceneView.h"
#include "OpenVR/VRDebugCamera.h"
#include "OpenVR/OpenVR_Device.h"
#include "OpenVR/SoftVR_Device.h"
#include "resource/AssetManager.h"

#include "MultiPromise.h"

#include "PortalGame/PortalGame.h"

#undef interface
using namespace tim::core;
using namespace tim::renderer;
using namespace tim::resource;
using namespace tim::interface;
using namespace tim;



int main(int argc, char* argv[])
{
    cxxopts::Options optionsBase("A portal adventure");
    optionsBase.allow_unrecognised_options();
    optionsBase.add_options()
        ("noVR", "Run the game wihtout the VR headset", cxxopts::value<bool>()->implicit_value("true")->default_value("false"))
        ("roomSize", "Set the room size (between 2-3)", cxxopts::value<float>()->default_value("-1"))
        ("zShift", "Shift the world up", cxxopts::value<float>()->default_value("-1"))
        ("debugRoomSize", "Room size for the debug camera.", cxxopts::value<float>()->default_value("100"));

    auto cmdArgParseRresult = optionsBase.parse(std::min(argc, 2), argv);

    uint RES_X = 1000;
    uint RES_Y = 1000;
    const uivec2 WIN_RES = { 2000, 1000 };

    float roomSize = cmdArgParseRresult["roomSize"].as<float>();
    
    if (roomSize < 0) {
        std::cout << "Enter the size of the room in meters (between 2 and 3) :";
        std::cin >> roomSize; std::cin.clear();
    }
    roomSize = std::min(std::max(2.f, roomSize), 3.f);

    float zShift = cmdArgParseRresult["zShift"].as<float>();
    if (zShift < 0) {
        std::cout << "Enter the height shift (in meters) you want to apply :";
        std::cin >> zShift; std::cin.clear();
    }
    zShift = std::min(std::max(0.f, zShift), 1.f);

    float debugCameraRoomSpace = cmdArgParseRresult["debugRoomSize"].as<float>();

    std::cout << "1. Tutorial\n2. Forest\n3. Maze\n4. Sacred Groove\n5. Ocean" << std::endl;
    std::cout <<std::endl<< "Enter the ID of the level you want to start in :";
    int indexLevel=1;
    std::cin >> indexLevel;
    indexLevel = std::min(std::max(1, indexLevel), 5);

	tim::core::init();
	{
		initContextSDL(WIN_RES.x(), WIN_RES.y());
		tim::renderer::init();
		resource::textureLoader = new SDLTextureLoader;

        LOG(openGL.strHardward(),"\n");
		{
            ShaderPool::instance().add("gPass", "shader/gBufferPass.vert", "shader/gBufferPass.frag").value();
            ShaderPool::instance().add("gPassAlphaTest", "shader/gBufferPass.vert", "shader/gBufferPass.frag", "", {"ALPHA_TEST"}).value();
            ShaderPool::instance().add("water", "shader/gBufferPass.vert", "shader/gBufferPass.frag", "", {"WATER_SHADER"}).value();
            ShaderPool::instance().add("portalShader", "shader/gBufferPass.vert", "shader/gBufferPass.frag", "", {"PORTAL_SHADER"}).value();
            ShaderPool::instance().add("fxaa", "shader/fxaa.vert", "shader/fxaa.frag").value();
            ShaderPool::instance().add("combineScene", "shader/combineScene.vert", "shader/combineScene.frag").value();
            ShaderPool::instance().add("feedbackStereo", "shader/combineScene.vert", "shader/combineScene.frag", "", {"STEREO_DISPLAY"}).value();
            ShaderPool::instance().add("processSpecularCubeMap", "shader/processCubemap.vert", "shader/processCubemap.frag").value();

            SDLInputManager input;
            VR_DeviceInterface* pVRDevice = nullptr;
            if (cmdArgParseRresult["noVR"].as<bool>()) {
                pVRDevice = new SoftVR_Device(new VRDebugCamera(&input, vec3(debugCameraRoomSpace, debugCameraRoomSpace, 300)));
            } else {
                pVRDevice = new OpenVR_Device(false);
            }

            if (!pVRDevice->isInit()) {
                std::cout << "HMD no detected, use sofware device instead." << std::endl;
                delete pVRDevice;
                pVRDevice = new SoftVR_Device(new VRDebugCamera(&input, vec3(debugCameraRoomSpace, debugCameraRoomSpace, 300)));
            }

            RES_X = pVRDevice->hmdResolution().x();
            RES_Y = pVRDevice->hmdResolution().y();
            std::cout << "HMD resolution:" << RES_X << "x" << RES_Y << std::endl;

			/* Pipeline entity */
			FullPipeline pipeline;
			FullPipeline::Parameter pipelineParam;
            pipelineParam.useShadow = true;
            pipelineParam.usePointLight = true;
            pipelineParam.shadowCascad = vector<float>({5, 25});
            pipelineParam.shadowResolution = 2048;
            pipelineParam.useSSReflexion = false;
            pipelineParam.usePostSSReflexion = false;
            pipelineParam.useFxaa = true;

            OnHmdRenderer* hmdNode = new OnHmdRenderer;
            hmdNode->setDrawOnScreen(2);
            float ratio = float(RES_X)/RES_Y;
            hmdNode->setScreenResolution({ WIN_RES.x(), WIN_RES.y() });
            hmdNode->setShader(ShaderPool::instance().get("feedbackStereo"));

            pipeline.createStereoExtensible(*hmdNode, {RES_X,RES_Y}, pipelineParam);

            hmdNode->setVRDevice(pVRDevice);

            HmdSceneView hmdCamera(110, ratio, 500);
            hmdCamera.setScaleAndShiftRoom(3 / roomSize, zShift);

            pipeline.setStereoView(hmdCamera.cullingView(), hmdCamera.eyeView(0), hmdCamera.eyeView(1), 0);

            /* physic and setup */
            BulletEngine physEngine;
            ThreadPool threadPool(4);

            MultipleSceneHelper portalManager(pipelineParam, pipeline);
            portalManager.setResolution({RES_X,RES_Y});
            portalManager.setView(hmdCamera.cullingView());
            portalManager.setStereoView(hmdCamera.eyeView(0), hmdCamera.eyeView(1));
            portalManager.extendPipeline(NB_MAX_PIPELINE);

            switch(indexLevel)
            {
            case 1: indexLevel = 0; break;
            case 2: indexLevel = 2; break;
            case 3: indexLevel = 4; break;
            case 4: indexLevel = 5; break;
            case 5: indexLevel = 9; break;
            default: indexLevel = 0; break;
            }

            PortalGame portalGame(physEngine, portalManager, hmdCamera, *pVRDevice, indexLevel);

            pVRDevice->sync();

            float freqphys = 500;
            btSphereShape ballShape(0.075);

            float timeElapsed = 0, totalTime = 0;
            float scaleRoom = 1;

			while (!input.keyState(SDLK_ESCAPE).pressed)
			{
                SDLTimer timer;

                input.getEvent();
                threadPool.wait();

                if(input.keyState(SDLK_x).firstPress)
                {
                    scaleRoom -= 0.05;
                    hmdCamera.setScaleAndShiftRoom(scaleRoom, zShift);
                    std::cout << "Scale room:" << scaleRoom << std::endl;
                }
                if(input.keyState(SDLK_y).firstPress)
                {
                    scaleRoom += 0.05;
                    hmdCamera.setScaleAndShiftRoom(scaleRoom, zShift);
                    std::cout << "Scale room:" << scaleRoom << std::endl;
                }

//                if(input.keyState(SDLK_n).firstPress && !input.keyState(SDLK_r).pressed)
//                {
//                    portalGame.controllers().DAMPING -= 10;
//                    std::cout << "DAMPING:" << portalGame.controllers().DAMPING << std::endl;
//                }
//                if(input.keyState(SDLK_m).firstPress && !input.keyState(SDLK_r).pressed)
//                {
//                    portalGame.controllers().DAMPING += 10;
//                    std::cout << "DAMPING:" << portalGame.controllers().DAMPING << std::endl;
//                }

//                if(input.keyState(SDLK_k).firstPress && !input.keyState(SDLK_r).pressed)
//                {
//                    portalGame.controllers().STRENGTH -= 50;
//                    std::cout << "STRENGTH:" << portalGame.controllers().STRENGTH << std::endl;
//                }
//                if(input.keyState(SDLK_l).firstPress && !input.keyState(SDLK_r).pressed)
//                {
//                    portalGame.controllers().STRENGTH += 50;
//                    std::cout << "STRENGTH:" << portalGame.controllers().STRENGTH << std::endl;
//                }

//                if(input.keyState(SDLK_n).firstPress && input.keyState(SDLK_r).pressed)
//                {
//                    portalGame.controllers().DAMPING_R -= 0.1;
//                    std::cout << "DAMPING_R:" << portalGame.controllers().DAMPING_R << std::endl;
//                }
//                if(input.keyState(SDLK_m).firstPress && input.keyState(SDLK_r).pressed)
//                {
//                    portalGame.controllers().DAMPING_R += 0.1;
//                    std::cout << "DAMPING_R:" << portalGame.controllers().DAMPING_R << std::endl;
//                }

//                if(input.keyState(SDLK_k).firstPress && input.keyState(SDLK_r).pressed)
//                {
//                    portalGame.controllers().STRENGTH_R -= 1;
//                    std::cout << "STRENGTH_R:" << portalGame.controllers().STRENGTH_R << std::endl;
//                }
//                if(input.keyState(SDLK_l).firstPress && input.keyState(SDLK_r).pressed)
//                {
//                    portalGame.controllers().STRENGTH_R += 1;
//                    std::cout << "STRENGTH_R:" << portalGame.controllers().STRENGTH_R << std::endl;
//                }

                if(input.keyState(SDLK_p).firstPress)
                {
                    freqphys += 10;
                    std::cout << "PHYSIQUE_FREQ:" << freqphys << std::endl;
                }
                if(input.keyState(SDLK_o).firstPress)
                {
                    freqphys -= 10;
                    std::cout << "PHYSIQUE_FREQ:" << freqphys << std::endl;
                }

                if(input.keyState(SDLK_b).firstPress)
                {
                    portalGame.popBoxDebug();
                }
                if(input.keyState(SDLK_v).firstPress)
                {
                    portalGame.levelSystem().callDebug();
                }

                /*********************/
                /******* Flush *******/
                /*********************/
                int sceneIndex = portalGame.curSceneIndex();
                int nbLevel = portalGame.levelSystem().nbLevels();

#if 1
                portalGame.update(timeElapsed);

                for (int i = 0; i < nbLevel; ++i)
                    if (physEngine.dynamicsWorld[i])
                        physEngine.dynamicsWorld[i]->stepSimulation(std::min(timeElapsed, 1.f / 60), 20, 1.0 / freqphys);

                pipeline.pipeline()->prepare();
#else
                auto futur_updateGame = threadPool.schedule_trace([&]()
                {
                    /*PROFILE("Update game")*/ portalGame.update(timeElapsed);
                    //endUpdateGamePromise.set_value();
                    return true;
                });

                auto futur_prepare = threadPool.schedule_trace([&]()
                {
                    futur_updateGame.wait();
                    /*PROFILE("Pipeline prepare")*/ pipeline.pipeline()->prepare();
                    return true;
                });

                
                threadPool.schedule([&physEngine, timeElapsed, sceneIndex, freqphys, nbLevel, &futur_updateGame]()
                {
                    futur_updateGame.wait();
                    for(int i=0 ; i<nbLevel ; ++i)
                        if(physEngine.dynamicsWorld[i])
                            physEngine.dynamicsWorld[i]->stepSimulation(std::min(timeElapsed, 1.f/60), 20, 1 / freqphys);
               });
#endif

                pVRDevice->update(timeElapsed);
                hmdCamera.update(*pVRDevice);

                portalManager.updateCameras();

                /********************/
                /******* Draw *******/
                /********************/

                pipeline.pipeline()->render();
                swapBuffer();
                pVRDevice->sync();
                GL_ASSERT();

                if(input.keyState(SDLK_p).firstPress)
                    std::cout << "Cam pos:" << hmdCamera.cullingView().camera.pos << std::endl;

				timeElapsed = timer.elapsed()*0.001;
				totalTime += timeElapsed;
                pipeline.pipeline()->meshRenderer().frameParameter().setTime(totalTime, timeElapsed);

                {
                    float fps = countTime<0>(timeElapsed);
                   
                    if (fps > 0) {
                        std::cout << "Fps:" << 1.f / fps << "  Ms:" << 1000 * fps << " : " << pipeline.pipeline()->meshRenderer().getStats()._numTriangles << " Triangles" << std::endl;
                        pipeline.pipeline()->meshRenderer().resetStats();
                    }
                }

                pipeline.pipeline()->meshRenderer().resetStats();
			}

			/** Close context **/
			threadPool.wait();

            AssetManager<Geometry>::freeInstance();
            AssetManager<interface::Texture>::freeInstance();
            ShaderPool::freeInstance();
            openGL.execAllGLTask();
		}

		delete resource::textureLoader;
		tim::renderer::close();
        delContextSDL();
	}
	tim::core::quit();
	LOG("Quit\n");

	return 0;
}
