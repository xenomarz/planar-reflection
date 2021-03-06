// ImGui includes
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

// OpenGL includes
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <nfd.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>

// STL includes
#include <string>
#include <memory>

// Local includes
#include "material.h"
#include "point_light.h"
#include "obj_model.h"
#include "shader_program.h"
#include "camera.h"

/**
 * Defines
 */
#define VERTEX_SHADER_PATH ".//shaders//vertex.glsl"
#define FRAGMENT_SHADER_PATH ".//shaders//fragment.glsl"
#define PLANE_MODEL_PATH ".//models//obj//plane.obj"
#define INITIAL_WIDTH 1024
#define INITIAL_HEIGHT 768

/**
 * Function definitions
 */
void TransformModels(const std::vector<std::shared_ptr<ObjModel>>& models);
void RenderModels(const std::vector<std::shared_ptr<ObjModel>>& models, const std::shared_ptr<PointLight>& active_light, const glm::vec3& plane_normal, float distance, ShaderProgram& shader_program, bool mirror);
void RenderPlane(const std::vector<std::shared_ptr<ObjModel>>& models, const std::shared_ptr<PointLight>& active_light, const glm::vec3& position, const glm::vec3& normal, ShaderProgram& shader_program);
glm::mat4 CalculateRotationMatrix(const glm::mat4& mat, const glm::vec3& vec1, const glm::vec3& vec2);
glm::mat4 CalculateReflectionMatrix(const glm::vec3& normal);

/**
 * Main
 */
int main(int, char**)
{
	// Scene objects
    std::vector<std::shared_ptr<ObjModel>> models;
    std::vector<std::shared_ptr<PointLight>> point_lights;
    std::vector<std::shared_ptr<Camera>> cameras;
    uint32_t active_camera = 0;
    uint32_t active_light = 0;
    uint32_t active_model = 1;

	// OpenGL objects
    GLFWwindow* window;
    glm::vec4 clear_color(0.45f, 0.55f, 0.60f, 1.00f);
    glm::vec4 model_color(0, 1, 0.5, 1.00f);
    int width = INITIAL_WIDTH;
    int height = INITIAL_HEIGHT;
	
    // Initialize GLFW
    if (!glfwInit())
    {
        return -1;
    }

	// Use modern OpenGL version 3.3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    // Create window-mode rendering context
    window = glfwCreateWindow(INITIAL_WIDTH, INITIAL_HEIGHT, "Planar Reflection Demo", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    // Set the previously created window as the current rendering context
    glfwMakeContextCurrent(window);

    // Initialize GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK)
    {
        glfwTerminate();
        return -1;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer bindings
    const char* glsl_version = "#version 330";
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

	// Create shader program
    ShaderProgram shader_program(VERTEX_SHADER_PATH, FRAGMENT_SHADER_PATH);

	/**
	 * Create scene objects
	 */

	// Plane
	glm::vec3 plane_position(0, 0, 0);
	glm::vec3 plane_normal(0, 1, -1);
    auto plane_model = std::make_shared<ObjModel>(PLANE_MODEL_PATH);
    models.push_back(plane_model);
    plane_model->GetMaterial().SetAmbientColor(glm::vec3(0.6,0.6,0.6));
    plane_model->GetMaterial().SetDiffuseColor(glm::vec3(0.6, 0.6, 0.6));

	// Camera
	auto camera = std::make_shared<Camera>(
		glm::vec3(0,0,-6), 
		glm::vec3(0,0,0), 
		glm::vec3(0,1,0),
		float(width) / float(height),
        0.1f,
        100);

	// Point light
	auto point_light = std::make_shared<PointLight>(
        glm::vec3(0, 0, -8),
        glm::vec3(0.2, 0.2, 0.2),
        glm::vec3(0.5, 0.5, 0.5));

	// Move scene objects to corresponding lists
    cameras.push_back(camera);
	point_lights.push_back(point_light);

    glEnable(GL_DEPTH_TEST);

    /**
     * Main loop
     */
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        /**
         * ImGui stuff
         */
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::Begin("Menu");
        if (ImGui::Button("Load model..."))
        {
            nfdchar_t* file_path_ptr = NULL;
            nfdresult_t result = NFD_OpenDialog("obj;png,jpg", NULL, &file_path_ptr);
            if (result == NFD_OKAY)
            {
                auto model = std::make_shared<ObjModel>(std::string(file_path_ptr));
                models.push_back(model);
                model->GetMaterial().SetAmbientColor(model_color);
                model->GetMaterial().SetDiffuseColor(model_color);
            }
            else if (result == NFD_CANCEL)
            {
            	
            }
            else
            {
            	
            }
        }
    	
        ImGui::ColorEdit3("Clear color", (float*)&clear_color);
        if(ImGui::ColorEdit3("Model color", (float*)&model_color))
        {
			for(size_t i = 1; i < models.size(); i++)
            {
                models[i]->GetMaterial().SetAmbientColor(model_color);
                models[i]->GetMaterial().SetDiffuseColor(model_color);
			}
        }
    	
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();
        ImGui::Render();

		/**
		 * Scene rendering
		 */

    	// Prepare new frame
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);
        glClearColor(clear_color.r, clear_color.g, clear_color.b, clear_color.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    	// Reset aspect ratio
        cameras[active_camera]->SetAspectRatio(static_cast<float>(width) / static_cast<float>(height));

    	// Set shader program to use
        shader_program.Use();

        // Set lights
        //shader_program.SetUniform("point_light.position", point_lights[active_light]->GetPosition());
        shader_program.SetUniform("point_light.ambient", point_lights[active_light]->GetAmbientLight());
        shader_program.SetUniform("point_light.diffuse", point_lights[active_light]->GetDiffuseLight());

        // Set view and projection transformations
        shader_program.SetUniform("view", cameras[active_camera]->GetViewTransform());
        shader_program.SetUniform("projection", cameras[active_camera]->GetProjectionTransform());

		// Rotate models around y-axis  
        TransformModels(models);

    	// Render models
        RenderModels(models, point_lights[active_light], plane_normal, 2.0f, shader_program, false);

    	// Enable stencil test
        glEnable(GL_STENCIL_TEST);

    	// Write 1 in stencil buffer at all fragments of the next render (mirror)
        glStencilFunc(GL_ALWAYS, 1, 0xFF);
        glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
        glStencilMask(0xFF);

    	// Do not write to z-buffer (mirror)
        glDepthMask(GL_FALSE);

    	// Render mirror plane
        RenderPlane(models, point_lights[active_light], plane_position, plane_normal, shader_program);

    	// Set teh stencil test to pass only at fragments which were rendered by the mirror plane
        glStencilFunc(GL_EQUAL, 1, 0xFF);

    	// Do not write to stencil buffer
        glStencilMask(0x00);

    	// Write to z-buffer, so mirrored objects will appear correctly in mirror
        glDepthMask(GL_TRUE);

    	// Render mirrored objects
        RenderModels(models, point_lights[active_light], plane_normal, 2.0f, shader_program, true);

    	// Disable stencil test
        glDisable(GL_STENCIL_TEST);

        // Render ImGui
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    	// Swap buffers
        glfwSwapBuffers(window);
    }

    /**
     * Cleanup
     */

	// Cleanup ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

	// Cleanup GLFW
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

void RenderPlane(const std::vector<std::shared_ptr<ObjModel>>& models, const std::shared_ptr<PointLight>& active_light, const glm::vec3& position, const glm::vec3& normal, ShaderProgram& shader_program)
{
    auto model = models[0];

	// Set light position
	shader_program.SetUniform("point_light.position", active_light->GetPosition());
	
    // Set material
    shader_program.SetUniform("material.ambient", model->GetMaterial().GetAmbientColor());
    shader_program.SetUniform("material.diffuse", model->GetMaterial().GetDiffuseColor());

	// Scale plane
	glm::mat4 local_transform = glm::translate(CalculateRotationMatrix(glm::scale(glm::mat4(1), glm::vec3(2, 2, 2)), glm::vec3(0,1,0), normal), position);
    model->SetLocalTransform(local_transform);

    // Update model transform uniform
    shader_program.SetUniform("model", model->GetModelTransform());

	// Render
    model->Render();
}

void TransformModels(const std::vector<std::shared_ptr<ObjModel>>& models)
{
    for (size_t i = 1; i < models.size(); i++)
    {
        auto model = models[i];
        auto local_transform = model->GetLocalTransform();
        local_transform = glm::rotate(local_transform, glm::pi<float>() / 300, glm::vec3(0, 1, 0));
        model->SetLocalTransform(local_transform);
    }
}

void RenderModels(
	const std::vector<std::shared_ptr<ObjModel>>& models,
	const std::shared_ptr<PointLight>& active_light,
	const glm::vec3& plane_normal,
	float distance,
	ShaderProgram& shader_program,
	bool mirror)
{
	auto light_position = active_light->GetPosition();
	glm::vec3 normalized_plane_normal = glm::normalize(plane_normal);
	float light_distance = glm::dot(active_light->GetPosition(), normalized_plane_normal);

	if (mirror)
	{
		light_position = light_position - 2.0f * light_distance * normalized_plane_normal;
	}

	shader_program.SetUniform("point_light.position", glm::vec3(light_position));



    for (size_t i = 1; i < models.size(); i++)
    {
        auto model = models[i];

        // Set material
        const float factor = mirror ? 1.0 : 1.0;
        shader_program.SetUniform("material.ambient", factor * model->GetMaterial().GetAmbientColor());
        shader_program.SetUniform("material.diffuse", factor * model->GetMaterial().GetDiffuseColor());

    	// Determine y-axis offset

		


		glm::vec3 offset = normalized_plane_normal * distance;
        glm::mat4 world_transform = glm::mat4(1);

    	// If requested, mirror across the XZ plane
        if (mirror)
        {
			//glm::mat4 rot = CalculateRotationMatrix(glm::mat4(1), glm::vec3(0, 1, 0), -plane_normal);

			glm::mat4 reflection = CalculateReflectionMatrix(normalized_plane_normal);

			//world_transform = reflection;

			world_transform = reflection * glm::translate(glm::mat4(1), offset);
			//world_transform = rot;
        }
		else
		{
			world_transform = glm::translate(world_transform, offset);
		}

    	// Place the model above the XZ plane
        //world_transform = glm::translate(world_transform, glm::vec3(0, offset, 0));

    	// Set world transform
        model->SetWorldTransform(world_transform);

    	// Update model transform uniform
        shader_program.SetUniform("model", model->GetModelTransform());

    	// Render
        model->Render();
    }
}

glm::mat4 CalculateRotationMatrix(const glm::mat4& mat, const glm::vec3& vec1, const glm::vec3& vec2)
{
	auto vec1_normalized = glm::normalize(vec1);
	auto vec2_normalized = glm::normalize(vec2);
	glm::vec3 axis = glm::cross(vec2_normalized, vec1_normalized);
	float angle = -glm::acos(glm::dot(vec1_normalized, vec2_normalized));
	return glm::rotate(mat, angle, axis);
}

glm::mat4 CalculateReflectionMatrix(const glm::vec3& normal)
{
	float x = normal.x;
	float y = normal.y;
	float z = normal.z;
	glm::mat4 reflection = glm::mat4
	(
		1 - 2 * x * x, -2 * y * x, -2 * z * x, 0.0,
		-2 * y * x, 1 - 2 * y * y, -2 * y * z, 0.0,
		-2 * z * x, -2 * y * z, 1 - 2 * z * z, 0.0,
		0.0, 0.0, 0.0, 1.0
	);

	return reflection;
}