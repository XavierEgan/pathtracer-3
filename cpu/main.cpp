#include <string.h>
#include <glm/glm.hpp>
#include <glm/ext/scalar_relational.hpp>
#include <vector>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

float EPSILON = 1e-4;

struct Rng {
    int s;
    float next() {
        s ^= s << 13; s ^= s >> 17; s ^= s << 5;
        return (s >> 8) * (1.0f / 16777216.0f);
    }
};

class Frame {
    char* data;
    int width;
    int height;

    Frame(int x, int y) {
        data = new char[x * y * 3];
    }
    ~Frame() {
        delete[] data;
    }
    Frame(const Frame&) = delete;
    Frame& operator=(const Frame&) = delete;
    Frame(Frame&&) = delete;
    Frame& operator=(Frame&&) = delete;

    void write_pixel(int x, int y, glm::u8vec3& color) {
        data[(y * width + x) * 3] = color.r;
        data[(y * width + x) * 3 + 1] = color.g;
        data[(y * width + x) * 3 + 2] = color.b;
    }
};

class SwapChain {
    Frame cur_frame;
    Frame rendering_frame;
};

class Material {
    glm::vec3 color;
    float roughness;
};

class Scene {
    std::vector<glm::vec3> verts;
    std::vector<int> indices;

    void add_tri(glm::vec3 v0, glm::vec3 v1, glm::vec3 v2) {
        // winded v2 -> v1 -> v0
        // CCW winding
        add_vert(v0);
        add_vert(v1);
        add_vert(v2);
        
        add_index(verts.size() - 1);
        add_index(verts.size() - 2);
        add_index(verts.size() - 3);
    }

    void add_vert(const glm::vec3& v) {
        verts.push_back(v);
    }

    void add_index(int i) {
        indices.push_back(i);
    }
};

struct Ray {
    glm::vec3 origin;
    glm::vec3 direction;

    bool intersect(glm::vec3 v0, glm::vec3 v1, glm::vec3 v2, float& t, glm::vec2& uv, glm::vec3& pos) {
        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;

        // Backface culling
        const glm::vec3 normal = cross(edge1, edge2);
        if (dot(normal, direction) > 0) return false;

        glm::vec3 ray_cross_e2 = cross(direction, edge2);
        float det = dot(edge1, ray_cross_e2);

        if (abs(det) < EPSILON) return false;

        float inv_det = 1.0f / det;
        glm::vec3 s = origin - v0;
        float u = inv_det * dot(s, ray_cross_e2);

        if (u < -EPSILON || u - 1 > EPSILON) return false;

        glm::vec3 s_cross_e1 = cross(s, edge1);
        float v = inv_det * dot(direction, s_cross_e1);

        if (v < -EPSILON || u + v - 1 > EPSILON) return false;

        float t = inv_det * dot(edge2, s_cross_e1);

        if (t > EPSILON) {
            uv = glm::vec2(u, v);
            pos = origin + direction * t;
            return true;
        }

        return false;
    }
};

class Camera {
    Ray center;
    glm::vec3 forward;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 pos;

    int horizontal_pixels;
    int vertical_pixels;
    float horizontal_fov;

    Rng rng;

    static Camera look_at(
        glm::vec3 point, 
        glm::vec3 origin, 
        glm::vec3 kinda_up, 
        int horizontal_pixels, 
        int vertical_pixels, 
        float horizontal_fov,
        int seed
    ) {
        Camera cam = Camera();

        cam.forward = glm::normalize(point - origin);
        cam.right = glm::normalize(glm::cross(cam.forward, kinda_up));
        cam.up = glm::normalize(glm::cross(cam.right, cam.forward));

        cam.horizontal_pixels = horizontal_pixels;
        cam.vertical_pixels = vertical_pixels;
        cam.horizontal_fov = horizontal_fov;

        cam.rng = Rng(seed);

        return cam;
    }

    Ray generate_camera_ray(int x, int y) {
        // imagine a plane 1 unit from camera in forward direction with our forward vector being normal to the plane
        // right and up are also right and up on the plane
        // pos is the origin
        float pixel_size = 2.0f * tan(horizontal_fov * 0.5f) / horizontal_pixels;
        
        glm::vec3 top_left_pixel = forward + right * ((x - (horizontal_pixels / 2)) * pixel_size) + up * -((y - (vertical_pixels / 2.0f)) * pixel_size);
        glm::vec3 rand_point_in_pixel = top_left_pixel + right * static_cast<float>(rand() / RAND_MAX) - up * static_cast<float>(rand() / RAND_MAX);
        glm::vec3 dir = rand_point_in_pixel - pos;
        Ray ray = Ray();
        ray.origin = pos;
        ray.direction = dir;
        return ray;
    }
};

glm::u8vec3 shader() {

}

void render(Frame& frame, Scene& scene) {

}

int main() {

}