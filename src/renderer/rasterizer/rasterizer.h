#pragma once

#include "resource.h"

#include <functional>
#include <iostream>
#include <linalg.h>
#include <limits>
#include <memory>


using namespace linalg::aliases;

static constexpr float DEFAULT_DEPTH = std::numeric_limits<float>::max();

namespace cg::renderer
{
	template<typename VB, typename RT>
	class rasterizer
	{
	public:
		rasterizer(){};
		~rasterizer(){};
		void set_render_target(
				std::shared_ptr<resource<RT>> in_render_target,
				std::shared_ptr<resource<float>> in_depth_buffer = nullptr);
		void clear_render_target(
				const RT& in_clear_value, const float in_depth = DEFAULT_DEPTH);

		void set_vertex_buffer(std::shared_ptr<resource<VB>> in_vertex_buffer);
		void set_index_buffer(std::shared_ptr<resource<unsigned int>> in_index_buffer);

		void set_viewport(size_t in_width, size_t in_height);

		void draw(size_t num_vertexes, size_t vertex_offset);

		std::function<std::pair<float4, VB>(float4 vertex, VB vertex_data)> vertex_shader;
		std::function<cg::color(const VB& vertex_data, const float z)> pixel_shader;

	protected:
		std::shared_ptr<cg::resource<VB>> vertex_buffer;
		std::shared_ptr<cg::resource<unsigned int>> index_buffer;
		std::shared_ptr<cg::resource<RT>> render_target;
		std::shared_ptr<cg::resource<float>> depth_buffer;

		size_t width = 1920;
		size_t height = 1080;

		int edge_function(int2 a, int2 b, int2 c);
		bool depth_test(float z, size_t x, size_t y);
	};

	template<typename VB, typename RT>
	inline void rasterizer<VB, RT>::set_render_target(
			std::shared_ptr<resource<RT>> in_render_target,
			std::shared_ptr<resource<float>> in_depth_buffer)
	{
		if (in_render_target)
			render_target = in_render_target;
		// TODO Lab: 1.06 Adjust `set_render_target`, and `clear_render_target` methods of `cg::renderer::rasterizer` class to consume a depth buffer
	}

	template<typename VB, typename RT>
	inline void rasterizer<VB, RT>::set_viewport(size_t in_width, size_t in_height)
	{
		width = in_width;
		height = in_height;
	}

	template<typename VB, typename RT>
	inline void rasterizer<VB, RT>::clear_render_target(
			const RT& in_clear_value, const float in_depth)
	{
		if (render_target)
		{
			for (size_t i=0; i<render_target->count(); i++)
			{
				render_target->item(i) = in_clear_value;
			}
		}
		// TODO Lab: 1.06 Adjust `set_render_target`, and `clear_render_target` methods of `cg::renderer::rasterizer` class to consume a depth buffer
	}

	template<typename VB, typename RT>
	inline void rasterizer<VB, RT>::set_vertex_buffer(
			std::shared_ptr<resource<VB>> in_vertex_buffer)
	{
		vertex_buffer = in_vertex_buffer;
	}

	template<typename VB, typename RT>
	inline void rasterizer<VB, RT>::set_index_buffer(
			std::shared_ptr<resource<unsigned int>> in_index_buffer)
	{
		index_buffer = in_index_buffer;
	}

	template<typename VB, typename RT>
	inline void rasterizer<VB, RT>::draw(size_t num_vertexes, size_t vertex_offset)
	{
		size_t vertex_id = vertex_offset;

		while (vertex_id < vertex_offset + num_vertexes)
		{
			std::vector<VB> vertices(3);
			vertices[0] = vertex_buffer->item(index_buffer->item(vertex_id++));
			vertices[1] = vertex_buffer->item(index_buffer->item(vertex_id++));
			vertices[2] = vertex_buffer->item(index_buffer->item(vertex_id++));

			for (auto& vertex : vertices)
			{
				float4 coords{vertex.position.x, vertex.position.y, vertex.position.z, 1.f};
				auto processed_vertex = vertex_shader(coords, vertex);

				vertex.position = processed_vertex.first.xyz() / processed_vertex.first.w;

				vertex.position.x = (vertex.position.x + 1.f) * width / 2.f;
				vertex.position.y = (-vertex.position.y + 1.f) * height / 2.f;
			}
		}
		// TODO Lab: 1.05 Add `Rasterization` and `Pixel shader` stages to `draw` method of `cg::renderer::rasterizer`
		// TODO Lab: 1.06 Add `Depth test` stage to `draw` method of `cg::renderer::rasterizer`
	}

	template<typename VB, typename RT>
	inline int
	rasterizer<VB, RT>::edge_function(int2 a, int2 b, int2 c)
	{
		return (c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x);
	}

	template<typename VB, typename RT>
	inline bool rasterizer<VB, RT>::depth_test(float z, size_t x, size_t y)
	{
		if (!depth_buffer)
		{
			return true;
		}
		return depth_buffer->item(x, y) > z;
	}

}// namespace cg::renderer