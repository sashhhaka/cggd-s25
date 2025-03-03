#pragma once

#include "renderer/rasterizer/rasterizer.h"
#include "renderer/renderer.h"
#include "resource.h"


namespace cg::renderer
{
	class rasterization_renderer : public renderer
	{
	public:
		virtual void init();
		virtual void destroy();

		virtual void update();
		virtual void render();

	protected:
		std::shared_ptr<cg::resource<cg::unsigned_color>> render_target;
		std::shared_ptr<cg::resource<float>> depth_buffer;
		
		// New buffer for storing original colors (for transparency blending)
		std::shared_ptr<cg::resource<cg::unsigned_color>> original_color_buffer;
		
		// Alpha value for transparency
		float alpha_value = 0.5f;

		std::shared_ptr<cg::renderer::rasterizer<cg::vertex, cg::unsigned_color>> rasterizer;
	};
}// namespace cg::renderer