#include "TIM_SDL/SDLTimer.h"
#include "OnHmdRenderer.h"
#include "core/core.h"
#undef interface


namespace tim
{
	using namespace core;

	OnHmdRenderer::OnHmdRenderer() : interface::Pipeline::TerminalNode()
	{
		_stateDrawQuad.setCullFace(false);
		_stateDrawQuad.setDepthTest(false);
		_stateDrawQuad.setWriteDepth(false);
		_stateDrawQuad.setShader(renderer::drawQuadShader);

        renderer::FrameBuffer _fboBuffer[2];
	}

	OnHmdRenderer::~OnHmdRenderer()
	{
		delete _textureBuffer[0];
		delete _textureBuffer[1];
	}

	void OnHmdRenderer::prepare()
	{
		if (!tryPrepare()) return;

		for (uint i = 0; i<_input.size(); ++i)
		{
			if (_input[i])
				_input[i]->prepare();
		}
	}

	void OnHmdRenderer::render()
    {
		if (!tryRender()) return;

		for (uint i = 0; i<_input.size(); ++i)
		{
			if (_input[i])
            {
                _input[i]->acquire(0);
                _input[i]->render();
                if(_textureBuffer[i])
                    _fboInput[i].copyTo(_fboBuffer[i]);
            }
		}

        if (!_textureBuffer[0] && _input[0] && _input[1])
        {
            renderer::Texture::GenTexParam param;
            param.size = uivec3(_input[0]->buffer()->resolution(), 0);
            param.nbLevels = 1;
            param.format = renderer::Texture::Format::RGBA8;
            _textureBuffer[VR_DeviceInterface::LEFT] = renderer::Texture::genTexture2D(param);
            _textureBuffer[VR_DeviceInterface::RIGHT] = renderer::Texture::genTexture2D(param);

            _fboBuffer[VR_DeviceInterface::LEFT].attachTexture(0, _textureBuffer[VR_DeviceInterface::LEFT]);
            _fboBuffer[VR_DeviceInterface::RIGHT].attachTexture(0, _textureBuffer[VR_DeviceInterface::RIGHT]);

            _fboInput[VR_DeviceInterface::LEFT].attachTexture(0, _input[VR_DeviceInterface::LEFT]->buffer());
            _fboInput[VR_DeviceInterface::RIGHT].attachTexture(0, _input[VR_DeviceInterface::RIGHT]->buffer());
        }

		if (_drawOnScreen >= 0 && _input.size() > 0)
		{
			renderer::openGL.bindFrameBuffer(0);

            renderer::openGL.setViewPort({0,0}, _screenResolution);
			_stateDrawQuad.bind();

            renderer::openGL.bindTextureSampler(renderer::textureSampler[renderer::TextureMode::FilteredNoRepeat], 0);

            if(_drawOnScreen >= 2)
            {
                renderer::openGL.bindTextureSampler(renderer::textureSampler[renderer::TextureMode::FilteredNoRepeat], 1);
                renderer::openGL.bindTexture(_textureBuffer[VR_DeviceInterface::LEFT]->id(), GL_TEXTURE_2D, 0);
                renderer::openGL.bindTexture(_textureBuffer[VR_DeviceInterface::RIGHT]->id(), GL_TEXTURE_2D, 1);
            }
            else
                renderer::openGL.bindTexture(_input[std::min(_drawOnScreen, 1)]->buffer()->id(), GL_TEXTURE_2D, 0);

            renderer::quadMeshBuffers->draw(6, renderer::VertexMode::TRIANGLES, 1);
		}

        // Submit to hmd

		if (_device->isInit() && _input[0] && _input[1])
		{
            if (!_invertEyes) {
                _device->submit(_textureBuffer[VR_DeviceInterface::LEFT], _textureBuffer[VR_DeviceInterface::RIGHT]);
            } else {
                _device->submit(_textureBuffer[VR_DeviceInterface::RIGHT], _textureBuffer[VR_DeviceInterface::LEFT]);
            }
        }

        for (uint i = 0; i<_input.size(); ++i)
        {
            if (_input[i])
                _input[i]->release(0);
        }

	}

}

