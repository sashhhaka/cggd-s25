#include "rasterizer_renderer.h"
#include "utils/resource_utils.h"

#include <random>
#include <ctime>

// Modify the pixel_shader function in rasterization_renderer.cpp

// Add these includes at the top of the file
#include <random>
#include <ctime>

void cg::renderer::rasterization_renderer::init()
{
    rasterizer = std::make_shared<cg::renderer::rasterizer<cg::vertex, cg::unsigned_color>>();
    rasterizer->set_viewport(settings->width, settings->height);
    render_target = std::make_shared<cg::resource<cg::unsigned_color>>(settings->width, settings->height);

    depth_buffer = std::make_shared<cg::resource<float>>(settings->width, settings->height);
    
    // Add a new buffer to store the original color values for blending
    original_color_buffer = std::make_shared<cg::resource<cg::unsigned_color>>(settings->width, settings->height);

    rasterizer->set_render_target(render_target, depth_buffer);

    model = std::make_shared<cg::world::model>();
    model->load_obj(settings->model_path);

    for (size_t i=0; i<model->get_index_buffers().size(); i++)
    {
        auto index_buffer_size = model->get_index_buffers()[i]->size_bytes();
        auto vertex_buffer_size = model->get_vertex_buffers()[i]->size_bytes();

        auto pure_vertex_buffer_size = model->get_index_buffers()[i]->count() * sizeof(cg::vertex);

        std::cout << "Vertex buffer size: " << vertex_buffer_size << " bytes\n";
        std::cout << "Index buffer size: " << index_buffer_size << " bytes\n";
        std::cout << "Pure vertex buffer size: " << pure_vertex_buffer_size << " bytes\n";
        std::cout << "Saving: " << pure_vertex_buffer_size - vertex_buffer_size - index_buffer_size << " bytes\n";
    }

    camera = std::make_shared<cg::world::camera>();
    camera->set_height(static_cast<float>(settings->height));
    camera->set_width(static_cast<float>(settings->width));
    camera->set_position(float3{
        settings->camera_position[0],
        settings->camera_position[1],
        settings->camera_position[2]
    });

    camera->set_phi(settings->camera_phi);
    camera->set_theta(settings->camera_theta);
    camera->set_angle_of_view(settings->camera_angle_of_view);
    camera->set_z_near(settings->camera_z_near);
    camera->set_z_far(settings->camera_z_far);

    // Store the alpha value from settings
    alpha_value = settings->alpha;
    std::cout << "Using alpha value: " << alpha_value << std::endl;
    
    // Initialize noise settings from command line parameters or use defaults
    noise_amplitude = settings->noise_amplitude;
    noise_frequency = settings->noise_frequency;
    std::cout << "Using noise amplitude: " << noise_amplitude << std::endl;
    std::cout << "Using noise frequency: " << noise_frequency << std::endl;
    
    // Seed the random number generator
    random_generator.seed(static_cast<unsigned int>(std::time(nullptr)));
}

void cg::renderer::rasterization_renderer::render()
{
    float4x4 matrix = mul(
        camera->get_projection_matrix(),
        camera->get_view_matrix(),
        model->get_world_matrix());

    using namespace linalg::ostream_overloads;
    std::cout << camera->get_projection_matrix() << std::endl;
    std::cout << camera->get_view_matrix() << std::endl;

    rasterizer->vertex_shader = [&](float4 vertex, cg::vertex vertex_data) {
        float4 processed = mul(matrix, vertex);
        return std::make_pair(processed, vertex_data);
    };

    // First render: clear to background color
    auto start = std::chrono::high_resolution_clock::now();
    rasterizer->clear_render_target({111, 15, 112});
    
    // Store the background color
    for (size_t i = 0; i < render_target->count(); i++) {
        original_color_buffer->item(i) = render_target->item(i);
    }
    
    auto stop = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float, std::milli> duration = stop - start;
    std::cout << "Clearing took " << duration.count() << "ms\n";

    // Distribution for generating random noise
    std::uniform_real_distribution<float> noise_dist(-1.0f, 1.0f);

    // Modify the pixel shader to use alpha blending and apply noise
    rasterizer->pixel_shader = [&](cg::vertex vertex_data, float z) {
        // Get the current position in 2D
        int x = static_cast<int>(vertex_data.position.x);
        int y = static_cast<int>(vertex_data.position.y);
        
        // Clamp coordinates to ensure they're within bounds
        x = std::max(0, std::min(static_cast<int>(settings->width) - 1, x));
        y = std::max(0, std::min(static_cast<int>(settings->height) - 1, y));
        
        // Get source color (object color)
        cg::color source_color = cg::color::from_float3(vertex_data.ambient);
        
        // Apply noise to the color based on position and normal
        float noise_value = noise_dist(random_generator);
        
        // Create coherent noise based on position and normal
        float position_factor = std::sin(vertex_data.position.x * noise_frequency) * 
                                std::cos(vertex_data.position.y * noise_frequency) * 
                                std::sin(vertex_data.position.z * noise_frequency);
        
        // Use normal direction to influence noise (creates surface-aware noise)
        float normal_factor = std::abs(vertex_data.normal.x) + 
                              std::abs(vertex_data.normal.y) + 
                              std::abs(vertex_data.normal.z);
        
        // Combine noise types
        float combined_noise = noise_value * 0.3f + position_factor * 0.5f + normal_factor * 0.2f;
        combined_noise *= noise_amplitude;
        
        // Apply noise to color (keeping within valid range)
        float3 noisy_color = source_color.to_float3();
        noisy_color.x = std::clamp(noisy_color.x + combined_noise, 0.0f, 1.0f);
        noisy_color.y = std::clamp(noisy_color.y + combined_noise, 0.0f, 1.0f);
        noisy_color.z = std::clamp(noisy_color.z + combined_noise, 0.0f, 1.0f);
        
        // Get destination color (background color)
        cg::color dest_color = cg::color::from_unsigned_color(original_color_buffer->item(x, y));
        
        // Perform alpha blending: result = alpha * source + (1 - alpha) * destination
        float3 blended_color = alpha_value * noisy_color + (1.0f - alpha_value) * dest_color.to_float3();
        
        return cg::color::from_float3(blended_color);
    };

    // Draw the model with transparency and noise effect
    for (size_t shape_id=0; shape_id<model->get_index_buffers().size(); shape_id++)
    {
        rasterizer->set_vertex_buffer(model->get_vertex_buffers()[shape_id]);
        rasterizer->set_index_buffer(model->get_index_buffers()[shape_id]);
        rasterizer->draw(
            model->get_index_buffers()[shape_id]->count(), 0);
    }

    cg::utils::save_resource(*render_target, settings->result_path);
}

void cg::renderer::rasterization_renderer::destroy() {}

void cg::renderer::rasterization_renderer::update() {}