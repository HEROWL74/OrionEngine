#include "Renderer.hpp"

namespace Engine::Graphics
{
	void Renderer::beginFrame()
	{
		/*
		TODO:
		// - Command Allocater Reset
		// - CommandList Reset
		// - BackBuffer ‚Ì ResourceBarrier(Present -> RenderTarget)
		*/

	}

	void Renderer::render(Scene& scene, Camera& camera)
	{
		// TODO À‘Ì‚ğ‚½‚¹‚é
		ID3D12GraphicsCommandList* commandList = nullptr;
		UINT frameIndex = 0;

		scene.render(commandList, camera, frameIndex);
	}


	void Renderer::endFrame()
	{
		/*
		TODO:
		// - CommandList Close
		// - ExecuteCommandLists
		// - SwapChain Present
		// - Fence
		*/
	}
}