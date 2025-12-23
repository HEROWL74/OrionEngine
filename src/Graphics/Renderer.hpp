#include <Windows.h>
#include <wrl.h>
#include <d3d12.h>
#include <array>

namespace Engine::Graphics
{
	class Camera;
	class Scene;

	class Renderer
	{
	public:
		Renderer() = default;
		~Renderer() = default;

		// ƒtƒŒ[ƒ€§Œä
		void beginFrame();
		void endFrame();

		void render(Scene& scene, Camera& camera);
	};
}