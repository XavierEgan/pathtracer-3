#include <glm/glm.hpp>
#include <glm/ext/scalar_relational.hpp>
#include <vector>
#include <string>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

float EPSILON = 1e-4;

struct Rng {
    uint32_t s;
    float next() {
        s ^= s << 13; s ^= s >> 17; s ^= s << 5;
        return (s >> 8) * (1.0f / 16777216.0f);
    }
};

struct Frame {
    char* data;
    int width;
    int height;

    Frame(int x, int y) : data(new char[x * y * 3]), width(x), height(y) {}
    ~Frame() {
        delete[] data;
    }
    Frame(const Frame&) = delete;
    Frame& operator=(const Frame&) = delete;
    Frame(Frame&&) = delete;
    Frame& operator=(Frame&&) = delete;

    void write_pixel(int x, int y, glm::u8vec3 color) {
        data[(y * width + x) * 3] = color.r;
        data[(y * width + x) * 3 + 1] = color.g;
        data[(y * width + x) * 3 + 2] = color.b;
    }

    void save_image(const std::string& path) const {
        stbi_write_png(path.c_str(), width, height, 3, data, width * 3);
    }
};

struct SwapChain {
    Frame cur_frame;
    Frame next_frame;

    SwapChain(int x, int y) : cur_frame(Frame(x, y)), next_frame(Frame(x, y)) {}
};

struct Material {
    glm::vec3 color;
    float roughness;
};

struct Ray {
    glm::vec3 origin;
    glm::vec3 direction;

    bool intersect(glm::vec3 v0, glm::vec3 v1, glm::vec3 v2, float& t, glm::vec2& uv, glm::vec3& pos) const {
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

        float tt = inv_det * dot(edge2, s_cross_e1);

        if (tt > EPSILON) {
            t = tt;
            uv = glm::vec2(u, v);
            pos = origin + direction * tt;
            return true;
        }

        return false;
    }
};


struct Camera {
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

        cam.pos = origin;

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
        float pixel_size = 2.0f * tan(glm::radians(horizontal_fov) * 0.5f) / horizontal_pixels;
        
        glm::vec3 top_left_pixel = forward + right * ((x - (horizontal_pixels / 2)) * pixel_size) + up * -((y - (vertical_pixels / 2.0f)) * pixel_size);
        glm::vec3 rand_point_in_pixel = top_left_pixel + right * rng.next() * pixel_size - up * rng.next() * pixel_size;
        glm::vec3 dir = rand_point_in_pixel - pos;
        Ray ray = Ray();
        ray.origin = pos;
        ray.direction = dir;
        return ray;
    }
};

struct Scene {
    std::vector<glm::vec3> verts;
    std::vector<int> indices;

    std::vector<Material> materials;
    std::vector<int> mat_indices;

    Camera camera;

    void add_tri(glm::vec3 v0, glm::vec3 v1, glm::vec3 v2) {
        // winded v2 -> v1 -> v0
        // CCW winding
        verts.push_back(v0);
        verts.push_back(v1);
        verts.push_back(v2);
        
        indices.push_back(verts.size() - 1);
        indices.push_back(verts.size() - 2);
        indices.push_back(verts.size() - 3);
    }

    void add_tri(glm::vec3 v0, glm::vec3 v1, glm::vec3 v2, Material material) {
        // kinda jank since you cant share materials atm
        add_tri(v0, v1, v2);
        materials.push_back(material);
        mat_indices.push_back(mat_indices.size());
    }

    void add_camera(Camera camera) {
        this->camera = camera;
    }
};

bool find_intersecting_triangle(const Scene& scene, const Ray& ray, float& t, glm::vec2& uv, glm::vec3& pos, int& tri_indx) {
    float min_t = 9999999999.0f;
    glm::vec2 min_uv;
    glm::vec3 min_pos;
    bool intersected = false;

    for (auto tri{0uz}; tri < scene.indices.size() / 3; tri++) {
        if (!ray.intersect(
            scene.verts[scene.indices[tri * 3]],
            scene.verts[scene.indices[tri * 3 + 1]],
            scene.verts[scene.indices[tri * 3 + 2]],
            t, uv, pos
        )) continue;

        intersected = true;

        if (t < min_t) {
            min_uv = uv;
            min_pos = pos;
            tri_indx = tri;
            min_t = t;
        }
    }
    uv = min_uv;
    pos = min_pos;

    return intersected;
}

glm::u8vec3 shader(int x, int y, const Frame& prev_frame, Scene& scene) {
    glm::vec3 acc{0, 0, 0};
    for (int r = 0; r < 100; r++) {
        Ray ray = scene.camera.generate_camera_ray(x, y);

        float t;
        glm::vec2 uv;
        glm::vec3 pos;
        int tri_indx;

        if (find_intersecting_triangle(scene, ray, t, uv, pos, tri_indx)) {
            acc += scene.materials[scene.mat_indices[tri_indx]].color;
        }
    }

    return acc * (1/100.0f);
}

void render(SwapChain& swap_chain, Scene& scene) {
    // renders image to next_frame in the swapchain

    for (int y = 0; y < scene.camera.vertical_pixels; y++) {
        for (int x = 0; x < scene.camera.horizontal_pixels; x++) {
            swap_chain.next_frame.write_pixel(x, y, shader(x, y, swap_chain.cur_frame, scene));
        }
    }
}

void test_scene_1() {
    Scene scene = Scene();
    scene.add_tri(
        {0, 0, 0},
        {0, .5, 0},
        {0, 0, .5},
        Material{{255, 0, 0}, 0}
    );

    scene.add_camera(Camera::look_at(
        {0, 0, 0},
        {-5, 0, 0},
        {0, 1, 0},
        854, 480,
        90,
        1
    ));

    SwapChain swap_chain = SwapChain(scene.camera.horizontal_pixels, scene.camera.vertical_pixels);

    render(swap_chain, scene);

    swap_chain.next_frame.save_image("test.png");
}

int main() {
    test_scene_1();
}
